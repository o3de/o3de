/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_UTIL_MAILER_H
#define CRYINCLUDE_EDITOR_UTIL_MAILER_H
#pragma once


//////////////////////////////////////////////////////////////////////////
class CMailer
{
public:
    static bool SendMail(const char* _subject,                // E-Mail Subject
        const char* _messageBody,         // Message Text
        const std::vector<const char*>& _recipients,  // All Recipients' Addresses
        const std::vector<const char*>& _attachments, // All File Attachments
        bool bShowDialog);   // Whether to allow editing by user
};

#endif // CRYINCLUDE_EDITOR_UTIL_MAILER_H
