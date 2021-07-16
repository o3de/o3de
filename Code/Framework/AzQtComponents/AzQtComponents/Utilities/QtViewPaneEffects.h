/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
