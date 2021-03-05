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

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzCore/std/containers/vector.h>
#include <LmbrCentral/Scripting/TagComponentBus.h>
#include <AzCore/std/string/string.h>
#include "TagComponent.h"
#include <LmbrCentral/Scripting/EditorTagComponentBus.h>

namespace LmbrCentral
{
    /**
    * Tag Component
    *
    * Simple component that tags an entity with a list of filters or descriptors
    *
    */
    class EditorTagComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private LmbrCentral::EditorTagComponentRequestBus::Handler
        , private LmbrCentral::TagGlobalRequestBus::MultiHandler
    {
    public:
        AZ_COMPONENT(EditorTagComponent,
            "{5272B56C-6CCC-4118-8539-D881F463ACD1}",
            AzToolsFramework::Components::EditorComponentBase);

        ~EditorTagComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    private:
        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::Components::EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        //////////////////////////////////////////////////////////////////////////
        // Component descriptor

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("TagService", 0xf1ef347d));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("TagService", 0xf1ef347d));
        }

        //////////////////////////////////////////////////////////////////////////


        //////////////////////////////////////////////////////////////////////////
        // TagGlobalRequestBus::MultiHandler
        const AZ::EntityId RequestTaggedEntities() override { return GetEntityId(); }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorTagComponentRequestBus::Handler
        bool HasTag(const char* tag) override;
        void AddTag(const char* tag) override;
        void RemoveTag(const char* tag) override;
        const EditorTags& GetTags() override { return m_tags; }
        //////////////////////////////////////////////////////////////////////////

        void ActivateTag(const char* tagName);
        void DeactivateTag(const char* tagName);
        void ActivateTags();
        void DeactivateTags();

        void OnTagChanged();

        EditorTags m_activeTags;

        // Reflected Data
        EditorTags m_tags;
    };
} // namespace LmbrCentral
