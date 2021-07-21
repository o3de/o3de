/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_map.h>

#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiInterface.h>

namespace AzToolsFramework
{
    namespace Components
    {
        //! A System Component to manage UI overrides for Editor Entities
        class EditorEntityUiSystemComponent final
            : public AZ::Component
            , public EditorEntityUiInterface
        {
        public:
            AZ_COMPONENT(EditorEntityUiSystemComponent, "{65D3C8AB-7BA7-4FC1-9297-49C9602E32D2}");

            EditorEntityUiSystemComponent();
            ~EditorEntityUiSystemComponent() override;

            static void Reflect(AZ::ReflectContext* context);

            // Component ...
            void Init() override;
            void Activate() override;
            void Deactivate() override;

            // EditorEntityFrameworkInterface ...
            EditorEntityUiHandlerId RegisterHandler(EditorEntityUiHandlerBase* handler) override;
            void UnregisterHandler(EditorEntityUiHandlerBase* handler) override;
            bool RegisterEntity(AZ::EntityId entityId, EditorEntityUiHandlerId handlerId) override;
            bool UnregisterEntity(AZ::EntityId entityId) override;
            EditorEntityUiHandlerBase* GetHandler(AZ::EntityId entityId) override;

        private:
            EditorEntityUiHandlerId m_idCounter = 1;

            AZStd::unordered_map<AZ::EntityId, EditorEntityUiHandlerBase*> m_entityIdHandlerMap;
            AZStd::unordered_map<EditorEntityUiHandlerId, EditorEntityUiHandlerBase*> m_handlerIdHandlerMap;
        };

    } // Components
} // AzToolsFramework
