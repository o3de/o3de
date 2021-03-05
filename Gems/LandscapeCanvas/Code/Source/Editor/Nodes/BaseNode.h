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

// AZ
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

// Graph Model
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
        AZ_CLASS_ALLOCATOR(BaseNode, AZ::SystemAllocator, 0);
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

        const AZ::ComponentId& GetComponentId() const { return m_componentId; }
        void SetComponentId(const AZ::ComponentId& componentId);

        /// Retrieve a pointer to the Component on the respective Entity that
        /// this Node represents
        AZ::Component* GetComponent() const;

        /// By default our Landscape Canvas nodes will have a property display
        /// to show the name of the Entity the component lives on
        virtual const bool ShouldShowEntityName() const { return true; }

        /// Returns whether or not this node is a Vegetation Area Extender (Filter/Modifier/Selector)
        bool IsAreaExtender() const;

    protected:
        /// Create the property slot on our node to show the Entity name
        void CreateEntityNameSlot();

        /// EntityId of the Vegetation Entity that holds this node
        AZ::EntityId m_vegetationEntityId;
        AZ::ComponentId m_componentId = AZ::InvalidComponentId;
    };
}
