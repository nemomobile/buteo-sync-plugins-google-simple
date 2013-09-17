/*
 * This file is part of buteo-gcontact-plugins package
 *
 * Copyright (C) 2013 Jolla Ltd. and/or its subsidiary(-ies).
 *
 * Contributors: Sateesh Kavuri <sateesh.kavuri@gmail.com>
 *               Mani Chandrasekar <maninc@gmail.com>
 *               Chris Adams <chris.adams@jollamobile.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <QLibrary>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QUrl>

#include <QContactGuid>
#include <QContactDetailFilter>
#include <QContactAvatar>
#include <QContactSyncTarget>
#include <QContactFetchHint>
#include <QContactSortOrder>
#include <QContactEmailAddress>
#include <QContactManager>

#include <Accounts/Manager>
#include <Accounts/Account>
#include <Accounts/Service>
#include <Accounts/AccountService>
#include <SignOn/SessionData>
#include <SignOn/Identity>
#include <SignOn/AuthSession>

#include <PluginCbInterface.h>
#include <LogMacros.h>
#include <ProfileEngineDefs.h>
#include <ProfileManager.h>

#include "googlecontacts.h"
#include "googleatom.h"
#include "googlestream.h"

#define MAX_CONTACT_ENTRY_RESULTS 500

#include <sailfishkeyprovider.h>

QString storedKeyValue(const char *provider, const char *service, const char *key)
{
    char *storedKey = NULL;
    QString retn;

    int success = SailfishKeyProvider_storedKey(provider, service, key, &storedKey);
    if (success == 0 && storedKey != NULL && strlen(storedKey) != 0) {
        retn = QLatin1String(storedKey);
    }

    free(storedKey);
    return retn;
}

extern "C" GoogleContactsClient* createPlugin(const QString& aPluginName,
        const Buteo::SyncProfile& aProfile,
        Buteo::PluginCbInterface *aCbInterface) {
    return new GoogleContactsClient(aPluginName, aProfile, aCbInterface);
}

extern "C" void destroyPlugin(GoogleContactsClient *aClient) {
    delete aClient;
}

GoogleContactsClient::GoogleContactsClient(const QString& aPluginName,
        const Buteo::SyncProfile& aProfile,
        Buteo::PluginCbInterface *aCbInterface)
    : ClientPlugin(aPluginName, aProfile, aCbInterface)
    , m_session(0)
    , m_identity(0)
    , m_accountManager(new Accounts::Manager(this))
    , m_qnam(new QNetworkAccessManager(this))
{
}

GoogleContactsClient::~GoogleContactsClient()
{
}

bool GoogleContactsClient::init()
{
    return true;
}

bool GoogleContactsClient::uninit()
{
    return true;
}

bool GoogleContactsClient::cleanUp()
{
    if (m_identity)
        m_identity->deleteLater();
    if (m_session)
        m_session->deleteLater();
    if (m_accountManager)
        m_accountManager->deleteLater();

    return true;
}

bool GoogleContactsClient::startSync()
{
    m_accessToken = QString();
    m_remoteContacts.clear();

    QString clientId = storedKeyValue("google", "sync", "client_id");
    QString clientSecret = storedKeyValue("google", "sync", "client_secret");
    if (clientId.isEmpty() || clientSecret.isEmpty()) {
        // can't sync if we don't have valid oauth keys.
        qWarning() << Q_FUNC_INFO << "no valid oauth keys, aborting sync";
        return false;
    }

    // find an appropriate Google account
    // Note: this means that only a single account can be synced.
    // XXX TODO: support multiple accounts.
    Accounts::AccountIdList enabledSyncAccts = m_accountManager->accountListEnabled("sync");
    foreach (Accounts::AccountId accId, enabledSyncAccts) {
        Accounts::Account *currAcc = m_accountManager->account(accId);
        if (!currAcc || currAcc->providerName() != QLatin1String("google")) {
            delete currAcc;
            continue;
        }

        // found a valid account.
        Accounts::Service googleSync = m_accountManager->service("google-sync");
        if (!googleSync.isValid()) {
            delete currAcc;
            continue;
        }

        currAcc->selectService(googleSync);
        quint32 identityId = currAcc->credentialsId();
        if (identityId == 0) {
            // no valid credentials.
            delete currAcc;
            continue;
        }

        SignOn::Identity *ident = SignOn::Identity::existingIdentity(identityId);
        if (!ident) {
            // credentials id was incorrect.
            delete currAcc;
            continue;
        }

        Accounts::AccountService *accountSrv = new Accounts::AccountService(currAcc, googleSync);
        if (!accountSrv) {
            // cannot use this account with the sync service for some reason.
            delete currAcc;
            delete ident;
            continue;
        }

        m_session = ident->createSession(accountSrv->authData().method());
        if (!m_session) {
            // unable to create auth session
            delete accountSrv;
            delete currAcc;
            delete ident;
            continue;
        }

        // success.
        connect(this, SIGNAL(syncFinished(Sync::SyncStatus)),
                this, SLOT(handleSyncFinished(Sync::SyncStatus)),
                Qt::UniqueConnection);

        // sign in.
        connect(m_session, SIGNAL(response(SignOn::SessionData)),
                this, SLOT(authResponseReceived(SignOn::SessionData)),
                Qt::UniqueConnection);
        connect(m_session, SIGNAL(error(SignOn::Error)),
                this, SLOT(authError(SignOn::Error)),
                Qt::UniqueConnection);

        QVariantMap sdvmap(accountSrv->authData().parameters());
        sdvmap.insert("UiPolicy", SignOn::NoUserInteractionPolicy);
        sdvmap.insert("ClientId", clientId);
        sdvmap.insert("ClientSecret", clientSecret);
        m_session->process(SignOn::SessionData(sdvmap), accountSrv->authData().mechanism());
        m_identity = ident;

        delete accountSrv;
        delete currAcc;
        break;
    }

    if (!m_session) {
        qWarning() << Q_FUNC_INFO << "no valid account with enabled sync service; aborting sync";
        return false;
    }

    return true;
}

void GoogleContactsClient::abortSync(Sync::SyncStatus status)
{
    Sync::SyncStatus state = Sync::SYNC_ABORTED;
    if (status == Sync::SYNC_ERROR) {
        state = Sync::SYNC_CONNECTION_ERROR;
    }

    if(!this->abort(state)) {
        syncFinished(Sync::SYNC_ABORTED);
    }
}

bool GoogleContactsClient::abort(Sync::SyncStatus status)
{
    Q_UNUSED(status)
    emit syncFinished(Sync::SYNC_ABORTED);
    return true;
}

void GoogleContactsClient::authError(const SignOn::Error &err)
{
    m_identity->deleteLater();
    m_session->deleteLater();
    m_identity = 0;
    m_session = 0;

    qWarning() << Q_FUNC_INFO << err.message();
    emit syncFinished(Sync::SYNC_AUTHENTICATION_FAILURE);
}

Buteo::SyncResults GoogleContactsClient::getSyncResults() const
{
    return m_results;
}

void GoogleContactsClient::connectivityStateChanged(Sync::ConnectivityType aType, bool aState)
{
    Q_UNUSED(aType)
    Q_UNUSED(aState)
}

const QDateTime GoogleContactsClient::lastSyncTime() const
{
    Buteo::ProfileManager pm;
    Buteo::SyncProfile* sp = pm.syncProfile(iProfile.name());
    return sp->lastSuccessfulSyncTime();
}

void GoogleContactsClient::authResponseReceived(const SignOn::SessionData &responseData)
{
    m_identity->deleteLater();
    m_session->deleteLater();
    m_identity = 0;
    m_session = 0;

    m_accessToken = responseData.getProperty("AccessToken").toString();
    if (m_accessToken.isEmpty()) {
        emit syncFinished(Sync::SYNC_AUTHENTICATION_FAILURE);
    } else {
        fetchRemoteContacts();
    }
}

void GoogleContactsClient::fetchRemoteContacts(const int startIndex, const QString &continuationUrl)
{
    if (m_accessToken.isNull() || m_accessToken.isEmpty()) {
        emit syncFinished(Sync::SYNC_ERROR);
        return;
    }

    QUrl requestUrl;
    if (continuationUrl.isEmpty()) {
        requestUrl = QUrl(QString(QLatin1String("https://www.google.com/m8/feeds/contacts/default/full/")));
        QUrlQuery urlQuery;
        //For now, always query all contact information.  In the future, use the sync time as below.  TODO!
        //QDateTime syncTime = lastSyncTime();
        //if (!syncTime.isNull())
        //    urlQuery.addQueryItem("updated-min", syncTime.toString(Qt::ISODate));
        if (startIndex > 1)
            urlQuery.addQueryItem ("start-index", QString::number(startIndex));
        urlQuery.addQueryItem("max-results", QString::number(MAX_CONTACT_ENTRY_RESULTS));
        requestUrl.setQuery(urlQuery);
    } else {
        requestUrl = QUrl(continuationUrl);
    }

    QNetworkRequest req(requestUrl);
    req.setRawHeader("GData-Version", "3.0");
    req.setRawHeader(QString(QLatin1String("Authorization")).toUtf8(),
                     QString(QLatin1String("Bearer ") + m_accessToken).toUtf8());

    QNetworkReply *reply = m_qnam->get(req);
    if (reply) {
        reply->setProperty("startIndex", startIndex);
        connect(reply, SIGNAL(finished()), this, SLOT(networkRequestFinished()));
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkError(QNetworkReply::NetworkError)));
    } else {
        qWarning() << Q_FUNC_INFO << "could not create network request; aborting sync";
    }
}

void GoogleContactsClient::networkRequestFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    QByteArray data = reply->readAll();
    int startIndex = reply->property("startIndex").toInt();
    reply->deleteLater();
    if (data.isEmpty()) {
        return;
    }

    GoogleStream parser(false);
    GoogleAtom *atom = parser.parse(data);

    if (!atom) {
        // parse error - invalid atom.
        emit syncFinished(Sync::SYNC_ERROR);
    }

    QList<QContact> remoteContacts = atom->entryContacts();
    if (remoteContacts.size() > 0) {
        m_remoteContacts.append(remoteContacts);
    }

    if (!atom->nextEntriesUrl().isEmpty()) {
        // request more if they exist.
        startIndex += MAX_CONTACT_ENTRY_RESULTS;
        fetchRemoteContacts(startIndex, atom->nextEntriesUrl());
        m_syncStatus = Sync::SYNC_PROGRESS;
    } else {
        // we're finished - we should attempt to update our local cache.
        int addedCount = 0, modifiedCount = 0, removedCount = 0;
        bool success = storeToLocal(m_remoteContacts, &addedCount, &modifiedCount, &removedCount);
        m_results.setMajorCode(success
                ? Buteo::SyncResults::SYNC_RESULT_SUCCESS
                : Buteo::SyncResults::SYNC_RESULT_FAILED);
        m_results.setTargetId(iProfile.name());
        m_syncStatus = success ? Sync::SYNC_DONE : Sync::SYNC_ERROR;
        qWarning() << Q_FUNC_INFO
                   << QString(QLatin1String("Finished with result: %1: a: %2 m: %3 r: %4"))
                      .arg(success ? "SUCCESS" : "ERROR").arg(addedCount).arg(modifiedCount).arg(removedCount);

        // XXX TODO: set the added/modified/removed counts in the Buteo results struct.
        // Of course, at the moment we don't detect real modifications ... TODO.
    }

    delete atom;

    if (m_syncStatus == Sync::SYNC_DONE || m_syncStatus == Sync::SYNC_ERROR) {
        emit syncFinished(m_syncStatus);
    }
}

void GoogleContactsClient::networkError(QNetworkReply::NetworkError err)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    int errorCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (errorCode <= 0) {
        errorCode = static_cast<int>(err);
    }
    reply->deleteLater();
    switch (errorCode) {
        case 400:
            m_syncStatus = Sync::SYNC_ERROR;
            break;
        case 401:
            m_syncStatus = Sync::SYNC_AUTHENTICATION_FAILURE;
            break;
        case 403:
        case 408:
            m_syncStatus = Sync::SYNC_ERROR;
            break;
        case 500:
        case 503:
        case 504:
            // these are actually server-side failures.
            m_syncStatus = Sync::SYNC_ERROR;
            break;
        default:
            // generic network error
            m_syncStatus = Sync::SYNC_ERROR;
            break;
    }

    emit syncFinished(m_syncStatus);
}

bool GoogleContactsClient::storeToLocal(const QList<QContact> &remoteContacts, int *addedCount, int *modifiedCount, int *removedCount)
{
    // steps:
    // 1) load current data from backend
    // 2) determine delta (add/mod/rem)
    // 3) apply delta

    QContactDetailFilter syncTargetFilter;
    syncTargetFilter.setDetailType(QContactDetail::TypeSyncTarget, QContactSyncTarget::FieldSyncTarget);
    syncTargetFilter.setValue(QLatin1String("google"));
    QContactFetchHint noRelationships;
    noRelationships.setOptimizationHints(QContactFetchHint::NoRelationships);

    QContactManager mgr(QLatin1String("org.nemomobile.contacts.sqlite"));
    QList<QContact> localContacts = mgr.contacts(syncTargetFilter, QList<QContactSortOrder>(), noRelationships);
    QList<QContact> remoteToSave;
    QList<QContactId> localToRemove;
    QList<QContactId> foundLocal;

    // we always use the remote server's data in conflicts
    for (int i = 0; i < remoteContacts.size(); ++i) {
        QContact rc = remoteContacts[i];
        QString guid = rc.detail<QContactGuid>().guid();
        if (guid.isEmpty()) {
            qWarning() << Q_FUNC_INFO << "Error: cannot store remote google contact with no guid!";
            continue;
        }

        bool foundLocalToModify = false;
        for (int j = 0; j < localContacts.size(); ++j) {
            const QContact &lc = localContacts[j];
            if (lc.detail<QContactGuid>().guid() == guid) {
                foundLocalToModify = true;
                rc.setId(lc.id());
                foundLocal.append(lc.id());
                break;
            }
        }

        if (foundLocalToModify) {
            *modifiedCount += 1;
        } else {
            *addedCount += 1;
        }

        remoteToSave.append(rc);
    }

    // any local contacts which exist without a remote counterpart
    // are "stale" and should be removed.
    foreach (const QContact &lc, localContacts) {
        if (!foundLocal.contains(lc.id())) {
            localToRemove.append(lc.id());
            *removedCount += 1;
        }
    }

    // now write the changes to the database.
    bool success = true;
    if (remoteToSave.size()) {
        success = mgr.saveContacts(&remoteToSave);
        if (!success) {
            qWarning() << Q_FUNC_INFO << "Failed to save contacts:" << mgr.error();
        }
    }
    if (localToRemove.size()) {
        success = mgr.removeContacts(localToRemove);
        if (!success) {
            qWarning() << Q_FUNC_INFO << "Failed to remove stale local contacts:" << mgr.error();
        }
    }

    return success;
}

void GoogleContactsClient::handleSyncFinished(Sync::SyncStatus state)
{
    switch(state) {
        case Sync::SYNC_ERROR:
        case Sync::SYNC_AUTHENTICATION_FAILURE:
        case Sync::SYNC_DATABASE_FAILURE:
        case Sync::SYNC_CONNECTION_ERROR:
        case Sync::SYNC_NOTPOSSIBLE: {
            emit error(getProfileName(), "", state);
            break;
        }
        case Sync::SYNC_ABORTED:
        case Sync::SYNC_DONE: {
            emit success(getProfileName(), QString::number(state));
            break;
        }
        case Sync::SYNC_QUEUED:
        case Sync::SYNC_STARTED:
        case Sync::SYNC_PROGRESS:
        default: {
            emit error(getProfileName(), "", state);
            break;
        }
    }
}
