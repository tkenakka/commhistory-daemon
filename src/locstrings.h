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

#ifndef LOCSTRINGS_H
#define LOCSTRINGS_H

#include <MLocale>

//workaround for using %L1 instead of %Ln
#define CHD_PL_TR(LID, NUM) ((NUM) == 1 ? qtTrId(LID, NUM) : qtTrId(LID, NUM).arg(NUM))

//% "%L1 new message(s)"
#define txt_qtn_msg_notification_new_message(NUM) CHD_PL_TR("qtn_msg_notification_new_message", NUM)
//% "Contact card of %1"
#define txt_qtn_msg_notification_new_vcard qtTrId("qtn_msg_received_contact_card")
//% "%L1 missed call(s)"
#define txt_qtn_call_missed(NUM) qtTrId("qtn_call_missed", NUM)
//% "%L1 voicemail(s)"
#define txt_qtn_call_voicemail_notification(NUM) qtTrId("qtn_call_voicemail_notification", NUM)
//% "Private number"
#define txt_qtn_call_type_private qtTrId("qtn_call_type_private")
//% "Voicemail"
#define txt_qtn_call_type_voicemail qtTrId("qtn_call_type_voicemail")
//% "Multimedia message was delivered to %1"
#define txt_qtn_msg_notification_delivered(STR) qtTrId("qtn_mms_info_delivered").arg(STR)
//% "Multimedia message was read by %1"
#define txt_qtn_msg_notification_read(STR) qtTrId("qtn_mms_info_msg_read").arg(STR)
//% "Multimedia message was deleted without reading by %1"
#define txt_qtn_msg_notification_deleted(STR) qtTrId("qtn_mms_info_delete_wo_reading").arg(STR)
//% "%1 has joined"
#define txt_qtn_msg_group_chat_remote_joined(STR) qtTrId("qtn_msg_group_chat_remote_joined").arg(STR)
//% "%1 has left"
#define txt_qtn_msg_group_chat_remote_left(STR) qtTrId("qtn_msg_group_chat_remote_left").arg(STR)
//% "%1 changed room topic to %2"
#define txt_qtn_msg_group_chat_topic_changed(STR1,STR2) qtTrId("qtn_msg_group_chat_topic_changed").arg(STR1).arg(STR2)

//% "SMS sending failed to %1"
#define txt_qtn_msg_error_sms_sending_failed(STR) qtTrId("qtn_msg_error_sms_sending_failed").arg(STR)
//% "SMS sending failed. No message center number found in the sim card."
#define txt_qtn_msg_error_missing_smsc qtTrId("qtn_msg_error_missing_smsc")
//% "SMS sendng failed. Sending not allowed to this recipient: %1"
#define txt_qtn_msg_error_fixed_dialing_active(STR) qtTrId("qtn_msg_error_fixed_dialing_active").arg(STR)

//% "Contact is offline. Unable to guarantee that the message will be delivered."
#define txt_qtn_msg_general_supports_offline qtTrId("qtn_msg_general_supports_offline")
//% "Unable to send message. Contact is offline."
#define txt_qtn_msg_general_does_not_support_offline qtTrId("qtn_msg_general_does_not_support_offline")

//% "Authorization request %1"
#define txt_qtn_pers_authorization_req(STR) qtTrId("qtn_pers_authorization_req").arg(STR)

#endif // LOCSTRINGS_H