/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/SkyAtmosphere/SkyAtmosphereFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Feature/Utils/IndexedDataVector.h>
#include <Atom/Feature/Utils/SparseVector.h>
#include <SkyAtmosphere/SkyAtmosphereParentPass.h>

namespace AZ::Render
{
    //! This feature processor manages drawing sky atmospheres.
    //! Use CreateAtmosphere() to create a new atmosphere
    class SkyAtmosphereFeatureProcessor final
        : public SkyAtmosphereFeatureProcessorInterface
    {
    public:

        AZ_RTTI(AZ::Render::SkyAtmosphereFeatureProcessor, "{FB3155E9-BA3C-487B-B251-EB4BF3465E02}", AZ::Render::SkyAtmosphereFeatureProcessorInterface);

        static void Reflect(AZ::ReflectContext* context);

        SkyAtmosphereFeatureProcessor() = default;
        virtual ~SkyAtmosphereFeatureProcessor() = default;

        //! FeatureProcessor 
        void Activate() override;
        void Deactivate() override;
        void Simulate(const SimulatePacket& packet) override;

        //! SkyAtmosphereFeatureProcessorInterface
        AtmosphereId CreateAtmosphere() override;
        void ReleaseAtmosphere(AtmosphereId id) override;

        void Enable(AtmosphereId id, bool enable) override;
        bool IsEnabled(AtmosphereId id) override;
        void SetSunDirection(AtmosphereId id, const Vector3& direction) override;
        void SetSunIlluminance(AtmosphereId id, float illuminance) override;
        void SetMinMaxSamples(AtmosphereId id, uint32_t minSamples, uint32_t maxSamples) override;
        void SetRaleighScattering(AtmosphereId id, const AZ::Vector3& scattering) override;
        void SetMieScattering(AtmosphereId id, const AZ::Vector3& scattering) override;
        void SetAbsorptionExtinction(AtmosphereId id, const AZ::Vector3& extinction) override;
        void SetGroundAlbedo(AtmosphereId id, const AZ::Vector3& albedo) override;
        void SetPlanetRadius(AtmosphereId id, float radius) override;
        void SetAtmosphereRadius(AtmosphereId id, float radius) override;

    private:

        //! RPI::SceneNotificationBus::Handler
        void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;
        void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
        void OnRenderPipelineRemoved(RPI::RenderPipeline* pipeline) override;
        
        void InitializeAtmosphere(AtmosphereId id);
            
        void CachePasses();
            
        SparseVector<SkyAtmospherePass::AtmosphereParams> m_params;
        AZStd::set<AtmosphereId> m_atmosphereIds;
        AZStd::vector<SkyAtmosphereParentPass*> m_skyAtmosphereParentPasses;

        bool m_passNeedsUpdate = true;
    };
}
