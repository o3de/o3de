/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <Viewport/OpenParticleViewportWidget.h>
#endif

#include <QWidget>
#include <QMenu>
#include <QPoint>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QToolBar>
#include <QAction>
#include <QToolButton>
#include <LightingPresetMenu.h>

namespace OpenParticleSystemEditor
{
    class ViewInspector
        : public QWidget
        , public OpenParticleViewportNotificationBus::Handler
    {
        Q_OBJECT;

    public:
        AZ_CLASS_ALLOCATOR(ViewInspector, AZ::SystemAllocator, 0);

        explicit ViewInspector(QWidget* parent = nullptr);
        ~ViewInspector();

    Q_SIGNALS:

    private:
        void CreatTopBar();
        void CreatePlayerProgressBar();
        void CreatePlayerAction();

        // OpenParticleViewportNotificationBus::Handler overrides...
        void OnGridEnabledChanged([[maybe_unused]] bool enable) override;
        void OnAlternateSkyboxEnabledChanged([[maybe_unused]] bool enable) override;
        void OnDisplayMapperOperationTypeChanged(AZ::Render::DisplayMapperOperationType operationType) override;

        OpenParticleViewportWidget* m_viewPort = nullptr;
        QAction* m_toggleGrid = {};
        QAction* m_toggleAlternateSkybox = {};
        AZStd::unordered_map<AZ::Render::DisplayMapperOperationType, QAction*> m_operationActions;
        QToolButton* m_skyBtn;
        QVBoxLayout* m_verticalLayout = nullptr;
    };
} // namespace OpenParticleSystemEditor
