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

#ifndef GOOGLESTREAM_H
#define GOOGLESTREAM_H

#include <QObject>

#include <QXmlStreamReader>
#include <QMap>

#include <QContact>
#include <QContactDetail>

QTCONTACTS_USE_NAMESPACE

class GoogleAtom;
class GoogleStream : public QObject
{
    Q_OBJECT

public:
    explicit GoogleStream(bool response, QObject* parent = 0);
    explicit GoogleStream(const QByteArray &xmlStream, QObject *parent = 0);
    ~GoogleStream();

    GoogleAtom* parse(const QByteArray &xmlBuffer);

signals:
    void parseDone(bool);

private:
    void initAtomFunctionMap();
    void initResponseFunctionMap();
    void initFunctionMap();

    // Atom feed elements handler methods
    void handleAtomUpdated();
    void handleAtomCategory();
    void handleAtomAuthor();
    void handleAtomOpenSearch();
    void handleAtomEntry();
    void handleAtomLink();

    // Following are for the response received from the server
    // incase of failures
    //void handleEntryBatchStatus();
    //void handleEntryBatchOperation();

    // gContact:xxx schema handler methods
    QContactDetail handleEntryId();
    QContactDetail handleEntryContent();
    QContactDetail handleEntryLink();
    QContactDetail handleEntryBirthday();
    QContactDetail handleEntryGender();
    QContactDetail handleEntryHobby();
    QContactDetail handleEntryNickname();
    QContactDetail handleEntryOccupation();
    QContactDetail handleEntryWebsite();
    QContactDetail handleEntryComments();
    QContactDetail handleEntryEmail();
    QContactDetail handleEntryIm();
    QContactDetail handleEntryName();
    QContactDetail handleEntryOrganization();
    QContactDetail handleEntryPhoneNumber();
    QContactDetail handleEntryStructuredPostalAddress();

    QXmlStreamReader *mXml;

    typedef void (GoogleStream::*Handler)();
    typedef QContactDetail (GoogleStream::*DetailHandler)();
    QMap<QString, GoogleStream::Handler> mAtomFunctionMap;
    QMap<QString, GoogleStream::DetailHandler> mContactFunctionMap;

    GoogleAtom *mAtom;
};

#endif // GOOGLESTREAM_H
