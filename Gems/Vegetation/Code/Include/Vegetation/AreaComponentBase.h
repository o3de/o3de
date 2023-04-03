/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <Vegetation/Ebuses/AreaConfigRequestBus.h>
#include <Vegetation/Ebuses/AreaNotificationBus.h>
#include <Vegetation/Ebuses/AreaRequestBus.h>
#include <Vegetation/Ebuses/AreaInfoBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzCore/Component/TransformBus.h>

namespace Vegetation
{
    namespace AreaConstants
    {
        static const AZ::u32 s_backgroundLayer = 0;
        static const AZ::u32 s_foregroundLayer = 1;
        static const AZ::u32 s_priorityMin = 0;
        static const AZ::u32 s_priorityMax = 10000; //arbitrary number because std::numeric_limits<AZ::u32>::max() always dislays -1 in RPE
        static const AZ::u32 s_prioritySoftMax = 100; //design specified slider range
    }

    class AreaConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(AreaConfig, AZ::SystemAllocator);
        AZ_RTTI(AreaConfig, "{61599E53-2B6A-40AC-B5B8-FC1C3F87275E}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> GetSelectableLayers() const;
        AZ::u32 m_layer = AreaConstants::s_foregroundLayer;
        AZ::u32 m_priority = AreaConstants::s_priorityMin;
    };

    class AreaComponentBase
        : public AZ::Component
        , protected AreaNotificationBus::Handler
        , protected AreaRequestBus::Handler
        , protected AreaInfoBus::Handler
        , protected LmbrCentral::DependencyNotificationBus::Handler
        , protected AZ::TransformNotificationBus::Handler
        , protected LmbrCentral::ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_RTTI(AreaComponentBase, "{A50180C3-C14C-4292-BDBA-D7215F2EA7AB}", AZ::Component);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        AreaComponentBase(const AreaConfig& configuration);
        AreaComponentBase() = default;
        ~AreaComponentBase() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // AreaInfoBus
        AZ::u32 GetLayer() const override;
        AZ::u32 GetPriority() const override;
        AZ::u32 GetChangeIndex() const override;

        //////////////////////////////////////////////////////////////////////////
        // LmbrCentral::DependencyNotificationBus
        void OnCompositionChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // AreaNotificationBus
        void OnAreaConnect() override;
        void OnAreaDisconnect() override;
        void OnAreaRefreshed() override;

        //////////////////////////////////////////////////////////////////////////
        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        ////////////////////////////////////////////////////////////////////////
        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons reasons) override;

    protected:
        // Give subclasses the ability to do work whenever areas are registered and unregistered
        virtual void OnRegisterArea() {}
        virtual void OnUnregisterArea() {}

    private:
        void UpdateRegistration();

        AreaConfig m_configuration;
        bool m_areaRegistered { false };
        AZStd::atomic_int m_changeIndex{ 0 };
    };
}
