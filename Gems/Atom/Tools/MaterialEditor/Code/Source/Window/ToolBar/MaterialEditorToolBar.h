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
#include <Viewport/MaterialViewportSettingsNotificationBus.h>

#include <QAction>
#include <QToolBar>
#endif

namespace MaterialEditor
{
    class MaterialEditorToolBar
        : public QToolBar
        , public MaterialViewportSettingsNotificationBus::Handler
    {
        Q_OBJECT
    public:
        MaterialEditorToolBar(const AZ::Crc32& toolId, QWidget* parent = 0);
        ~MaterialEditorToolBar();

    private:
        void OnViewportSettingsChanged() override;

        const AZ::Crc32 m_toolId = {};
        AtomToolsFramework::AssetSelectionComboBox* m_lightingPresetComboBox = {};
        AtomToolsFramework::AssetSelectionComboBox* m_modelPresetComboBox = {};
        QAction* m_toggleGrid = {};
        QAction* m_toggleShadowCatcher = {};
        QAction* m_toggleAlternateSkybox = {};

        AZStd::unordered_map<AZ::Render::DisplayMapperOperationType, QString> m_operationNames;
        AZStd::unordered_map<AZ::Render::DisplayMapperOperationType, QAction*> m_operationActions;
    };
} // namespace MaterialEditor
