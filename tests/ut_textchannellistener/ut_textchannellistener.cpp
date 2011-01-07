/******************************************************************************
**
** This file is part of commhistory-daemon.
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Alexander Shalamov <alexander.shalamov@nokia.com>
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of the GNU Lesser General Public License version 2.1 as
** published by the Free Software Foundation.
**
** This library is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
** or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
** License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this library; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
**
******************************************************************************/

// INCLUDES
#include "ut_textchannellistener.h"

// Qt includes
#include <QDebug>
#include <QTest>
#include <QTime>
#include <QSignalSpy>
#include <QUuid>

#include "TelepathyQt4/types.h"
#include "TelepathyQt4/account.h"
#include "TelepathyQt4/text-channel.h"
#include "TelepathyQt4/message.h"
#include "TelepathyQt4/connection.h"
#include "TelepathyQt4/contact-manager.h"

#include "RTComTelepathyQt4/cli-connection.h" // stored messages if

#include <CommHistory/GroupModel>
#include <CommHistory/SingleEventModel>

#include "textchannellistener.h"
#include "notificationmanager.h"

// constants
#define IM_USERNAME QLatin1String("dut@localhost")
#define SENT_MESSAGE QLatin1String("Hello, how is life mon?")
#define RECEIVED_MESSAGE QLatin1String("Good, good")
#define IM_ACCOUNT_PATH QLatin1String("/org/freedesktop/Telepathy/Account/gabble/jabber/dut_40localhost0")
#define SMS_ACCOUNT_PATH QLatin1String("/org/freedesktop/Telepathy/Account/ring/tel/ring")
#define SMS_NUMBER QLatin1String("+358987654321")
#define IM_CHANNEL_PATH QLatin1String("/org/freedesktop/Telepathy/Account/gabble/jabber/dut_40localhost0")
#define SMS_CHANNEL_PATH QLatin1String("/org/freedesktop/Telepathy/Connection/ring/tel/ring/text0")
#define TARGET_HANDLE 1

using namespace RTComLogger;

namespace {
    bool waitSignal(QSignalSpy &spy, int msec)
    {
        QTime timer;
        timer.start();
        while (timer.elapsed() < msec && spy.isEmpty())
            QCoreApplication::processEvents();

        return !spy.isEmpty();
    }

    void waitInvocationContext(Tp::MethodInvocationContextPtr<> &ctx, int msec)
    {
        QTime timer;
        timer.start();
        while (timer.elapsed() < msec && !ctx->isFinished())
            QCoreApplication::processEvents();
    }

    template<typename T>
    void addMsgHeader(Tp::Message &msg, int index, const char *key, T value) {
        msg.ut_part(index).insert(QLatin1String(key),
                                  QDBusVariant(value));
    }

    template<const char*>
    void addMsgHeader(Tp::Message &msg, int index, const char *key, const char* value) {
        msg.ut_part(index).insert(QLatin1String(key),
                                  QDBusVariant(QLatin1String(value)));
    }
}

Ut_TextChannelListener::Ut_TextChannelListener()
{
}

Ut_TextChannelListener::~Ut_TextChannelListener()
{
}

/*!
 * This function will be called before the first testfunction is executed.
 */
void Ut_TextChannelListener::initTestCase()
{
    qRegisterMetaType<Tp::PendingOperation*>("Tp::PendingOperation*");
}

/*!
 * This function will be called after the last testfunction was executed.
 */
void Ut_TextChannelListener::cleanupTestCase()
{
}

/*!
 * This function will be called before each testfunction is executed.
 */
void Ut_TextChannelListener::init()
{
}

/*!
 * This unction will be called after every testfunction.
 */
void Ut_TextChannelListener::cleanup()
{
}

