/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            provided.push_back(AZ_CRC_CE("CompoundShapeService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        AZ::u32 ConfigurationChanged();

    private:
        // CompoundShapeComponentRequestsBus::Handler
        const CompoundShapeConfiguration& GetCompoundShapeConfiguration() const override
        {
            return m_configuration;
        }

        // CompoundShapeComponentHierarchyRequestsBus::Handler
        bool HasChildId(const AZ::EntityId& entityId) const override;

        bool ValidateChildIds() override;

        bool ValidateConfiguration();

        CompoundShapeConfiguration m_configuration; ///< Stores configuration for this component.
        CompoundShapeComponent m_component;
    };
} // namespace LmbrCentral
