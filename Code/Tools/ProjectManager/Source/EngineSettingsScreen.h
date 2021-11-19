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

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(FormLineEditWidget)
    QT_FORWARD_DECLARE_CLASS(FormBrowseEditWidget)

    class EngineSettingsScreen
        : public ScreenWidget
    {
    public:
        explicit EngineSettingsScreen(QWidget* parent = nullptr);
        ~EngineSettingsScreen() = default;

        ProjectManagerScreen GetScreenEnum() override;

    protected slots:
        void OnTextChanged();

    private:
        FormBrowseEditWidget* m_thirdParty;
        FormBrowseEditWidget* m_defaultProjects;
        FormBrowseEditWidget* m_defaultGems;
        FormBrowseEditWidget* m_defaultProjectTemplates;
    };

} // namespace O3DE::ProjectManager
