/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            provided.push_back(AZ_CRC_CE("TagService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("TagService"));
        }

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
