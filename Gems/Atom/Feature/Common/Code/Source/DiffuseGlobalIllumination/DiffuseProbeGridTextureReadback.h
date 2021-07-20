/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/Feature/DiffuseGlobalIllumination/DiffuseProbeGridFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseProbeGrid;

        enum class DiffuseProbeGridReadbackState
        {
            Idle,
            Irradiance,
            Distance,
            Relocation,
            Classification,
            Complete
        };

        //! This class contains functionality necessary to read back the DiffuseProbeGrid textures, which
        //! allows them to be saved as assets to run the DiffuseProbeGrid in non-realtime mode.
        class DiffuseProbeGridTextureReadback final
        {
        public:
            DiffuseProbeGridTextureReadback(DiffuseProbeGrid* diffuseProbeGrid);
            ~DiffuseProbeGridTextureReadback() = default;

            void BeginTextureReadback(DiffuseProbeGridBakeTexturesCallback callback);
            void Update(const AZ::Name& passName);
            void FrameBegin(AZ::RPI::Pass::FramePrepareParams& params);

        private:

            DiffuseProbeGrid* m_diffuseProbeGrid = nullptr;
            DiffuseProbeGridReadbackState m_readbackState = DiffuseProbeGridReadbackState::Idle;
            AZStd::shared_ptr<AZ::RPI::AttachmentReadback> m_attachmentReadback;
            DiffuseProbeGridBakeTexturesCallback m_callback;

            AZ::RPI::AttachmentReadback::ReadbackResult m_irradianceReadbackResult;
            AZ::RPI::AttachmentReadback::ReadbackResult m_distanceReadbackResult;
            AZ::RPI::AttachmentReadback::ReadbackResult m_relocationReadbackResult;
            AZ::RPI::AttachmentReadback::ReadbackResult m_classificationReadbackResult;
        };
    }   // namespace Render
}   // namespace AZ
