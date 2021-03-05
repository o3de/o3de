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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzToolsFramework/Component/EditorLevelComponentAPIBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace AzToolsFramework
{
    namespace Components
    {
        //! A System Component to reflect Editor operations on Components to Behavior Context
        class EditorLevelComponentAPIComponent
            : public AZ::Component
            , public EditorLevelComponentAPIBus::Handler
        {
        public:
            AZ_COMPONENT(EditorLevelComponentAPIComponent, "{F3A07F35-9679-4A88-B123-85474ECFEC21}");

            EditorLevelComponentAPIComponent() = default;
            ~EditorLevelComponentAPIComponent() = default;

            static void Reflect(AZ::ReflectContext* context);
            
            // Component ...
            void Activate() override;
            void Deactivate() override;
            
            // EditorLevelComponentAPIBus ...
            EditorComponentAPIRequests::AddComponentsOutcome AddComponentsOfType(const AZ::ComponentTypeList& componentTypeIds) override;
            bool HasComponentOfType(AZ::Uuid componentTypeId) override;
            size_t CountComponentsOfType(AZ::Uuid componentTypeId) override;
            EditorComponentAPIRequests::GetComponentOutcome GetComponentOfType(AZ::Uuid componentTypeId) override;
            EditorComponentAPIRequests::GetComponentsOutcome GetComponentsOfType(AZ::Uuid componentTypeId) override;
        };

    } // Components
} // AzToolsFramework
