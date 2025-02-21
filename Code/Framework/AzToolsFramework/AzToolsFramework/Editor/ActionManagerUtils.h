/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
//AZTF-SHARED

#include <AzCore/std/string/string.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

class QWidget;

namespace AzToolsFramework
{
    AZTF_API bool IsNewActionManagerEnabled();

    AZTF_API void AssignWidgetToActionContextHelper(const AZStd::string& actionContextIdentifier, QWidget* widget);
    AZTF_API void RemoveWidgetFromActionContextHelper(const AZStd::string& actionContextIdentifier, QWidget* widget);

} // namespace AzToolsFramework
