/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
