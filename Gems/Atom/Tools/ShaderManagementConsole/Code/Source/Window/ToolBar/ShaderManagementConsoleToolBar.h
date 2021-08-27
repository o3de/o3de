/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QToolBar>
#endif

namespace ShaderManagementConsole
{
    class ModelComboBox;
    class LightingPresetComboBox;

    class ShaderManagementConsoleToolBar
        : public QToolBar
    {
        Q_OBJECT
    public:

        ShaderManagementConsoleToolBar(QWidget* parent = 0);
        ~ShaderManagementConsoleToolBar();

    private:
    };
} // namespace ShaderManagementConsole
