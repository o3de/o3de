/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        void OnDisplayMapperOperationTypeChanged(AZ::Render::DisplayMapperOperationType operationType) override;

        QAction* m_toggleGrid = {};
        QAction* m_toggleShadowCatcher = {};

        AZStd::unordered_map<AZ::Render::DisplayMapperOperationType, QString> m_operationNames;
        AZStd::unordered_map<AZ::Render::DisplayMapperOperationType, QAction*> m_operationActions;
    };
} // namespace MaterialEditor
