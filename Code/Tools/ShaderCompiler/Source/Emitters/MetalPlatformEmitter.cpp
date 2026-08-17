/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzslcEmitter.h>
#include "MetalPlatformEmitter.h"

namespace AZ::ShaderCompiler
{
    static constexpr char MetalPlatformEmitterName[] = "mt";
    static const PlatformEmitter* s_metalPlatformEmitter = MetalPlatformEmitter::RegisterPlatformEmitter();

    const PlatformEmitter* MetalPlatformEmitter::RegisterPlatformEmitter() noexcept(false)
    {
        static MetalPlatformEmitter platformEmitter; // Static linkage, will be destroyed

        static bool alreadyRegistered = false;
        if (!alreadyRegistered)
        {
            PlatformEmitter::SetEmitter(MetalPlatformEmitterName, &platformEmitter);
            alreadyRegistered = true;
        }

        return &platformEmitter;
    }

    std::string MetalPlatformEmitter::GetRootConstantsView(const CodeEmitter& codeEmitter, const RootSigDesc& rootSig, const Options& options, BindingPair::Set signatureQuery) const
    {
        std::stringstream strOut;

        strOut << "[[vk::push_constant]]\n";
        const auto& structUid = codeEmitter.GetIR()->m_rootConstantStructUID;
        const auto& bindInfo = rootSig.Get(structUid);
        assert(structUid == bindInfo.m_uid);
        const auto& rootCBForEmission = codeEmitter.GetTranslatedName(RootConstantsViewName, UsageContext::DeclarationSite);
        const auto& rootConstClassForEmission = codeEmitter.GetTranslatedName(structUid.GetName(), UsageContext::ReferenceSite);
        const auto& spaceX = ", space" + std::to_string(bindInfo.m_registerBinding.m_pair[signatureQuery].m_logicalSpace);
        strOut << "ConstantBuffer<" << rootConstClassForEmission << "> " << rootCBForEmission << " : register(b" << bindInfo.m_registerBinding.m_pair[signatureQuery].m_registerIndex << spaceX << ");\n\n";
        return strOut.str();
    }

    uint32_t MetalPlatformEmitter::AlignRootConstants(uint32_t size) const
    {
        return Packing::AlignUp(size, Packing::s_bytesPerRegister);
    }

    SubpassInputSupportFlag MetalPlatformEmitter::GetSubpassInputSupport() const
    {
        // Metal doesn't support reading from a depth/stencil attachment, only color.
        return SubpassInputSupportFlag::Color;
    }
}
