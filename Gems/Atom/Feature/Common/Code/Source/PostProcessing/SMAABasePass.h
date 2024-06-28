/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/ScopeProducer.h>

#include <Atom/RPI.Reflect/Pass/PassName.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        //! SMAABasePass
        //! This pass contains data structure and method commonly used by SMAA passes implementation.
        class SMAABasePass
            : public AZ::RPI::FullscreenTrianglePass
        {
        public:
            AZ_RTTI(SMAABasePass, "{D879B4E8-DEDC-422D-950A-8B5341A8FD48}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(SMAABasePass, SystemAllocator);
            virtual ~SMAABasePass();

        protected:
            SMAABasePass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void InitializeInternal() override;

            // An interface to update pass srg.
            virtual void UpdateSRG() = 0;

            // An interface to get current shader variation option.
            virtual void GetCurrentShaderOption(AZ::RPI::ShaderOptionGroup& shaderOption) const = 0;

            void InvalidateShaderVariant();
            void InvalidateSRG();

            AZ::Vector4 CalculateRenderTargetMetrics(const RPI::PassAttachment* attachment);

            AZ::Vector4 m_renderTargetMetrics = AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f);

        private:

            void UpdateCurrentShaderVariant();

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            bool m_needToUpdateShaderVariant = false;
            bool m_needToUpdateSRG = true;
        };
    }   // namespace Render
}   // namespace AZ
