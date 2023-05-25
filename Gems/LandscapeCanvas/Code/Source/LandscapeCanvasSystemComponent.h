/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <LandscapeCanvas/LandscapeCanvasBus.h>

namespace LandscapeCanvas
{
    class LandscapeCanvasSystemComponent
        : public AZ::Component
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private LandscapeCanvasNodeFactoryRequestBus::Handler
        , private LandscapeCanvasSerializationRequestBus::Handler
        , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(LandscapeCanvasSystemComponent, "{891CA15A-725A-430F-B395-BCA005CFF606}");

        LandscapeCanvasSystemComponent();
        ~LandscapeCanvasSystemComponent() override;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ////////////////////////////////////////////////////////////////////////
        // AztoolsFramework::EditorEvents::Bus::Handler overrides
        void NotifyRegisterViews() override;
        ////////////////////////////////////////////////////////////////////////

        // ActionManagerRegistrationNotificationBus overrides ...
        void OnActionContextRegistrationHook() override;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // LandscapeCanvas::LandscapeCanvasNodeFactoryRequestBus::Handler overrides
        BaseNode::BaseNodePtr CreateNodeForType(GraphModel::GraphPtr graph, const AZ::TypeId& typeId) override;
        GraphModel::NodePtr CreateNodeForTypeName(GraphModel::GraphPtr graph, AZStd::string_view nodeName) override;
        const AZ::TypeId GetComponentTypeId(const AZ::TypeId& nodeTypeId) override;
        int GetNodeRegisteredIndex(const AZ::TypeId& nodeTypeId) const override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // LandscapeCanvas::LandscapeCanvasSerializationRequestBus::Handler overrides
        const LandscapeCanvasSerialization& GetSerializedMappings() override;
        void SetDeserializedEntities(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& entities) override;
        ////////////////////////////////////////////////////////////////////////

        using NodeFactoryFunction = AZStd::function<BaseNode::BaseNodePtr(GraphModel::GraphPtr)>;

        template<typename NodeType>
        void RegisterFactoryMethod(const AZ::TypeId& typeId)
        {
            NodeFactoryFunction factory = [](GraphModel::GraphPtr graph)
            {
                return AZStd::make_shared<NodeType>(graph);
            };

            m_nodeFactory[typeId] = factory;
            m_nodeComponentTypeIds[AZ::RttiTypeId<NodeType>()] = AZStd::make_pair(typeId, aznumeric_cast<AZ::u32>(m_nodeComponentTypeIds.size()));
        }

        AZ::SerializeContext* m_serializeContext = nullptr;

        // Map where the key is the Component TypeId and the value is the factory function for creating a new node for that component type
        AZStd::unordered_map<AZ::TypeId, NodeFactoryFunction> m_nodeFactory;
        // Map where the key is the node RTTI TypeId and the value is a pair with the
        // corresponding Component TypeId plus an index so we can keep track of the order they were registered
        AZStd::unordered_map<AZ::TypeId, AZStd::pair<AZ::TypeId, AZ::u32>> m_nodeComponentTypeIds;

        LandscapeCanvasSerialization m_serialization;
    };
}
