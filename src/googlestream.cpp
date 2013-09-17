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

#include "googlestream.h"
#include "googleatom.h"
#include <LogMacros.h>

#include <QContact>
#include <QContactSyncTarget>
#include <QContactName>
#include <QContactAddress>
#include <QContactEmailAddress>
#include <QContactPhoneNumber>
#include <QContactNote>
#include <QContactOrganization>
#include <QContactGender>
#include <QContactUrl>
#include <QContactOnlineAccount>
#include <QContactHobby>
#include <QContactAvatar>
#include <QContactBirthday>
#include <QContactGuid>
#include <QContactNickname>
#include <QContactDisplayLabel>

GoogleStream::GoogleStream(bool response, QObject* parent)
    : QObject(parent)
    , mXml(0)
    , mAtom(0)
{
    FUNCTION_CALL_TRACE;

    if (response == true) {
        initResponseFunctionMap();
    } else {
        initFunctionMap();
    }
}

GoogleStream::GoogleStream(const QByteArray &xmlStream, QObject *parent)
    : QObject(parent)
{
    FUNCTION_CALL_TRACE;

    mXml = new QXmlStreamReader(xmlStream);
    mAtom = new GoogleAtom;

    initFunctionMap();
}

GoogleStream::~GoogleStream()
{
    FUNCTION_CALL_TRACE;
}

void GoogleStream::initAtomFunctionMap()
{
    mAtomFunctionMap.insert("updated", &GoogleStream::handleAtomUpdated);
    mAtomFunctionMap.insert("category", &GoogleStream::handleAtomCategory);
    mAtomFunctionMap.insert("author", &GoogleStream::handleAtomAuthor);
    mAtomFunctionMap.insert("totalResults", &GoogleStream::handleAtomOpenSearch);
    mAtomFunctionMap.insert("startIndex", &GoogleStream::handleAtomOpenSearch);
    mAtomFunctionMap.insert("itemsPerPage", &GoogleStream::handleAtomOpenSearch);
    mAtomFunctionMap.insert("link", &GoogleStream::handleAtomLink);
    mAtomFunctionMap.insert("entry", &GoogleStream::handleAtomEntry);
}

void GoogleStream::initResponseFunctionMap()
{
    initAtomFunctionMap();
    mContactFunctionMap.insert("id", &GoogleStream::handleEntryId);
    //mContactFunctionMap.insert("batch:status", &GoogleStream::handleEntryBatchStatus);
    //mContactFunctionMap.insert("batch:operation", &GoogleStream::handleEntryBatchOperation);
}

void GoogleStream::initFunctionMap()
{
    initAtomFunctionMap();
    mContactFunctionMap.insert("id", &GoogleStream::handleEntryId);
    mContactFunctionMap.insert("content", &GoogleStream::handleEntryContent);
    mContactFunctionMap.insert("link", &GoogleStream::handleEntryLink);
    mContactFunctionMap.insert("gContact:birthday", &GoogleStream::handleEntryBirthday);
    mContactFunctionMap.insert("gcontact::gender", &GoogleStream::handleEntryGender);
    mContactFunctionMap.insert("gContact:hobby", &GoogleStream::handleEntryHobby);
    mContactFunctionMap.insert("gContact:nickname", &GoogleStream::handleEntryNickname);
    mContactFunctionMap.insert("gContact:occupation", &GoogleStream::handleEntryOccupation);
    mContactFunctionMap.insert("gContact:website", &GoogleStream::handleEntryWebsite);
    mContactFunctionMap.insert("gd:comments", &GoogleStream::handleEntryComments);
    mContactFunctionMap.insert("gd:email", &GoogleStream::handleEntryEmail);
    mContactFunctionMap.insert("gd:im", &GoogleStream::handleEntryIm);
    mContactFunctionMap.insert("gd:name", &GoogleStream::handleEntryName);
    mContactFunctionMap.insert("gd:organization", &GoogleStream::handleEntryOrganization);
    mContactFunctionMap.insert("gd:phoneNumber", &GoogleStream::handleEntryPhoneNumber);
    mContactFunctionMap.insert("gd:structuredPostalAddress", &GoogleStream::handleEntryStructuredPostalAddress);
}

