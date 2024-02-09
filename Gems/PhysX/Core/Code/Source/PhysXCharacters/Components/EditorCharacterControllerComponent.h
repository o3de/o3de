/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <PhysXCharacters/API/CharacterController.h>

namespace PhysX
{
    class CharacterControllerConfiguration;

    /// Proxy container for only displaying a specific shape configuration depending on the shapeType selected.
    struct EditorCharacterControllerProxyShapeConfig
    {
        AZ_CLASS_ALLOCATOR(EditorCharacterControllerProxyShapeConfig, AZ::SystemAllocator);
        AZ_RTTI(EditorCharacterControllerProxyShapeConfig, "{0A9F0213-E281-4424-97C5-BAD2D318F496}");
        static void Reflect(AZ::ReflectContext* context);

        EditorCharacterControllerProxyShapeConfig() = default;
        EditorCharacterControllerProxyShapeConfig(const Physics::ShapeConfiguration& shapeConfiguration);
        virtual ~EditorCharacterControllerProxyShapeConfig() = default;

        Physics::ShapeType m_shapeType = Physics::ShapeType::Capsule;
        Physics::CapsuleShapeConfiguration m_capsule;
        Physics::BoxShapeConfiguration m_box;

        bool IsBoxConfig() const;
        bool IsCapsuleConfig() const;
        virtual const Physics::ShapeConfiguration& GetCurrent() const;
    };

    /// Editor component that allows a PhysX character controller to be edited.
    class EditorCharacterControllerComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzFramework::EntityDebugDisplayEventBus::Handler
        , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorCharacterControllerComponent, "{F361E19D-34C7-4E70-BF1B-909F48305702}",
            AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
            // Character controller acts as dynamic kinematic rigid body,
            // so it also serves the rigid body service.
            provided.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
            provided.push_back(AZ_CRC_CE("PhysicsCharacterControllerService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
            incompatible.push_back(AZ_CRC_CE("PhysicsCharacterControllerService"));
            incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("TransformService"));
        }

        static void GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
        }

        EditorCharacterControllerComponent();

        const CharacterControllerConfiguration& GetCharacterConfiguration() const
        {
            return m_configuration;
        }

        const EditorCharacterControllerProxyShapeConfig& GetShapeConfiguration() const
        {
            return m_proxyShapeConfiguration;
        }

    protected:

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::EntitySelectionEvents
        void OnSelected() override;
        void OnDeselected() override;

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // Editor change notification
        AZ::u32 OnControllerConfigChanged();
        AZ::u32 OnShapeConfigChanged();

    private:
        CharacterControllerConfiguration m_configuration;
        EditorCharacterControllerProxyShapeConfig m_proxyShapeConfiguration;

        AZStd::vector<AZ::Vector3> m_vertexBuffer;
        AZStd::vector<AZ::u32> m_indexBuffer;
        AZStd::vector<AZ::Vector3> m_lineBuffer;

        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler m_physXConfigChangedHandler;
    };
} // namespace PhysX


