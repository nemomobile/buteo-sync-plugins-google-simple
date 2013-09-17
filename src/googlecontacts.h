/*
 * This file is part of buteo-gcontact-plugin package
 *
 * Copyright (C) 2013 Jolla Ltd. and/or its subsidiary(-ies).
 *
 * Contributors: Sateesh Kavuri <sateesh.kavuri@gmail.com>
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

#ifndef GOOGLECONTACTSCLIENT_H
#define GOOGLECONTACTSCLIENT_H

#include "googlecontacts_global.h"
#include <ClientPlugin.h>

#include <QNetworkReply>
#include <QContact>
#include <QList>
#include <QPair>

#include <SyncResults.h>
#include <SyncCommonDefs.h>

#include <Accounts/Manager>
#include <SignOn/Identity>
#include <SignOn/AuthSession>
#include <SignOn/SessionData>

QTCONTACTS_USE_NAMESPACE

class QNetworkAccessManager;
class BUTEOGCONTACTPLUGINSHARED_EXPORT GoogleContactsClient : Buteo::ClientPlugin
{
    Q_OBJECT

public:
    GoogleContactsClient(const QString& aPluginName,
                  const Buteo::SyncProfile& aProfile,
                  Buteo::PluginCbInterface *aCbInterface);
    ~GoogleContactsClient();

    // reimp
    bool init();
    bool uninit();
    bool startSync();
    void abortSync(Sync::SyncStatus aStatus = Sync::SYNC_ABORTED);
    Buteo::SyncResults getSyncResults() const;
    bool cleanUp();

public slots:
    virtual void connectivityStateChanged(Sync::ConnectivityType aType, bool aState);

signals:
    void stateChanged(Sync::SyncProgressDetail progress);
    void syncFinished(Sync::SyncStatus);

protected slots:
    void authResponseReceived(const SignOn::SessionData &responseData);
    void authError(const SignOn::Error &err);
    void networkRequestFinished();
    void networkError(QNetworkReply::NetworkError errorCode);
    void handleSyncFinished(Sync::SyncStatus state);

private:
    bool abort(Sync::SyncStatus status);

    void fetchRemoteContacts(const int startIndex = 1, const QString &continuationUrl = QString());
    const QDateTime lastSyncTime() const;
    bool storeToLocal(const QList<QContact> &remoteContacts, int *addedCount, int *modifiedCount, int *removedCount);

    QList<QContact> m_remoteContacts;
    QString m_accessToken;
    Buteo::SyncResults m_results;
    Sync::SyncStatus m_syncStatus;
    SignOn::AuthSession *m_session;
    SignOn::Identity *m_identity;
    Accounts::Manager *m_accountManager;
    QNetworkAccessManager *m_qnam;
};

extern "C" GoogleContactsClient* createPlugin(const QString& aPluginName,
                                       const Buteo::SyncProfile& aProfile,
                                       Buteo::PluginCbInterface *aCbInterface);

extern "C" void destroyPlugin(GoogleContactsClient *aClient);

#endif // GOOGLECONTACTSCLIENT_H
