/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QCoreApplication>

namespace Camera
{
    static const char* const s_viewportCameraSelectorName = QT_TRANSLATE_NOOP("Camera", "Viewport Camera Selector");

    extern void RegisterViewportCameraSelectorWindow();
} // namespace Camera
