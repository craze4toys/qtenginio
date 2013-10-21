/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>

#include <Enginio/enginioclient.h>
#include <Enginio/enginioreply.h>
#include <Enginio/enginioidentity.h>

#include "../../common/common.h"

template<class Derived, class Identity>
class IdentityCommonTest
{
    Derived *This() { return static_cast<Derived*>(this); }
protected:
    QString _backendName;
    EnginioTests::EnginioBackendManager _backendManager;
    QByteArray _backendId;
    QByteArray _backendSecret;

    void initTestCase(const QString &name);
    void cleanupTestCase();
    void error(EnginioReply *reply);

    void identity();
    void identity_changing();
    void identity_invalid();
    void identity_afterLogout();
};

template<class Derived, class Identity>
void IdentityCommonTest<Derived, Identity>::initTestCase(const QString &name)
{
    if (EnginioTests::TESTAPP_URL.isEmpty())
        QFAIL("Needed environment variable ENGINIO_API_URL is not set!");

    _backendName = name + QString::number(QDateTime::currentMSecsSinceEpoch());
    QVERIFY(_backendManager.createBackend(_backendName));

    QJsonObject apiKeys = _backendManager.backendApiKeys(_backendName, EnginioTests::TESTAPP_ENV);
    _backendId = apiKeys["backendId"].toString().toUtf8();
    _backendSecret = apiKeys["backendSecret"].toString().toUtf8();

    QVERIFY(!_backendId.isEmpty());
    QVERIFY(!_backendSecret.isEmpty());

    EnginioTests::prepareTestUsersAndUserGroups(_backendId, _backendSecret);
}

template<class Derived, class Identity>
void IdentityCommonTest<Derived, Identity>::cleanupTestCase()
{
    QVERIFY(_backendManager.removeBackend(_backendName));
}

template<class Derived, class Identity>
void IdentityCommonTest<Derived, Identity>::error(EnginioReply *reply)
{
    qDebug() << "\n\n### ERROR";
    qDebug() << reply->errorString();
    reply->dumpDebugInfo();
    qDebug() << "\n###\n";
}