CommHistory::Group Ut_TextChannelListener::fetchGroup(const QString &localUid,
                                                      const QString &remoteUid,
                                                      bool wait)
{
    CommHistory::GroupModel model;

    QSignalSpy modelReady(&model, SIGNAL(modelReady()));
    model.getGroups(localUid, remoteUid);

    if (!waitSignal(modelReady, 5000))
        goto end;

    if (!model.rowCount() && wait) {
        QSignalSpy rowInsert(&model, SIGNAL(rowsInserted(const QModelIndex &, int, int)));
        if (!waitSignal(rowInsert, 5000))
            goto end;
    }

    if (model.rowCount() == 1)
        return model.group(model.index(0, 0));

end:
    qWarning() << Q_FUNC_INFO << "Failed to fetch group" << localUid << remoteUid;
    return CommHistory::Group();
}

CommHistory::Event Ut_TextChannelListener::fetchEvent(int eventId)
{
    CommHistory::SingleEventModel model;
    QSignalSpy modelReady(&model, SIGNAL(modelReady()));

    model.getEventByUri(CommHistory::Event::idToUrl(eventId));
    if(!waitSignal(modelReady, 5000))
        goto end;

    if(model.rowCount())
        return model.event(model.index(0, 0));

end:
    qWarning() << Q_FUNC_INFO << "Failed to fetch event" << eventId;
    return CommHistory::Event();
}

void Ut_TextChannelListener::imSending()
{
    QString message = QString(SENT_MESSAGE) + QString(" : ") + QTime::currentTime().toString(Qt::ISODate);

    NotificationManager *nm = NotificationManager::instance();
    QVERIFY(nm);
    nm->postedNotifications.clear();

    Tp::AccountPtr acc(new Tp::Account(IM_ACCOUNT_PATH));
    // setup connection
    Tp::ConnectionPtr conn(new Tp::Connection());
    conn->ut_setIsReady(true);

    //setup channel
    Tp::ChannelPtr ch(new Tp::TextChannel(IM_CHANNEL_PATH));
    ch->ut_setIsRequested(true);
    ch->ut_setTargetHandleType(Tp::HandleTypeContact);
    ch->ut_setTargetHandle(TARGET_HANDLE);
    QVariantMap immProp;
    immProp.insert(TELEPATHY_INTERFACE_CHANNEL ".TargetID", IM_USERNAME);
    ch->ut_setImmutableProperties(immProp);
    ch->ut_setConnection(conn);

    Tp::MethodInvocationContextPtr<> ctx(new Tp::MethodInvocationContext<>());

    TextChannelListener tcl(acc, ch, ctx);
    waitInvocationContext(ctx, 5000);

    QVERIFY(ctx->isFinished());
    QVERIFY(!ctx->isError());

    // send sent message
    uint timestamp = QDateTime::currentDateTime().toTime_t();
    Tp::Message msg(timestamp, (uint)Tp::ChannelTextMessageTypeNormal, message);
    QString token = QUuid::createUuid().toString();
    Tp::TextChannelPtr::dynamicCast(ch)->ut_sendMessage(msg, Tp::MessageSendingFlagReportDelivery, token);

    QSignalSpy eventCommitted(&tcl.eventModel(), SIGNAL(eventsCommitted(const QList<CommHistory::Event>&, bool)));
    QVERIFY(waitSignal(eventCommitted, 5000));

    CommHistory::Group g = fetchGroup(IM_ACCOUNT_PATH, IM_USERNAME, true);

    QVERIFY(g.isValid());
    QCOMPARE(g.localUid(), IM_ACCOUNT_PATH);
    QCOMPARE(g.remoteUids().first(), IM_USERNAME);
    QCOMPARE(g.lastMessageText(), message);
    QCOMPARE(g.lastEventType(), CommHistory::Event::IMEvent);

    CommHistory::Event e = fetchEvent(g.lastEventId());
    QCOMPARE(e.direction(), CommHistory::Event::Outbound);
    QCOMPARE(e.freeText(), message);
    QCOMPARE(e.startTime().toTime_t(), timestamp);
    QCOMPARE(e.endTime().toTime_t(), timestamp);
    QCOMPARE(e.messageToken(), token);

    QCOMPARE(nm->postedNotifications.size(), 0);
}

