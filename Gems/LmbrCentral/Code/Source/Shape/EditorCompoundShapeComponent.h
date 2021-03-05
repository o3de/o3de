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

#include "EditorBaseShapeComponent.h"
#include "CompoundShapeComponent.h"
#include <LmbrCentral/Shape/CompoundShapeComponentBus.h>

namespace LmbrCentral
{
    class EditorCompoundShapeComponent
        : public EditorBaseShapeComponent
        , private CompoundShapeComponentRequestsBus::Handler
        , private CompoundShapeComponentHierarchyRequestsBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorCompoundShapeComponent, EditorCompoundShapeComponentTypeId, EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            EditorBaseShapeComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("CompoundShapeService", 0x4f7c640a));
        }

        AZ::u32 ConfigurationChanged();

    private:
        // CompoundShapeComponentRequestsBus::Handler
        CompoundShapeConfiguration GetCompoundShapeConfiguration() override
        {
            return m_configuration;
        }

        // CompoundShapeComponentHierarchyRequestsBus::Handler
        bool HasChildId(const AZ::EntityId& entityId) override;

        bool ValidateChildIds() override;

        bool ValidateConfiguration();

        CompoundShapeConfiguration m_configuration; ///< Stores configuration for this component.
        CompoundShapeComponent m_component;
    };
} // namespace LmbrCentral