GoogleAtom *GoogleStream::parse(const QByteArray &xmlBuffer)
{
    mXml = new QXmlStreamReader(xmlBuffer);
    mAtom = new GoogleAtom;

    Q_CHECK_PTR(mXml);
    Q_CHECK_PTR(mAtom);

    while (!mXml->atEnd() && !mXml->hasError()) {
        if (mXml->readNextStartElement()) {
            Handler handler = mAtomFunctionMap.value(mXml->name().toString());
            if (handler) {
                (*this.*handler)();
            }
        }
    }

    delete mXml;
    return mAtom;
}

void GoogleStream::handleAtomUpdated()
{
    Q_ASSERT(mXml->isStartElement() && mXml->name() == "updated");
    mAtom->setUpdated(mXml->readElementText());
}

void GoogleStream::handleAtomCategory()
{
    Q_ASSERT(mXml->isStartElement() && mXml->name() == "category");

    QXmlStreamAttributes attributes = mXml->attributes();
    QString schema, term;
    if (attributes.hasAttribute("scheme")) {
        schema = attributes.value("scheme").toString();
    } else if (attributes.hasAttribute("term")) {
        term = attributes.value("term").toString();
    }

    mAtom->setCategory(schema, term);
}

void GoogleStream::handleAtomAuthor()
{
    Q_ASSERT(mXml->isStartElement() && mXml->name() == "author");

    while (!(mXml->tokenType() == QXmlStreamReader::EndElement && mXml->name() == "author")) {
        if (mXml->tokenType() == QXmlStreamReader::StartElement) {
            if (mXml->name() == "name") {
                mAtom->setAuthorName(mXml->readElementText());
            } else if (mXml->name() == "email") {
                mAtom->setAuthorEmail(mXml->readElementText());
            }
        }
        mXml->readNextStartElement();
    }
}

void GoogleStream::handleAtomOpenSearch()
{
    Q_ASSERT(mXml->isStartElement() && mXml->prefix() == "openSearch");

    if (mXml->name() == "totalResults") {
        mAtom->setTotalResults(mXml->readElementText().toInt());
    } else if (mXml->name() == "startIndex") {
        mAtom->setStartIndex(mXml->readElementText().toInt());
    } else if (mXml->name() == "itemsPerPage") {
        mAtom->setItemsPerPage(mXml->readElementText().toInt());
    }
}

void GoogleStream::handleAtomLink()
{
    FUNCTION_CALL_TRACE;
    Q_ASSERT(mXml->isStartElement() && mXml->name() == "link");

    if (mXml->attributes().hasAttribute("rel") && (mXml->attributes().value("rel") == "next")) {
        mAtom->setNextEntriesUrl(mXml->attributes().value("href").toString());
    }
}

void GoogleStream::handleAtomEntry()
{
    FUNCTION_CALL_TRACE;
    Q_ASSERT(mXml->isStartElement() && mXml->name() == "entry");

    QContact entryContact;

    bool isInGroup = false;
    while (!((mXml->tokenType() == QXmlStreamReader::EndElement) && (mXml->name() == "entry"))) {
        if (mXml->tokenType() == QXmlStreamReader::StartElement) {
            DetailHandler handler = mContactFunctionMap.value(mXml->qualifiedName().toString());
            if (handler) {
                QContactDetail convertedDetail = (*this.*handler)();
                if (convertedDetail != QContactDetail()) {
                    entryContact.saveDetail(&convertedDetail);
                }
            } else if (mXml->qualifiedName().toString() == QLatin1String("gContact:groupMembershipInfo")) {
                isInGroup = true;
            }
        }
        mXml->readNextStartElement();
    }

    QContactSyncTarget syncTarget;
    syncTarget.setSyncTarget("google");
    entryContact.saveDetail(&syncTarget);

    if (isInGroup) {
        // Only sync the contact if it is in a "real" group
        // as otherwise we get hundreds of "Other Contacts"
        // (random email addresses etc).
        mAtom->addEntryContact(entryContact);
    }
}

