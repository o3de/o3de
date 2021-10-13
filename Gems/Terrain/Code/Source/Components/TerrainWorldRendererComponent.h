/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace AZ::RPI
{
    class Scene;
}

namespace Terrain
{
    class TerrainFeatureProcessor;

    struct TerrainWorldRendererConfig final
        : public AZ::ComponentConfig
    {
        AZ_CLASS_ALLOCATOR(TerrainWorldRendererConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainWorldRendererConfig, "{08C5863C-092D-4A69-8226-4978E4F6E343}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        enum class WorldSize : uint8_t
        {
            Unknown,

            _512Meters,
            _1024Meters,
            _2048Meters,
            _4096Meters,
            _8192Meters,
            _16384Meters,

            WorldSizeCount,
        };

        WorldSize m_worldSize;
    };


    class TerrainWorldRendererComponent
        : public AZ::Component
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TerrainWorldRendererComponent, "{3B0DB71E-5944-437C-8C88-70F8B405BFC7}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainWorldRendererComponent(const TerrainWorldRendererConfig& configuration);
        TerrainWorldRendererComponent() = default;
        ~TerrainWorldRendererComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

    protected:

        AZ::RPI::Scene* GetScene() const;

    private:
        TerrainWorldRendererConfig m_configuration;
        bool m_terrainRendererActive{ false };
        TerrainFeatureProcessor* m_terrainFeatureProcessor{ nullptr };
    };
}
