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
#include <Viewport/MaterialCanvasViewportNotificationBus.h>
#endif

namespace MaterialCanvas
{
    class MaterialCanvasToolBar
        : public QToolBar
        , public MaterialCanvasViewportNotificationBus::Handler
    {
        Q_OBJECT
    public:
        MaterialCanvasToolBar(QWidget* parent = 0);
        ~MaterialCanvasToolBar();

    private:
        // MaterialCanvasViewportNotificationBus::Handler overrides...
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
} // namespace MaterialCanvas