QContactDetail GoogleStream::handleEntryId()
{
    QString idUrl = mXml->readElementText();
    QContactGuid guid;
    guid.setGuid(idUrl.remove(0, idUrl.lastIndexOf('/') + 1));
    return guid;
}

QContactDetail GoogleStream::handleEntryContent()
{
    QContactNote note;
    QString content = mXml->readElementText();
    note.setNote(content);
    return note;
}

QContactDetail GoogleStream::handleEntryLink()
{
    Q_ASSERT(mXml->isStartElement() && mXml->name() == "link");

    if (mXml->attributes().hasAttribute("gd:etag")
            && (mXml->attributes().value("rel") == "http://schemas.google.com/contacts/2008/rel#photo")) {
        // the contact entry has a photo
        QContactAvatar avatar;
        avatar.setImageUrl(mXml->attributes().value("href").toString());
        return avatar;
    }

    return QContactDetail();
}

QContactDetail GoogleStream::handleEntryBirthday()
{
    Q_ASSERT(mXml->isStartElement() && mXml->qualifiedName() == "gContact:birthday");

    QContactBirthday birthday;
    birthday.setDate(QDate::fromString(mXml->attributes().value("when").toString(), Qt::ISODate));
    return birthday;
}

QContactDetail GoogleStream::handleEntryGender()
{
    Q_ASSERT(mXml->isStartElement() && mXml->qualifiedName() == "gContact:gender");

    QString genderStr = mXml->attributes().value("value").toString().toLower();
    QContactGender gender;
    if (genderStr.startsWith('m')) {
        gender.setGender(QContactGender::GenderMale);
    } else if (genderStr.startsWith('f')) {
        gender.setGender(QContactGender::GenderFemale);
    } else {
        gender.setGender(QContactGender::GenderUnspecified);
    }

    return gender;
}

QContactDetail GoogleStream::handleEntryHobby()
{
    Q_ASSERT(mXml->isStartElement() && mXml->qualifiedName() == "gContact:hobby");

    QContactHobby hobby;
    hobby.setHobby(mXml->readElementText());
    return hobby;
}

QContactDetail GoogleStream::handleEntryNickname()
{
    Q_ASSERT(mXml->isStartElement() && mXml->qualifiedName() == "gContact:nickname");

    QContactNickname nickname;
    nickname.setNickname(mXml->readElementText());
    return nickname;
}

QContactDetail GoogleStream::handleEntryOccupation()
{
    Q_ASSERT(mXml->isStartElement() && mXml->qualifiedName() == "gContact:occupation");

    QContactOrganization org;
    org.setRole(mXml->readElementText());
    return org;
}

QContactDetail GoogleStream::handleEntryWebsite()
{
    Q_ASSERT(mXml->isStartElement() && mXml->qualifiedName() == "gContact:website");

    QContactUrl url;
    url.setUrl(mXml->attributes().value("href").toString());
    return url;
}

QContactDetail GoogleStream::handleEntryComments()
{
    Q_ASSERT(mXml->isStartElement() && mXml->qualifiedName() == "gd:comments");

    QContactNote note;
    note.setNote(mXml->readElementText());
    return note;
}

QContactDetail GoogleStream::handleEntryEmail()
{
    Q_ASSERT(mXml->isStartElement() && mXml->qualifiedName() == "gd:email");

    QContactEmailAddress email;
    email.setEmailAddress(mXml->attributes().value("address").toString());
    return email;
}

QContactDetail GoogleStream::handleEntryIm()
{
    Q_ASSERT(mXml->isStartElement() && mXml->qualifiedName() == "gd:im");

    QContactOnlineAccount onlineAccount;
    onlineAccount.setAccountUri(mXml->attributes().value("address").toString());
    //if (mXml->attributes().hasAttribute("protocol")) {
    //    QString protocolUrl = mXml->attributes().value("protocol").toString();
    //    onlineAccount.setProtocol(protocolUrl.right(protocolUrl.lastIndexOf("#")));
    //}

    return onlineAccount;
}