void Ut_TextChannelListener::receiving_data()
{
    QTest::addColumn<QString>("accountPath");
    QTest::addColumn<QString>("username");
    QTest::addColumn<QString>("messageBase");
    QTest::addColumn<QString>("channelPath");
    QTest::addColumn<bool>("cellular");

    QTest::newRow("IM") << QString(IM_ACCOUNT_PATH)
            << QString(IM_USERNAME)
            << QString(RECEIVED_MESSAGE)
            << QString(IM_CHANNEL_PATH)
            << false;
    QTest::newRow("SMS") << QString(SMS_ACCOUNT_PATH)
            << QString(SMS_NUMBER)
            << QString(RECEIVED_MESSAGE)
            << QString(SMS_CHANNEL_PATH)
            << true;
}

void Ut_TextChannelListener::receiving()
{
    QFETCH(QString, accountPath);
    QFETCH(QString, username);
    QFETCH(QString, messageBase);
    QFETCH(QString, channelPath);
    QFETCH(bool, cellular);

    QString message = messageBase + QString(" : ") + QTime::currentTime().toString(Qt::ISODate);

    NotificationManager *nm = NotificationManager::instance();
    QVERIFY(nm);
    nm->postedNotifications.clear();

    //setup account
    Tp::AccountPtr acc(new Tp::Account(accountPath));
    if (cellular)
        acc->ut_setProtocolName("tel");

    // setup connection
    Tp::ConnectionPtr conn(new Tp::Connection());
    conn->ut_setIsReady(true);
    if (cellular)
        conn->ut_setInterfaces(QStringList() << RTComTp::Client::ConnectionInterfaceStoredMessagesInterface::staticInterfaceName());

    //setup channel
    Tp::ChannelPtr ch(new Tp::TextChannel(channelPath));
    ch->ut_setIsRequested(false);
    ch->ut_setTargetHandleType(Tp::HandleTypeContact);
    ch->ut_setTargetHandle(TARGET_HANDLE);
    QVariantMap immProp;
    immProp.insert(TELEPATHY_INTERFACE_CHANNEL ".TargetID", username);
    ch->ut_setImmutableProperties(immProp);
    ch->ut_setConnection(conn);

    Tp::MethodInvocationContextPtr<> ctx(new Tp::MethodInvocationContext<>());

    TextChannelListener tcl(acc, ch, ctx);
    waitInvocationContext(ctx, 5000);

    QVERIFY(ctx->isFinished());
    QVERIFY(!ctx->isError());

    // send received message
    Tp::ReceivedMessage msg(Tp::MessagePartList() << Tp::MessagePart() << Tp::MessagePart());

    uint timestamp = QDateTime::currentDateTime().toTime_t();
    addMsgHeader(msg, 0, "received", timestamp);
    addMsgHeader(msg, 0, "message-type", (uint)Tp::ChannelTextMessageTypeNormal);
    QString token = QUuid::createUuid().toString();
    addMsgHeader(msg, 0, "message-token", token);

    addMsgHeader(msg, 1,"content-type", "text/plain");
    addMsgHeader(msg, 1,"content", message);
    // set sender contact
    Tp::ContactPtr sender(new Tp::Contact());
    sender->ut_setHandle(22);
    sender->ut_setId(username);
    msg.ut_setSender(sender);

    Tp::TextChannelPtr::dynamicCast(ch)->ut_receiveMessage(msg);

    QSignalSpy eventCommitted(&tcl.eventModel(), SIGNAL(eventsCommitted(const QList<CommHistory::Event>&, bool)));
    QVERIFY(waitSignal(eventCommitted, 5000));

    CommHistory::Group g = fetchGroup(accountPath, username, true);

    QVERIFY(g.isValid());
    QCOMPARE(g.localUid(), accountPath);
    QCOMPARE(g.remoteUids().first(), username);
    QCOMPARE(g.lastMessageText(), message);
    if (cellular)
        QCOMPARE(g.lastEventType(), CommHistory::Event::SMSEvent);
    else
        QCOMPARE(g.lastEventType(), CommHistory::Event::IMEvent);

    CommHistory::Event e = fetchEvent(g.lastEventId());
    QCOMPARE(e.id(), g.lastEventId());
    QCOMPARE(e.direction(), CommHistory::Event::Inbound);
    QCOMPARE(e.freeText(), message);
    QCOMPARE(e.startTime().toTime_t(), timestamp);
    QCOMPARE(e.endTime().toTime_t(), timestamp);
    QCOMPARE(e.messageToken(), token);

    QCOMPARE(nm->postedNotifications.size(), 1);
    QCOMPARE(nm->postedNotifications.first().event.freeText(), message);
    QCOMPARE(nm->postedNotifications.first().channelTargetId, username);
    QCOMPARE(nm->postedNotifications.first().chatType, CommHistory::Group::ChatTypeP2P);

    if (cellular) {
        RTComTp::Client::ConnectionInterfaceStoredMessagesInterface* storedMessages =
                conn->interface<RTComTp::Client::ConnectionInterfaceStoredMessagesInterface>();
        QVERIFY(storedMessages);
        QStringList sm = storedMessages->ut_getExpungedMessages();
        QVERIFY(sm.contains(token));
    }
}

