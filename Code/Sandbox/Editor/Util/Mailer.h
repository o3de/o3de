/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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
