/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzslcEmitter.h>
#include "VulkanPlatformEmitter.h"

namespace AZ::ShaderCompiler
{
    static constexpr char VulkanPlatformEmitterName[] = "vk";
    static const PlatformEmitter* s_vulkanPlatformEmitter = VulkanPlatformEmitter::RegisterPlatformEmitter();

    const PlatformEmitter* VulkanPlatformEmitter::RegisterPlatformEmitter() noexcept(false)
    {
        static VulkanPlatformEmitter platformEmitter; // Static linkage, will be destroyed

        static bool alreadyRegistered = false;
        if (!alreadyRegistered)
        {
            PlatformEmitter::SetEmitter(VulkanPlatformEmitterName, &platformEmitter);
            alreadyRegistered = true;
        }

        return &platformEmitter;
    }

    string VulkanPlatformEmitter::GetRootConstantsView(const CodeEmitter& codeEmitter, const RootSigDesc&, const Options&, BindingPair::Set) const
    {
        std::stringstream strOut;

        const auto& structUid = codeEmitter.GetIR()->m_rootConstantStructUID;
        const auto& rootCBForEmission = codeEmitter.GetTranslatedName(RootConstantsViewName, UsageContext::DeclarationSite);
        strOut << "[[vk::push_constant]]\n";
        strOut << UnmangleTrimedName(structUid.GetName()) << " " << rootCBForEmission << ";\n\n";

        return strOut.str();        
    }

    SubpassInputSupportFlag VulkanPlatformEmitter::GetSubpassInputSupport() const
    {
        return SubpassInputSupportFlag::All;
    }
}
