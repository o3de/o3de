/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QMenu>
#include <QAction>
#include <Viewport/OpenParticleViewportNotificationBus.h>
#endif

namespace OpenParticleSystemEditor
{
    class LightingPresetMenu
        : public QMenu
        , public OpenParticleViewportNotificationBus::Handler
    {
        Q_OBJECT
    public:
        LightingPresetMenu(const QString& title, QWidget* parent = 0);
        ~LightingPresetMenu();
        void Refresh();
    Q_SIGNALS:
        void ActionIndexChange(const int index);

    protected slots:
        void ClickAction();

    private:
        // OpenParticleViewportNotificationsBus::Handler overrides...
        void OnLightingPresetSelected(AZStd::string preset) override;
        void OnLightingPresetAdded(AZ::Render::LightingPresetPtr preset) override;
        void OnLightingPresetChanged(AZ::Render::LightingPresetPtr preset) override;
        void OnBeginReloadContent() override;
        void OnEndReloadContent() override;

        bool m_reloading = false;
        AZStd::set<AZStd::string> m_presetNameSet;
        AZStd::vector<QAction*> m_actions;
    };
} // namespace OpenParticleSystemEditor
