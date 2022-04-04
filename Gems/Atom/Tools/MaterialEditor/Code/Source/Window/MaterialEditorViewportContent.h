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
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportContent.h>
#endif

namespace MaterialEditor
{
    class MaterialEditorViewportContent final
        : public AtomToolsFramework::EntityPreviewViewportContent
        , public AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
    {
    public:
        MaterialEditorViewportContent(
            const AZ::Crc32& toolId,
            AtomToolsFramework::RenderViewportWidget* widget,
            AZStd::shared_ptr<AzFramework::EntityContext> entityContext);
        ~MaterialEditorViewportContent();

        AZ::EntityId GetObjectEntityId() const override;
        AZ::EntityId GetEnvironmentEntityId() const override;
        AZ::EntityId GetPostFxEntityId() const override;
        AZ::EntityId GetShadowCatcherEntityId() const;
        AZ::EntityId GetGridEntityId() const;

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
