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
#include <${Name}/${Name}FeatureProcessorInterface.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <Components/${Name}ComponentConstants.h>

namespace AZ::Render
{
    class ${Name}ComponentConfig final
        : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(AZ::Render::${Name}ComponentConfig, "{${Random_Uuid}}", ComponentConfig);
        AZ_CLASS_ALLOCATOR(${Name}ComponentConfig, SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        ${Name}ComponentConfig() = default;

        AZ::u64 m_entityId{ EntityId::InvalidEntityId };
    };

    class ${Name}ComponentController final
        : public Data::AssetBus::MultiHandler
        , private TransformNotificationBus::Handler
    {
    public:
        friend class Editor${Name}Component;

        AZ_CLASS_ALLOCATOR(${Name}ComponentController, AZ::SystemAllocator);
        AZ_RTTI(${Name}ComponentController, "{${Random_Uuid}}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        ${Name}ComponentController() = default;
        ${Name}ComponentController(const ${Name}ComponentConfig& config);

        void Activate(AZ::EntityId entityId);
        void Deactivate();
        void SetConfiguration(const ${Name}ComponentConfig& config);
        const ${Name}ComponentConfig& GetConfiguration() const;

    private:

        AZ_DISABLE_COPY(${Name}ComponentController);

        // TransformNotificationBus overrides
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // handle for this probe in the feature processor
        ${Name}Handle m_handle;

        ${Name}FeatureProcessorInterface* m_featureProcessor = nullptr;
        TransformInterface* m_transformInterface = nullptr;
        AZ::EntityId m_entityId;
        ${Name}ComponentConfig m_configuration;

    };
}
