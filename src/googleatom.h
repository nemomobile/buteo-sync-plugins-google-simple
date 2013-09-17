/*
 * This file is part of buteo-gcontact-plugin package
 *
 * Copyright (C) 2013 Jolla Ltd. and/or its subsidiary(-ies).
 *
 * Contributors: Sateesh Kavuri <sateesh.kavuri@gmail.com>
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

#ifndef GOOGLEATOM_H
#define GOOGLEATOM_H

#include "googleatom_global.h"

#include <QMetaEnum>
#include <QMap>
#include <QList>
#include <QXmlStreamWriter>

#include <QContact>

QTCONTACTS_USE_NAMESPACE

class GoogleAtom {
public:
    GoogleAtom ();

    typedef enum
    {
        text,
        html,
        xhtml
    } TYPE;

    void setAuthorName(const QString &authorName);
    QString authorName() const;

    void setAuthorEmail(const QString &authorEmail);
    QString authorEmail() const;

    void setId(const QString &id);
    QString id() const;

    void setUpdated(const QString &updated);
    QString updated() const;

    void setCategory (const QString &schema = QLatin1String("http://schemas.google.com/g/2005#kind"),
                      const QString &term = QLatin1String("http://schemas.google.com/contact/2008#contact"));

    void setTitle(const QString &title);
    QString title() const;

    void setContent (const QString &note, const QString &type = QLatin1String("text"));

    void setGenerator(const QString &name = QLatin1String("Contacts"),
                      const QString &version = QLatin1String("1.0"),
                      const QString &uri = QLatin1String("http://sailfish.org"));
    QString generatorName() const;
    QString generatorVersion() const;
    QString generatorUri() const;

    void setTotalResults(int totalResults);
    int totalResults() const;

    void setStartIndex(int startIndex);
    int startIndex() const;

    void setItemsPerPage(int itemsPerPage);
    int itemsPerPage() const;

    void addEntryContact(const QContact &contact);
    QList<QContact> entryContacts() const;

    void setNextEntriesUrl (const QString &nextUrl);
    QString nextEntriesUrl() const;

private:
    QString mAuthorEmail;
    QString mAuthorName;
    QString mCategory;
    QString mCategoryTerm;
    QString mSchema;
    QString mContributor;
    QString mGeneratorName;
    QString mGeneratorVersion;
    QString mGeneratorUri;
    QString mIcon;
    QString mId;
    QString mLink;
    QString mLogo;
    QString mRights;
    QString mSubtitle;
    QString mTitle;
    QString mUpdated;

    int mTotalResults;
    int mStartIndex;
    int mItemsPerPage;

    QList<QContact> mContactList;
    QString mNextEntriesUrl;
};

#endif // GOOGLEATOM_H
