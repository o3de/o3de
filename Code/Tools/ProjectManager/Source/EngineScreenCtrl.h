/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <ScreenWidget.h>
#endif

QT_FORWARD_DECLARE_CLASS(QTabWidget)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(EngineSettingsScreen)
    QT_FORWARD_DECLARE_CLASS(GemRepoScreen)

    class EngineScreenCtrl
        : public ScreenWidget
    {
    public:
        explicit EngineScreenCtrl(QWidget* parent = nullptr);
        ~EngineScreenCtrl() = default;
        ProjectManagerScreen GetScreenEnum() override;

        QString GetTabText() override;
        bool IsTab() override;
        bool ContainsScreen(ProjectManagerScreen screen) override;
        void GoToScreen(ProjectManagerScreen screen) override;

        QTabWidget* m_tabWidget = nullptr;
        EngineSettingsScreen* m_engineSettingsScreen = nullptr;
        GemRepoScreen* m_gemRepoScreen = nullptr;
    };

} // namespace O3DE::ProjectManager
