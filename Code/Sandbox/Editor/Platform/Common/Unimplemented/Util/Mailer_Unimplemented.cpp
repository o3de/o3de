/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Send Mail.


#include "EditorDefs.h"

#include "Util/Mailer.h"

bool CMailer::SendMail(const char* subject,
    const char* messageBody,
    const std::vector<const char*>& _recipients,
    const std::vector<const char*>& _attachments,
    bool bShowDialog)
{
    return true;
}
