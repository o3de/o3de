/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

class QWidget;

namespace AzToolsFramework
{
    bool IsNewActionManagerEnabled();

    void AssignWidgetToActionContextHelper(const AZStd::string& actionContextIdentifier, QWidget* widget);
    void RemoveWidgetFromActionContextHelper(const AZStd::string& actionContextIdentifier, QWidget* widget);

} // namespace AzToolsFramework