template<class Derived, class Identity>
void IdentityCommonTest<Derived, Identity>::identity()
{
    {
        EnginioClient client;
        QObject::connect(&client, SIGNAL(error(EnginioReply *)), This(), SLOT(error(EnginioReply *)));
        Identity identity;
        QSignalSpy spy(&client, SIGNAL(sessionAuthenticated(EnginioReply*)));
        QSignalSpy spyAuthError(&client, SIGNAL(sessionAuthenticationError(EnginioReply*)));
        QSignalSpy spyError(&client, SIGNAL(error(EnginioReply*)));

        identity.setUser("logintest");
        identity.setPassword("logintest");


        client.setBackendId(_backendId);
        client.setBackendSecret(_backendSecret);
        client.setServiceUrl(EnginioTests::TESTAPP_URL);
        client.setIdentity(&identity);

        QTRY_COMPARE(spy.count(), 1);
        QCOMPARE(spyError.count(), 0);
        QCOMPARE(spyAuthError.count(), 0);
        QCOMPARE(client.authenticationState(), EnginioClient::Authenticated);

        QJsonObject token = spy[0][0].value<EnginioReply*>()->data();
        QVERIFY(token.contains("sessionToken"));
        QVERIFY(token.contains("user"));
        QVERIFY(token.contains("usergroups"));
    }
    {
        // Different initialization order
        EnginioClient client;
        QObject::connect(&client, SIGNAL(error(EnginioReply *)), This(), SLOT(error(EnginioReply *)));
        Identity identity;
        QSignalSpy spy(&client, SIGNAL(sessionAuthenticated(EnginioReply*)));
        QSignalSpy spyAuthError(&client, SIGNAL(sessionAuthenticationError(EnginioReply*)));
        QSignalSpy spyError(&client, SIGNAL(error(EnginioReply*)));

        identity.setUser("logintest");
        identity.setPassword("logintest");

        client.setIdentity(&identity);
        client.setServiceUrl(EnginioTests::TESTAPP_URL);
        client.setBackendId(_backendId);
        client.setBackendSecret(_backendSecret);

        QTRY_COMPARE(spy.count(), 1);
        QCOMPARE(spyError.count(), 0);
        QCOMPARE(spyAuthError.count(), 0);
        QCOMPARE(client.authenticationState(), EnginioClient::Authenticated);
    }
    {
        // login / logout
        EnginioClient client;
        QObject::connect(&client, SIGNAL(error(EnginioReply *)), This(), SLOT(error(EnginioReply *)));
        Identity identity;
        QSignalSpy spyAuthError(&client, SIGNAL(sessionAuthenticationError(EnginioReply*)));
        QSignalSpy spyError(&client, SIGNAL(error(EnginioReply*)));

        identity.setUser("logintest");
        identity.setPassword("logintest");
        client.setIdentity(&identity);
        client.setServiceUrl(EnginioTests::TESTAPP_URL);
        client.setBackendId(_backendId);
        client.setBackendSecret(_backendSecret);

        QTRY_COMPARE(client.authenticationState(), EnginioClient::Authenticated);
        QCOMPARE(spyError.count(), 0);

        client.setIdentity(0);
        QTRY_COMPARE(client.authenticationState(), EnginioClient::NotAuthenticated);
        QCOMPARE(spyError.count(), 0);

        client.setIdentity(&identity);
        QTRY_COMPARE(client.authenticationState(), EnginioClient::Authenticated);
        QCOMPARE(spyError.count(), 0);
        QCOMPARE(spyAuthError.count(), 0);
    }
    {
        // change backend id
        EnginioClient client;
        QObject::connect(&client, SIGNAL(error(EnginioReply *)), This(), SLOT(error(EnginioReply *)));
        Identity identity;
        QSignalSpy spyAuthError(&client, SIGNAL(sessionAuthenticationError(EnginioReply*)));
        QSignalSpy spyError(&client, SIGNAL(error(EnginioReply*)));

        identity.setUser("logintest");
        identity.setPassword("logintest");
        client.setIdentity(&identity);
        client.setServiceUrl(EnginioTests::TESTAPP_URL);
        client.setBackendId(_backendId);
        client.setBackendSecret(_backendSecret);

        QTRY_COMPARE(client.authenticationState(), EnginioClient::Authenticated);
        QCOMPARE(spyError.count(), 0);

        client.setBackendId(QByteArray());
        client.setBackendId(_backendId);
        QTRY_COMPARE(client.authenticationState(), EnginioClient::Authenticated);
        QCOMPARE(spyError.count(), 0);
        QCOMPARE(spyAuthError.count(), 0);
    }
    {
        // fast identity change before initialization
        EnginioClient client;
        QObject::connect(&client, SIGNAL(error(EnginioReply *)), This(), SLOT(error(EnginioReply *)));
        client.setServiceUrl(EnginioTests::TESTAPP_URL);
        client.setBackendSecret(_backendSecret);

        QSignalSpy spyAuthError(&client, SIGNAL(sessionAuthenticationError(EnginioReply*)));
        QSignalSpy spyError(&client, SIGNAL(error(EnginioReply*)));

        Identity identity1;
        Identity identity2;
        Identity identity3;

        identity1.setUser("logintest");
        identity1.setPassword("logintest");
        identity2.setUser("logintest2");
        identity2.setPassword("logintest2");
        identity3.setUser("logintest3");
        identity3.setPassword("logintest3");

        QCOMPARE(spyError.count(), 0);
        QCOMPARE(client.authenticationState(), EnginioClient::NotAuthenticated);

        for (uint i = 0; i < 4; ++i) {
            client.setIdentity(&identity1);
            client.setIdentity(&identity2);
            client.setIdentity(&identity3);
        }

        QCOMPARE(spyError.count(), 0);
        QCOMPARE(client.authenticationState(), EnginioClient::NotAuthenticated);

        client.setBackendId(_backendId); // trigger authentication process

        QCOMPARE(client.authenticationState(), EnginioClient::Authenticating);
        QTRY_COMPARE(client.authenticationState(), EnginioClient::Authenticated);

        QCOMPARE(spyError.count(), 0);
        QCOMPARE(spyAuthError.count(), 0);
    }
    {
        // check if EnginoClient is properly detached from identity in destructor.
        EnginioClient *client = new EnginioClient;
        client->setServiceUrl(EnginioTests::TESTAPP_URL);
        client->setBackendId(_backendId);
        client->setBackendSecret(_backendSecret);

        Identity identity;
        client->setIdentity(&identity);

        delete client;

        identity.setPassword("blah");
        identity.setUser("blah");
        // we should not crash
    }
    {
        // check if EnginoClient is properly detached from identity destructor.
        EnginioClient client;
        QObject::connect(&client, SIGNAL(error(EnginioReply *)), This(), SLOT(error(EnginioReply *)));
        client.setServiceUrl(EnginioTests::TESTAPP_URL);
        client.setBackendSecret(_backendSecret);
        client.setBackendId(_backendId);
        {
            Identity identity;
            client.setIdentity(&identity);
        }
        QVERIFY(!client.identity());
    }
}

