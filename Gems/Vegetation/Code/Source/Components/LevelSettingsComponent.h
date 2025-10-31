/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AreaSystemComponent.h>
#include <InstanceSystemComponent.h>
#include <Vegetation/Ebuses/LevelSettingsRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    /*
    * The settings for the area and instance managers
    */
    class LevelSettingsConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(LevelSettingsConfig, AZ::SystemAllocator);
        AZ_RTTI(LevelSettingsConfig, "{794F7DE4-188C-4031-8B00-C2BA0C351A1E}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AreaSystemConfig m_areaSystemConfig;
        InstanceSystemConfig m_instanceSystemConfig;
    };

    inline constexpr AZ::TypeId LevelSettingsComponentTypeId{ "{FDF8520C-933F-4ED5-9B3A-4ABC9B62496C}" };

    /*
    * Sends out updates for the settings for the area and instance managers
    */
    class LevelSettingsComponent
        : public AZ::Component
        , private LevelSettingsRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(LevelSettingsComponent, LevelSettingsComponentTypeId);
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);

        LevelSettingsComponent(const LevelSettingsConfig& configuration);
        LevelSettingsComponent() = default;
        ~LevelSettingsComponent() override;


        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

    protected:
        void UpdateSystemConfig();

        LevelSettingsConfig m_configuration;
        AreaSystemConfig m_previousAreaSystemConfig;
        InstanceSystemConfig m_previousInstanceSystemConfig;
        bool m_componentActivated = false;

        //////////////////////////////////////////////////////////////////////////
        // LevelSettingsRequestBus
        AreaSystemConfig* GetAreaSystemConfig() override;
        InstanceSystemConfig* GetInstanceSystemConfig() override;

        bool m_active = false;
    };

} // namespace Vegetation
