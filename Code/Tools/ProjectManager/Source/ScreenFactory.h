/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScreenDefs.h>

#include <QWidget>

namespace O3DE::ProjectManager
{
    class ScreenWidget;

    ScreenWidget* BuildScreen(QWidget* parent = nullptr, ProjectManagerScreen screen = ProjectManagerScreen::Empty);
} // namespace O3DE::ProjectManager
