/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <SkyAtmosphere/SkyAtmospherePass.h>
#include <AzCore/Math/Vector3.h>

namespace AZ::Render
{
    static const char* const SkyAtmosphereParentPassTemplateName = "SkyAtmosphereParentPassTemplate";

    class SkyAtmosphereParentPass final
        : public RPI::ParentPass
    {
        using Base = RPI::ParentPass;
        AZ_RPI_PASS(SkyAtmosphereParentPass);

    public:
        AZ_RTTI(SkyAtmosphereParentPass, "{3FF065BD-67B6-4D46-9589-BFAF6364D4ED}", Base);
        AZ_CLASS_ALLOCATOR(SkyAtmosphereParentPass, SystemAllocator, 0);

        virtual ~SkyAtmosphereParentPass() = default;

        static RPI::Ptr<SkyAtmosphereParentPass> Create(const RPI::PassDescriptor& descriptor);

        void CreateAtmospherePass(SkyAtmosphereFeatureProcessorInterface::AtmosphereId id);
        void ReleaseAtmospherePass(SkyAtmosphereFeatureProcessorInterface::AtmosphereId id);
        void UpdateAtmospherePassSRG(SkyAtmosphereFeatureProcessorInterface::AtmosphereId id, const SkyAtmospherePass::AtmosphereParams& params);

    private:
        SkyAtmosphereParentPass() = delete;
        explicit SkyAtmosphereParentPass(const RPI::PassDescriptor& descriptor);

        RPI::Ptr<SkyAtmospherePass> GetPass(SkyAtmosphereFeatureProcessorInterface::AtmosphereId id) const;
    };
} // namespace AZ::Render