void Ut_TextChannelListener::smsSending_data()
{
    QTest::addColumn<bool>("finalStatus");

    QTest::newRow("Delivered") << true;
    QTest::newRow("Failed") << true;
}

void Ut_TextChannelListener::smsSending()
{
    QFETCH(bool, finalStatus);

    QString message = QString(SENT_MESSAGE) + QString(" : ") + QTime::currentTime().toString(Qt::ISODate);

    NotificationManager *nm = NotificationManager::instance();
    QVERIFY(nm);
    nm->postedNotifications.clear();

    Tp::AccountPtr acc(new Tp::Account(SMS_ACCOUNT_PATH));
    acc->ut_setProtocolName("tel");
    // setup connection
    Tp::ConnectionPtr conn(new Tp::Connection());
    conn->ut_setIsReady(true);
    conn->ut_setInterfaces(QStringList() << RTComTp::Client::ConnectionInterfaceStoredMessagesInterface::staticInterfaceName());

    //setup channel
    Tp::ChannelPtr ch(new Tp::TextChannel(SMS_CHANNEL_PATH));
    ch->ut_setIsRequested(true);
    ch->ut_setTargetHandleType(Tp::HandleTypeContact);
    ch->ut_setTargetHandle(TARGET_HANDLE);
    QVariantMap immProp;
    immProp.insert(TELEPATHY_INTERFACE_CHANNEL ".TargetID", SMS_NUMBER);
    ch->ut_setImmutableProperties(immProp);
    ch->ut_setConnection(conn);

    Tp::MethodInvocationContextPtr<> ctx(new Tp::MethodInvocationContext<>());

    TextChannelListener tcl(acc, ch, ctx);
    waitInvocationContext(ctx, 5000);

    QVERIFY(ctx->isFinished());
    QVERIFY(!ctx->isError());

    // send sent message
    uint timestamp = QDateTime::currentDateTime().toTime_t();
    Tp::Message msg(timestamp, (uint)Tp::ChannelTextMessageTypeNormal, message);
    QString token = QUuid::createUuid().toString();
    Tp::TextChannelPtr::dynamicCast(ch)->ut_sendMessage(msg, Tp::MessageSendingFlagReportDelivery, token);

    QSignalSpy eventCommitted(&tcl.eventModel(), SIGNAL(eventsCommitted(const QList<CommHistory::Event>&, bool)));
    QVERIFY(waitSignal(eventCommitted, 5000));

    CommHistory::Group g = fetchGroup(SMS_ACCOUNT_PATH, SMS_NUMBER, true);

    QVERIFY(g.isValid());
    QCOMPARE(g.localUid(), SMS_ACCOUNT_PATH);
    QCOMPARE(g.remoteUids().first(), SMS_NUMBER);
    QCOMPARE(g.lastMessageText(), message);
    QCOMPARE(g.lastEventType(), CommHistory::Event::SMSEvent);
    QVERIFY(g.lastEventStatus() == CommHistory::Event::SendingStatus
            || g.lastEventStatus() == CommHistory::Event::UnknownStatus);

    CommHistory::Event e = fetchEvent(g.lastEventId());
    QCOMPARE(e.direction(), CommHistory::Event::Outbound);
    QCOMPARE(e.freeText(), message);
    QCOMPARE(e.startTime().toTime_t(), timestamp);
    QCOMPARE(e.endTime().toTime_t(), timestamp);
    QCOMPARE(e.messageToken(), token);
    QVERIFY(e.status() == CommHistory::Event::SendingStatus
            || e.status() == CommHistory::Event::UnknownStatus);

    QCOMPARE(nm->postedNotifications.size(), 0);

    // test delivery report handling
    // accepted
    Tp::ReceivedMessage accepted(Tp::MessagePartList() << Tp::MessagePart());

    uint timestampAccepted = QDateTime::currentDateTime().toTime_t();
    addMsgHeader(accepted, 0, "received", timestampAccepted);
    addMsgHeader(accepted, 0, "message-sent", timestampAccepted);
    addMsgHeader(accepted, 0, "message-type", (uint)Tp::ChannelTextMessageTypeDeliveryReport);
    addMsgHeader(accepted, 0, "delivery-token", token);
    addMsgHeader(accepted, 0, "delivery-status", (uint)Tp::DeliveryStatusAccepted);
    QString acceptedToken = QUuid::createUuid().toString();
    addMsgHeader(accepted, 0, "message-token", acceptedToken);

    Tp::TextChannelPtr::dynamicCast(ch)->ut_receiveMessage(accepted);

    eventCommitted.clear();
    QVERIFY(waitSignal(eventCommitted, 5000));

    g = fetchGroup(SMS_ACCOUNT_PATH, SMS_NUMBER, true);

    QVERIFY(g.isValid());
    QCOMPARE(g.lastMessageText(), message);
    QCOMPARE(g.lastEventType(), CommHistory::Event::SMSEvent);
    QCOMPARE(g.lastEventStatus(), CommHistory::Event::SentStatus);

    // delivered
    Tp::ReceivedMessage delivered(Tp::MessagePartList() << Tp::MessagePart());

    uint timestampDelivered = QDateTime::currentDateTime().toTime_t();
    addMsgHeader(delivered, 0, "received", timestampDelivered);
    addMsgHeader(delivered, 0, "message-sent", timestampDelivered);
    addMsgHeader(delivered, 0, "message-type", (uint)Tp::ChannelTextMessageTypeDeliveryReport);
    addMsgHeader(delivered, 0, "delivery-token", token);
    QString deliveredToken = QUuid::createUuid().toString();
    addMsgHeader(delivered, 0, "message-token", deliveredToken);


    if (finalStatus)
        addMsgHeader(delivered, 0, "delivery-status", (uint)Tp::DeliveryStatusDelivered);
    else
        addMsgHeader(delivered, 0, "delivery-status", (uint)Tp::DeliveryStatusPermanentlyFailed);

    Tp::TextChannelPtr::dynamicCast(ch)->ut_receiveMessage(delivered);

    eventCommitted.clear();
    QVERIFY(waitSignal(eventCommitted, 5000));

    g = fetchGroup(SMS_ACCOUNT_PATH, SMS_NUMBER, true);

    QVERIFY(g.isValid());
    QCOMPARE(g.lastMessageText(), message);
    QCOMPARE(g.lastEventType(), CommHistory::Event::SMSEvent);
    if (finalStatus)
        QCOMPARE(g.lastEventStatus(), CommHistory::Event::DeliveredStatus);
    else
        QCOMPARE(g.lastEventStatus(), CommHistory::Event::FailedStatus);

    e = fetchEvent(g.lastEventId());
    QCOMPARE(e.endTime().toTime_t(), timestampDelivered);

    RTComTp::Client::ConnectionInterfaceStoredMessagesInterface* storedMessages =
            conn->interface<RTComTp::Client::ConnectionInterfaceStoredMessagesInterface>();
    QVERIFY(storedMessages);
    QStringList sm = storedMessages->ut_getExpungedMessages();
    QVERIFY(sm.contains(acceptedToken));
    QVERIFY(sm.contains(deliveredToken));
    QVERIFY(!sm.contains(token));
}

QTEST_MAIN(Ut_TextChannelListener)