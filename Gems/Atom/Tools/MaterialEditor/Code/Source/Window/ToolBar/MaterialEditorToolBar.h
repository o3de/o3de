/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QAction>
#include <QToolBar>
#include <Atom/Viewport/MaterialViewportNotificationBus.h>
#endif

namespace MaterialEditor
{
    class MaterialEditorToolBar
        : public QToolBar
        , public MaterialViewportNotificationBus::Handler
    {
        Q_OBJECT
    public:
        MaterialEditorToolBar(QWidget* parent = 0);
        ~MaterialEditorToolBar();

    private:
        // MaterialViewportNotificationBus::Handler overrides...
        void OnShadowCatcherEnabledChanged([[maybe_unused]] bool enable) override;
        void OnGridEnabledChanged([[maybe_unused]] bool enable) override;
        void OnAlternateSkyboxEnabledChanged([[maybe_unused]] bool enable) override;
        void OnDisplayMapperOperationTypeChanged(AZ::Render::DisplayMapperOperationType operationType) override;

        QAction* m_toggleGrid = {};
        QAction* m_toggleShadowCatcher = {};
        QAction* m_toggleAlternateSkybox = {};

        AZStd::unordered_map<AZ::Render::DisplayMapperOperationType, QString> m_operationNames;
        AZStd::unordered_map<AZ::Render::DisplayMapperOperationType, QAction*> m_operationActions;
    };
} // namespace MaterialEditor
