/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/SkyAtmosphere/SkyAtmosphereFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/SparseVector.h>

namespace AZ::Render
{
    class SkyAtmosphereParentPass;

    //! This feature processor manages drawing sky atmospheres.
    //! Use CreateAtmosphere() to create a new atmosphere
    class SkyAtmosphereFeatureProcessor final
        : public SkyAtmosphereFeatureProcessorInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(SkyAtmosphereFeatureProcessor, AZ::SystemAllocator)

        AZ_RTTI(AZ::Render::SkyAtmosphereFeatureProcessor, "{FB3155E9-BA3C-487B-B251-EB4BF3465E02}", AZ::Render::SkyAtmosphereFeatureProcessorInterface);

        static void Reflect(AZ::ReflectContext* context);

        SkyAtmosphereFeatureProcessor() = default;
        virtual ~SkyAtmosphereFeatureProcessor() = default;

        //! FeatureProcessor 
        void Activate() override;
        void Deactivate() override;
        void AddRenderPasses(RPI::RenderPipeline* renderPipeline) override;
        void Render(const RenderPacket& packet) override;

        //! SkyAtmosphereFeatureProcessorInterface
        AtmosphereId CreateAtmosphere() override;
        void ReleaseAtmosphere(AtmosphereId id) override;
        void SetAtmosphereParams(AtmosphereId id, const SkyAtmosphereParams& params) override;
        void SetAtmosphereEnabled(AtmosphereId id, bool enabled) override;
        bool GetAtmosphereEnabled(AtmosphereId id) override;

    private:

        //! RPI::SceneNotificationBus::Handler
        void OnRenderPipelineChanged(AZ::RPI::RenderPipeline* pipeline, RPI::SceneNotification::RenderPipelineChangeType changeType) override;
        
        void InitializeAtmosphere(AtmosphereId id);
        void UpdateBackgroundClearColor();
        bool HasValidAtmosphere();
            
        struct SkyAtmosphere
        {
            AtmosphereId m_id;
            SkyAtmosphereParams m_params;
            bool m_passNeedsUpdate = false;
            bool m_enabled = false;
        };

        SparseVector<SkyAtmosphere> m_atmospheres;
        AZStd::map<RPI::RenderPipeline*, AZStd::vector<SkyAtmosphereParentPass*>> m_renderPipelineToSkyAtmosphereParentPasses;
    };
}