template<class Derived, class Identity>
void IdentityCommonTest<Derived, Identity>::identity_changing()
{
    {   // check if login is triggered on passowrd change
        EnginioClient client;
        client.setServiceUrl(EnginioTests::TESTAPP_URL);
        client.setBackendId(_backendId);
        client.setBackendSecret(_backendSecret);

        Identity identity;
        identity.setUser("logintest");
        client.setIdentity(&identity);
        QTRY_COMPARE(client.authenticationState(), EnginioClient::AuthenticationFailure);

        identity.setPassword("logintest");
        QCOMPARE(client.authenticationState(), EnginioClient::Authenticating);

        QTRY_COMPARE(client.authenticationState(), EnginioClient::Authenticated);
    }
    {   // check if login is triggered on user change
        EnginioClient client;
        client.setServiceUrl(EnginioTests::TESTAPP_URL);
        client.setBackendId(_backendId);
        client.setBackendSecret(_backendSecret);

        Identity identity;
        identity.setUser("logintest");
        identity.setPassword("logintest");
        client.setIdentity(&identity);

        QTRY_COMPARE(client.authenticationState(), EnginioClient::Authenticated);

        identity.setUser("invalid");
        QCOMPARE(client.authenticationState(), EnginioClient::Authenticating);
        QTRY_COMPARE(client.authenticationState(), EnginioClient::AuthenticationFailure);

        identity.setPassword("invalid");
        QCOMPARE(client.authenticationState(), EnginioClient::Authenticating);
        QTRY_COMPARE(client.authenticationState(), EnginioClient::AuthenticationFailure);
    }
    {   // check races when identity flickers fast.
        EnginioClient client;
        client.setServiceUrl(EnginioTests::TESTAPP_URL);
        client.setBackendId(_backendId);
        client.setBackendSecret(_backendSecret);

        QSignalSpy spy(&client, SIGNAL(sessionAuthenticated(EnginioReply*)));
        Identity identity;
        client.setIdentity(&identity);
        QTRY_COMPARE(client.authenticationState(), EnginioClient::AuthenticationFailure);

        for (int i = 0; i < 8; ++i) {
            identity.setUser("logintest1");
            identity.setPassword("logintest1");
            identity.setUser("logintest2");
            identity.setPassword("logintest2");
        }
        identity.setUser("logintest");
        identity.setPassword("logintest");
        QCOMPARE(client.authenticationState(), EnginioClient::Authenticating);

        QTRY_COMPARE_WITH_TIMEOUT(client.authenticationState(), EnginioClient::Authenticated, 20000);
        QJsonObject data = spy.last().last().value<EnginioReply*>()->data();
        QCOMPARE(data["user"].toObject()["username"].toString(), QString::fromLatin1("logintest"));
    }
}

