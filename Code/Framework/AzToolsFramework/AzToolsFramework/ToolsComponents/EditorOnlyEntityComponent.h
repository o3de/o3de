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

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>

#pragma once

namespace AzToolsFramework
{
    namespace Components
    {
        /**
         * Acts as storage for the "editor-only" flag on entities, and offers an API for getting/setting the value.
         */
        class EditorOnlyEntityComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorOnlyEntityComponentRequestBus::Handler
        {
        public:
            AZ_EDITOR_COMPONENT(EditorOnlyEntityComponent, "{22A16F1D-6D49-422D-AAE9-91AE45B5D3E7}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);

            ////////////////////////////////////////////////////////////////////
            // EditorOnlyEntityComponentRequestBus
            bool IsEditorOnlyEntity() override;
            void SetIsEditorOnlyEntity(bool isEditorOnly) override;
            ////////////////////////////////////////////////////////////////////

            EditorOnlyEntityComponent();
            ~EditorOnlyEntityComponent() override;

        private:
            ////////////////////////////////////////////////////////////////////
            // AZ::Entity
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////

            bool m_isEditorOnly; ///< Is the entity marked as editor-only?
        };

    } // namespace Components

} // namespace AzToolsFramework