QContactDetail GoogleStream::handleEntryName()
{
    Q_ASSERT(mXml->isStartElement() && mXml->qualifiedName() == "gd:name");

    QContactName name;
    while (!(mXml->tokenType() == QXmlStreamReader::EndElement && mXml->qualifiedName() == "gd:name")) {
        if (mXml->tokenType() == QXmlStreamReader::StartElement) {
            if (mXml->qualifiedName() == "gd:givenName") {
                name.setFirstName(mXml->readElementText());
            } else if (mXml->qualifiedName() == "gd:additionalName") {
                name.setMiddleName(mXml->readElementText());
            } else if (mXml->qualifiedName() == "gd:familyName") {
                name.setLastName(mXml->readElementText());
            } else if (mXml->qualifiedName() == "gd:namePrefix") {
                name.setPrefix(mXml->readElementText());
            } else if (mXml->qualifiedName() == "gd:nameSuffix") {
                name.setSuffix(mXml->readElementText());
            }
        }
        mXml->readNextStartElement();
    }

    return name;
}

QContactDetail GoogleStream::handleEntryOrganization()
{
    Q_ASSERT(mXml->isStartElement() && mXml->qualifiedName() == "gd:organization");

    QContactOrganization org;

    while (!(mXml->tokenType() == QXmlStreamReader::EndElement && mXml->qualifiedName() == "gd:organization")) {
        if (mXml->tokenType() == QXmlStreamReader::StartElement) {
            if (mXml->qualifiedName() == "gd:orgDepartment") {
                QStringList dept = org.department();
                dept.append(mXml->readElementText());
                org.setDepartment(dept);
            } else if (mXml->qualifiedName() == "gd:orgJobDescription") {
                org.setRole(mXml->readElementText());
            } else if (mXml->qualifiedName() == "gd:orgName") {
                org.setName(mXml->readElementText());
            } else if (mXml->qualifiedName() == "gd:orgSymbol") {
                org.setLogoUrl(mXml->readElementText());
            } else if (mXml->qualifiedName() == "gd:orgTitle") {
                org.setTitle(mXml->readElementText());
            }
        }
        mXml->readNextStartElement();
    }

    return org;
}

QContactDetail GoogleStream::handleEntryPhoneNumber()
{
    Q_ASSERT(mXml->isStartElement() && mXml->qualifiedName() == "gd:phoneNumber");

    QContactPhoneNumber phone;
    phone.setNumber(mXml->readElementText());
    return phone;
}

QContactDetail GoogleStream::handleEntryStructuredPostalAddress()
{
    Q_ASSERT(mXml->isStartElement() && mXml->qualifiedName() == "gd:structuredPostalAddress");

    QContactAddress address;

    while (!(mXml->tokenType() == QXmlStreamReader::EndElement && mXml->qualifiedName() == "gd:structuredPostalAddress")) {
        if (mXml->tokenType() == QXmlStreamReader::StartElement) {
            if (mXml->qualifiedName() == "gd:street") {
                address.setStreet(mXml->readElementText());
            } else if (mXml->qualifiedName() == "gd:pobox") {
                address.setPostOfficeBox(mXml->readElementText());
            } else if (mXml->qualifiedName() == "gd:neighborhood") {
                address.setLocality(mXml->readElementText());
            } else if (mXml->qualifiedName() == "gd:city") {
                address.setLocality(mXml->readElementText());
            } else if (mXml->qualifiedName() == "gd:region") {
                address.setRegion(mXml->readElementText());
            } else if (mXml->qualifiedName() == "gd:postcode") {
                address.setPostcode(mXml->readElementText());
            } else if (mXml->qualifiedName() == "gd:country") {
                address.setCountry(mXml->readElementText());
            }
        }
        mXml->readNextStartElement();
    }

    return address;
}
