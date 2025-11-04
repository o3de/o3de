/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <ScreenSpace/DeferredFogSettings.h>

namespace AZ
{
    namespace Render
    {
        static const char* const DeferredFogPassTemplateName = "DeferredFogPassTemplate";

        // Deferred screen space fog pass class.  The fog is calculated post
        // the main render using the linear depth and turbulence texture with two blended
        // octaves that emulate the fog thickness and fog animation along the view ray direction.
        // The fog can be a full screen fog or a thin 3D layer fog representing mountains morning mist.
        // The pass also exposes the fog settings to be used by an editor component node that will
        // control the visual properties of the fog.
        // Enhancements of this fog can contain more advanced noise handling (real volumetric), areal
        // mask, blending between areal fog nodes and other enhancements required for production.
        class DeferredFogPass final
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(DeferredFogPass);

        public:
            AZ_RTTI(DeferredFogPass, "{0406C8AB-E95D-43A7-AF53-BDEE22D36746}", RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(DeferredFogPass, SystemAllocator);
            ~DeferredFogPass() = default;


            static RPI::Ptr<DeferredFogPass> Create(const RPI::PassDescriptor& descriptor);

            DeferredFogSettings* GetPassFogSettings();

            virtual bool IsEnabled() const override;

        protected:
            DeferredFogPass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void InitializeInternal() override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;

            //! Set the binding indices of all members of the SRG
            void SetSrgBindIndices();

            //! Bind SRG constants - done via macro reflection
            void SetSrgConstants();

            //! Check if the pass should be enabled or disabled 
            void UpdateEnable(DeferredFogSettings* fogSettings);

            void UpdateShaderOptions();

        private:
            // When a component is not present we want to fall back to the default settings and
            // actively pass them to the shader.
            DeferredFogSettings m_fallbackSettings;

            // Fog mode option name
            const AZ::Name m_fogModeOptionName;

            // Shader input constant index for the depth texture dimensions.
            RHI::ShaderInputConstantIndex m_depthTextureDimensionsIndex;
        };       
    }   // namespace Render
}   // namespace AZ
