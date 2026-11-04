/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// =============================================================================
// ${Name}Pass.h    (variant: PostVolume)
// -----------------------------------------------------------------------------
// FullscreenTrianglePass that pulls its per-frame uniforms from the active
// ${Name}SettingsComponent in the scene (via ${Name}SettingsProviderInterface).
//
// REGISTRATION CHAIN:
//   ${GemName}RenderingSystemComponent::Activate
//      -> AddPassCreator(AZ::Name("${Name}Pass"), &${Name}Pass::Create)
//   The settings component is added by the ARTIST in the level editor; no
//   code-side registration is needed beyond the pass registration above.
// =============================================================================

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>

namespace ${GemName}
{
    class ${Name}Pass
        : public AZ::RPI::FullscreenTrianglePass
    {
    public:
        AZ_RTTI(${GemName}::${SanitizedCppName}Pass, "{${Random_Uuid}}", AZ::RPI::FullscreenTrianglePass);
        AZ_CLASS_ALLOCATOR(${SanitizedCppName}Pass, AZ::SystemAllocator);

        static AZ::RPI::Ptr<${SanitizedCppName}Pass> Create(const AZ::RPI::PassDescriptor& descriptor);

        ~${SanitizedCppName}Pass() override = default;

    protected:
        explicit ${SanitizedCppName}Pass(const AZ::RPI::PassDescriptor& descriptor);

        void InitializeInternal() override;
        void FrameBeginInternal(FramePrepareParams params) override;

    private:
        // Cached SRG indices. Pair these with matching constants you add to
        // PassSrg in ${Name}.azsl, then push values from FrameBeginInternal
        // by reading ${Name}SettingsProviderInterface::Get()->GetSettings().
        //
        //   AZ::RHI::ShaderInputConstantIndex m_intensityIndex;
        //   AZ::RHI::ShaderInputConstantIndex m_tintIndex;
        //   AZ::RHI::ShaderInputConstantIndex m_exposureIndex;
    };

} // namespace ${GemName}