template<class Derived, class Identity>
void IdentityCommonTest<Derived, Identity>::identity_invalid()
{
    {
        EnginioClient client;
        QObject::connect(&client, SIGNAL(error(EnginioReply *)), This(), SLOT(error(EnginioReply *)));
        Identity identity;
        QSignalSpy spy(&client, SIGNAL(sessionAuthenticated(EnginioReply*)));
        QSignalSpy spyError(&client, SIGNAL(error(EnginioReply*)));
        QSignalSpy spyAuthError(&client, SIGNAL(sessionAuthenticationError(EnginioReply*)));

        identity.setUser("invalidLogin");
        identity.setPassword("invalidPassword");

        client.setBackendId(_backendId);
        client.setBackendSecret(_backendSecret);
        client.setServiceUrl(EnginioTests::TESTAPP_URL);
        client.setIdentity(&identity);

        QTRY_COMPARE(spyAuthError.count(), 1);
        QTRY_COMPARE(spy.count(), 0);
        QCOMPARE(spyError.count(), 0);
        QCOMPARE(client.authenticationState(), EnginioClient::AuthenticationFailure);
    }
    {   // check if an old session is _not_ invalidated on an invalid re-login
        EnginioClient client;
        QObject::connect(&client, SIGNAL(error(EnginioReply *)), This(), SLOT(error(EnginioReply *)));
        Identity identity;
        QSignalSpy spy(&client, SIGNAL(sessionAuthenticated(EnginioReply*)));
        QSignalSpy spyError(&client, SIGNAL(error(EnginioReply*)));
        QSignalSpy spyAuthError(&client, SIGNAL(sessionAuthenticationError(EnginioReply*)));

        identity.setUser("logintest");
        identity.setPassword("logintest");

        client.setBackendId(_backendId);
        client.setBackendSecret(_backendSecret);
        client.setServiceUrl(EnginioTests::TESTAPP_URL);
        client.setIdentity(&identity);

        QTRY_COMPARE(spy.count(), 1);
        QCOMPARE(spyError.count(), 0);
        QTRY_COMPARE(spyAuthError.count(), 0);
        QCOMPARE(client.authenticationState(), EnginioClient::Authenticated);

        const QJsonObject identityReplyData = spy[0][0].value<EnginioReply*>()->data();

        // we are logged-in
        identity.setUser("invalidLogin");
        QTRY_COMPARE(spyAuthError.count(), 1);
        identity.setPassword("invalidPass");
        QTRY_COMPARE(spyAuthError.count(), 2);

        // get back to logged-in state
        identity.setUser("logintest2");
        QTRY_COMPARE(spyAuthError.count(), 3);
        QCOMPARE(spy.count(), 1);
        identity.setPassword("logintest2");
        QTRY_COMPARE(spy.count(), 2);
        QTRY_COMPARE(spyAuthError.count(), 3);

        QVERIFY(spy[1][0].value<EnginioReply*>()->data() != identityReplyData);
        QCOMPARE(client.authenticationState(), EnginioClient::Authenticated);
    }
    {
        // wrong backend id and secret plus invalid password
        EnginioClient client;
        client.setBackendId("deadbeef");
        client.setBackendSecret("deadbeef");
        Identity identity;
        identity.setUser("invalidLogin");
        identity.setPassword("invalidPassword");
        client.setIdentity(&identity);
        QCOMPARE(client.authenticationState(), EnginioClient::Authenticating);
        QTRY_COMPARE(client.authenticationState(), EnginioClient::AuthenticationFailure);
    }
    {
        // missing empty user name and pass
        EnginioClient client;
        client.setBackendId(_backendId);
        client.setBackendSecret(_backendSecret);
        Identity identity;
        client.setIdentity(&identity);
        QCOMPARE(client.authenticationState(), EnginioClient::Authenticating);
        QTRY_COMPARE(client.authenticationState(), EnginioClient::AuthenticationFailure);
    }
}

template<class Derived, class Identity>
void IdentityCommonTest<Derived, Identity>::identity_afterLogout()
{
    qRegisterMetaType<QNetworkReply*>();
    EnginioClient client;

    Identity identity;
    identity.setUser("logintest");
    identity.setPassword("logintest");

    client.setBackendId(_backendId);
    client.setBackendSecret(_backendSecret);
    client.setServiceUrl(EnginioTests::TESTAPP_URL);
    client.setIdentity(&identity);

    QVERIFY(client.networkManager());
    QTRY_COMPARE(client.authenticationState(), EnginioClient::Authenticated);

    // This may be fragile, we need real network reply to catch the header.
    QSignalSpy spy(client.networkManager(), SIGNAL(finished(QNetworkReply*)));
    QByteArray headerName = QByteArrayLiteral("Enginio-Backend-Session");

    // make a connection with a new session token
    QJsonObject obj;
    obj["objectType"] = QString::fromUtf8("objects.todos");
    const EnginioReply* reqId = client.query(obj);
    QVERIFY(reqId);
    QTRY_VERIFY(!reqId->data().isEmpty());
    CHECK_NO_ERROR(reqId);
    QCOMPARE(spy.count(), 1);

    QVERIFY(spy[0][0].value<QNetworkReply*>()->request().hasRawHeader(headerName));

    client.setIdentity(0);
    QTRY_COMPARE(client.authenticationState(), EnginioClient::NotAuthenticated);

    // unsecured data should be still accessible
    reqId = client.query(obj);
    QVERIFY(reqId);
    QTRY_VERIFY(!reqId->data().isEmpty());
    CHECK_NO_ERROR(reqId);
    QCOMPARE(spy.count(), 2);
    QVERIFY(!spy[1][0].value<QNetworkReply*>()->request().hasRawHeader(headerName));
}