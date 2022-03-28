/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportWidget.h>
#endif

namespace MaterialEditor
{
    class MaterialEditorViewportWidget final
        : public AtomToolsFramework::EntityPreviewViewportWidget
        , public AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
    {
    public:
        MaterialEditorViewportWidget(
            const AZ::Crc32& toolId,
            const AZStd::string& sceneName,
            const AZStd::string pipelineAssetPath,
            QWidget* parent = nullptr);
        ~MaterialEditorViewportWidget();

        void Init() override;
        AZ::Aabb GetObjectBoundsLocal() const override;
        AZ::Aabb GetObjectBoundsWorld() const override;
        AZ::EntityId GetObjectEntityId() const override;
        AZ::EntityId GetEnvironmentEntityId() const override;
        AZ::EntityId GetPostFxEntityId() const override;
        AZ::EntityId GetShadowCatcherEntityId() const;
        AZ::EntityId GetGridEntityId() const;

        void CreateEntities() override;

    private:
        // AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;

        // EntityPreviewViewportSettingsNotificationBus::Handler overrides...
        void OnViewportSettingsChanged() override;

        AZ::Entity* m_environmentEntity = {};
        AZ::Entity* m_gridEntity = {};
        AZ::Entity* m_objectEntity = {};
        AZ::Entity* m_postFxEntity = {};
        AZ::Entity* m_shadowCatcherEntity = {};
    };
} // namespace MaterialEditor
