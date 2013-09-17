/*
 * This file is part of buteo-gcontact-plugin package
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

#include "googleatom.h"
#include <LogMacros.h>

GoogleAtom::GoogleAtom()
{
}

void GoogleAtom::setAuthorEmail(const QString &authorEmail)
{
    mAuthorEmail = authorEmail;
}

QString GoogleAtom::authorEmail() const
{
    return mAuthorEmail;
}

void GoogleAtom::setAuthorName(const QString &authorName)
{
    mAuthorName = authorName;
}

QString GoogleAtom::authorName() const
{
    return mAuthorName;
}

void GoogleAtom::setId(const QString &id)
{
    mId = id;
}

QString GoogleAtom::id() const
{
    return mId;
}

void GoogleAtom::setUpdated(const QString &updated)
{
    mUpdated = updated;
}

QString GoogleAtom::updated() const
{
    return mUpdated;
}

void GoogleAtom::setCategory (const QString &schema, const QString &term)
{
    Q_UNUSED(schema)
    Q_UNUSED(term)
}

void GoogleAtom::setTitle(const QString &title)
{
    mTitle = title;
}

QString GoogleAtom::title() const
{
    return mTitle;
}

void GoogleAtom::setGenerator(const QString &name, const QString &version, const QString &uri)
{
    mGeneratorName = name;
    mGeneratorVersion = version;
    mGeneratorUri = uri;
}

void GoogleAtom::setContent (const QString &note, const QString &type)
{
    Q_UNUSED(note)
    Q_UNUSED(type)
}

QString GoogleAtom::generatorName() const
{
    return mGeneratorName;
}

QString GoogleAtom::generatorVersion() const
{
    return mGeneratorVersion;
}

QString GoogleAtom::generatorUri() const
{
    return mGeneratorUri;
}

void GoogleAtom::setTotalResults(int totalResults)
{
    mTotalResults = totalResults;
}

int GoogleAtom::totalResults() const
{
    return mTotalResults;
}

void GoogleAtom::setStartIndex(int startIndex)
{
    mStartIndex = startIndex;
}

int GoogleAtom::startIndex() const
{
    return mStartIndex;
}

void GoogleAtom::setItemsPerPage(int itemsPerPage)
{
    mItemsPerPage = itemsPerPage;
}

int GoogleAtom::itemsPerPage() const
{
    return mItemsPerPage;
}

void GoogleAtom::addEntryContact(const QContact &entryContact)
{
    mContactList.append(entryContact);
}

QList<QContact> GoogleAtom::entryContacts() const
{
    return mContactList;
}

void GoogleAtom::setNextEntriesUrl(const QString &nextUrl)
{
    mNextEntriesUrl = nextUrl;
}

QString GoogleAtom::nextEntriesUrl() const
{
    return mNextEntriesUrl;
}
