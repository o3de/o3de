/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Send Mail.


#include "EditorDefs.h"

#include "Util/Mailer.h"

#include <mapi.h>


bool CMailer::SendMail(const char* subject,
    const char* messageBody,
    const std::vector<const char*>& _recipients,
    const std::vector<const char*>& _attachments,
    bool bShowDialog)
{
    // Preserve directory, (Can be changed if attachment specified)
    WCHAR dir[MAX_PATH];
    GetCurrentDirectoryW(sizeof(dir), dir);

    // Load MAPI dll
    HMODULE hMAPILib = LoadLibraryW(L"MAPI32.DLL");
    LPMAPISENDMAIL lpfnMAPISendMail = (LPMAPISENDMAIL) GetProcAddress(hMAPILib, "MAPISendMail");

    int numRecipients  = (int)_recipients.size();

    // Handle Attachments
    MapiFileDesc* attachments = new MapiFileDesc[_attachments.size()];

    int i = 0;
    for (unsigned int k = 0; k < _attachments.size(); k++)
    {
        FILE* file = nullptr;
        azfopen(&file, _attachments[k], "r");
        if (!file)
        {
            continue;
        }
        fclose(file);

        attachments[i].ulReserved   = 0;
        attachments[i].flFlags      = 0;
        attachments[i].nPosition    = (ULONG)-1;
        attachments[i].lpszPathName = (char*)(const char*)_attachments[k];
        attachments[i].lpszFileName = nullptr;
        attachments[i].lpFileType   = nullptr;
        i++;
    }
    int numAttachments = i;

    // Handle Recipients
    MapiRecipDesc* recipients = new MapiRecipDesc[numRecipients];

    std::vector<AZStd::string> addresses;
    addresses.resize(numRecipients);
    for (i = 0; i < numRecipients; i++)
    {
        addresses[i] = AZStd::string("SMTP:") + _recipients[i];
    }

    for (i = 0; i < numRecipients; i++)
    {
        recipients[i].ulReserved   = 0;
        recipients[i].ulRecipClass = MAPI_TO;
        recipients[i].lpszName     = (char*)(const char*)_recipients[i];
        recipients[i].lpszAddress  = (char*)addresses[i].c_str();
        recipients[i].ulEIDSize    = 0;
        recipients[i].lpEntryID    = nullptr;
    }

    MapiMessage message;
    memset(&message, 0, sizeof(message));
    message.lpszSubject = (char*)(const char*)subject;
    message.lpszNoteText = (char*)(const char*)messageBody;
    message.lpszMessageType = nullptr;

    message.nRecipCount = numRecipients;
    message.lpRecips = recipients;

    message.nFileCount = numAttachments;
    message.lpFiles = attachments;


    //Next, the client calls the MAPISendMail function and stores the return status so it can detect whether the call succeeded. You should use a more sophisticated error reporting mechanism than the C library function printf.

    FLAGS flags = bShowDialog ? MAPI_DIALOG : 0;
    flags |= MAPI_LOGON_UI;
    ULONG err = (*lpfnMAPISendMail)(0L,           // use implicit session.
            0L,      // ulUIParam; 0 is always valid
            &message, // the message being sent
            flags,   // if user allowed to edit the message
            0L);     // reserved; must be 0
    delete [] attachments;
    delete [] recipients;

    FreeLibrary(hMAPILib);   // Free DLL module through handle

    // Restore previous directory.
    SetCurrentDirectoryW(dir);

    if (err != SUCCESS_SUCCESS)
    {
        return false;
    }

    return true;
}

