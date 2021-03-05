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
#ifndef AZFRAMEWORK_LOGGINGCOMMON_H
#define AZFRAMEWORK_LOGGINGCOMMON_H

#pragma once

#include <qnamespace.h>

namespace AzToolsFramework
{
    namespace LogPanel
    {
        enum ExtraRoles
        {
            // your model's DATA() function should return boolean true for this on any elements that are rich text:
            RichTextRole = Qt::UserRole + 1,
            LogLineRole = Qt::UserRole + 2, // return the actual s_logLine pointer if the underlying data can synthesize it
        };
    }
}

#endif