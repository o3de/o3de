/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>

#include <GraphicsGem_AR_Test/GraphicsGem_AR_TestFeatureProcessorInterface.h>

namespace GraphicsGem_AR_Test
{
    class GraphicsGem_AR_TestComponentConfig final
        : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(GraphicsGem_AR_TestComponentConfig, "{769A36B6-BC93-4C78-842B-AB38FC17F410}", ComponentConfig);
        AZ_CLASS_ALLOCATOR(GraphicsGem_AR_TestComponentConfig, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        GraphicsGem_AR_TestComponentConfig() = default;

        AZ::u64 m_entityId{ AZ::EntityId::InvalidEntityId };
    };

    class GraphicsGem_AR_TestComponentController final
        : public AZ::Data::AssetBus::MultiHandler
        , private AZ::TransformNotificationBus::Handler
    {
    public:
        friend class EditorGraphicsGem_AR_TestComponent;

        AZ_RTTI(GraphicsGem_AR_TestComponentController, "{5533CEB4-E361-4266-A2B0-F6EAD934CC3C}");
        AZ_CLASS_ALLOCATOR(GraphicsGem_AR_TestComponentController, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        GraphicsGem_AR_TestComponentController() = default;
        GraphicsGem_AR_TestComponentController(const GraphicsGem_AR_TestComponentConfig& config);

        void Activate(AZ::EntityId entityId);
        void Deactivate();
        void SetConfiguration(const GraphicsGem_AR_TestComponentConfig& config);
        const GraphicsGem_AR_TestComponentConfig& GetConfiguration() const;

    private:

        AZ_DISABLE_COPY(GraphicsGem_AR_TestComponentController);

        // TransformNotificationBus overrides
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // handle for this probe in the feature processor
        GraphicsGem_AR_TestHandle m_handle;

        GraphicsGem_AR_TestFeatureProcessorInterface* m_featureProcessor = nullptr;
        AZ::TransformInterface* m_transformInterface = nullptr;
        AZ::EntityId m_entityId;
        
        GraphicsGem_AR_TestComponentConfig m_configuration;

    };
}
