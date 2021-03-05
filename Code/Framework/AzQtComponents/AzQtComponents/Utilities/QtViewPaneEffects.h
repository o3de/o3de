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

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

class QWidget;

namespace AzQtComponents
{
    /// Set the state of all widgets that should be enabled/disabled.
    /// When \p on is \p false, the widget is set to disabled and a graphics effect is applied
    /// to show the widget as inactive. The reverse of this is applied when \p on is \p true.
    AZ_QT_COMPONENTS_API void SetWidgetInteractEnabled(QWidget* widget, bool on);
    
} // namespace AzQtComponents