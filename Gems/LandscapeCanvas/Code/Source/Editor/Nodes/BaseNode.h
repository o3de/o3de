/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <GraphModel/Model/Node.h>

namespace AZ
{
    class Component;
    class ReflectContext;
}

namespace LandscapeCanvas
{
    /**
     * Base Node class that all our LandscapeCanvas nodes must derive from for
     * tracking the associated vegetation Entity that holds the node, and other
     * common functionality
     */
    class BaseNode : public GraphModel::Node
    {
    public:
        AZ_CLASS_ALLOCATOR(BaseNode, AZ::SystemAllocator);
        AZ_RTTI(BaseNode, "{94ECF2FF-C46C-4CCA-878C-5C47B943B6B7}", Node);

        using BaseNodePtr = AZStd::shared_ptr<BaseNode>;

        enum BaseNodeType
        {
            Invalid = -1,
            Shape,
            VegetationArea,
            Gradient,
            GradientGenerator,
            GradientModifier,
            TerrainArea,
            TerrainExtender,
            TerrainSurfaceExtender,
            VegetationAreaModifier,
            VegetationAreaFilter,
            VegetationAreaSelector
        };

        static void Reflect(AZ::ReflectContext* context);

        BaseNode() = default;
        explicit BaseNode(GraphModel::GraphPtr graph);

        virtual const BaseNodeType GetBaseNodeType() const { return Invalid; }

        const AZ::EntityId& GetVegetationEntityId() const { return m_vegetationEntityId; }
        void SetVegetationEntityId(const AZ::EntityId& entityId);

        /// Refresh the name in the entity name property slot
        void RefreshEntityName();

        const AZ::ComponentId& GetComponentId() const { return m_componentId; }
        void SetComponentId(const AZ::ComponentId& componentId);

        virtual AZ::ComponentDescriptor::DependencyArrayType GetOptionalRequiredServices() const;

        /// Retrieve a pointer to the Component on the respective Entity that
        /// this Node represents
        AZ::Component* GetComponent() const;

        /// Returns whether or not this node is a Vegetation Area Extender (Filter/Modifier/Selector)
        bool IsAreaExtender() const;

        /// Override the PostLoadSetup calls to ensure the entity name is refreshed correctly.
        void PostLoadSetup(GraphModel::GraphPtr graph, GraphModel::NodeId id) override;
        void PostLoadSetup() override;

    protected:
        /// Create the property slot on our node to show the Entity name
        void CreateEntityNameSlot();

        /// EntityId of the Vegetation Entity that holds this node
        AZ::EntityId m_vegetationEntityId;
        AZ::ComponentId m_componentId = AZ::InvalidComponentId;
    };
}
