/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector3.h>
#include <TerrainSystem/TerrainSystem.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    class TerrainWorldDebuggerConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainWorldDebuggerConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainWorldDebuggerConfig, "{92686FA9-2C0B-47F1-8E2D-F2F302CDE5AA}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        bool m_debugWireframeEnabled{true};
    };


    class TerrainWorldDebuggerComponent
        : public AZ::Component
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TerrainWorldDebuggerComponent, "{ECA1F4CB-5395-41FD-B6ED-FFD2C80096E2}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainWorldDebuggerComponent(const TerrainWorldDebuggerConfig& configuration);
        TerrainWorldDebuggerComponent() = default;
        ~TerrainWorldDebuggerComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

    private:
        TerrainWorldDebuggerConfig m_configuration;
    };
}
