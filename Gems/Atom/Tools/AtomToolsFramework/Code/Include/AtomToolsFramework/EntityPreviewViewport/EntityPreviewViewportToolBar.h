/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/AssetSelection/AssetSelectionComboBox.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsNotificationBus.h>

#include <QAction>
#include <QToolBar>
#endif

namespace AtomToolsFramework
{
    //! EntityPreviewViewportToolBar contains common, easily accessible controls for manipulating viewport settings
    class EntityPreviewViewportToolBar final
        : public QToolBar
        , public EntityPreviewViewportSettingsNotificationBus::Handler
    {
        Q_OBJECT
    public:
        EntityPreviewViewportToolBar(const AZ::Crc32& toolId, QWidget* parent = 0);
        ~EntityPreviewViewportToolBar();

    private:
        void OnViewportSettingsChanged() override;

        const AZ::Crc32 m_toolId = {};
        AssetSelectionComboBox* m_lightingPresetComboBox = {};
        AssetSelectionComboBox* m_modelPresetComboBox = {};
        QAction* m_toggleGrid = {};
        QAction* m_toggleShadowCatcher = {};
        QAction* m_toggleAlternateSkybox = {};
    };
} // namespace AtomToolsFramework
