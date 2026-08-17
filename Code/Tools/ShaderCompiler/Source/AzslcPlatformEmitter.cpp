/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <mutex>

#include "AzslcEmitter.h"
#include "AzslcPlatformEmitter.h"
#include "Emitters/DirectX12PlatformEmitter.h"
#include "Emitters/MetalPlatformEmitter.h"
#include "Emitters/VulkanPlatformEmitter.h"

namespace AZ::ShaderCompiler
{
    namespace
    {
        void RegisterPlatformEmitters()
        {
            (void)DirectX12PlatformEmitter::RegisterPlatformEmitter();
            (void)MetalPlatformEmitter::RegisterPlatformEmitter();
            (void)VulkanPlatformEmitter::RegisterPlatformEmitter();
        }
    }

    const PlatformEmitter* PlatformEmitter::GetDefaultEmitter() noexcept(true)
    {
        // The default platform emitter is never registered and will never be a result in PlatformEmitter::GetEmitter()
        static PlatformEmitter platformEmitter; // Static linkage, will be destroyed
        return &platformEmitter;
    }

    unordered_map<string, const PlatformEmitter*>* s_emitters = nullptr;
    std::mutex emitterListMutex;

    const PlatformEmitter* PlatformEmitter::GetEmitter(const string& key) noexcept(true)
    {
        RegisterPlatformEmitters();
        std::lock_guard<std::mutex> lock(emitterListMutex);

        try
        {
            return (*s_emitters)[key];            
        }
        catch (...)
        {   // We should never return a default emitter here. This method searches by name only
            return nullptr;
        }        
    }

    void PlatformEmitter::SetEmitter(const string& key, const PlatformEmitter* const platformEmitter) noexcept(false)
    {
        std::lock_guard<std::mutex> lock(emitterListMutex);

        if (!s_emitters)
        {
            s_emitters = new unordered_map<string, const PlatformEmitter*>();
        }

        if (s_emitters->find(key) != s_emitters->end())
        {
            throw std::runtime_error{ "PlatformEmitter::RegisterEmitter cannot register two platforms with the same key!" };            
        }
        s_emitters->try_emplace(key, platformEmitter);
    }

    // Virtual methods to be overridden

    string PlatformEmitter::GetRootSig(const CodeEmitter&, const RootSigDesc&, const Options&, BindingPair::Set) const
    {
        // The default implementation of most emission methods does nothing
        return "";
    }

    string PlatformEmitter::GetRootConstantsView(const CodeEmitter& codeEmitter, const RootSigDesc& rootSig, const Options& options, BindingPair::Set signatureQuery) const
    {
        std::stringstream strOut;

        const auto& structUid = codeEmitter.GetIR()->m_rootConstantStructUID;
        const auto& bindInfo = rootSig.Get(structUid);
        assert(structUid == bindInfo.m_uid);
        const auto& rootCBForEmission = codeEmitter.GetTranslatedName(RootConstantsViewName, UsageContext::DeclarationSite);
        const auto& rootConstClassForEmission = codeEmitter.GetTranslatedName(structUid.GetName(), UsageContext::ReferenceSite);
        const auto& spaceX = ", space" + std::to_string(bindInfo.m_registerBinding.m_pair[signatureQuery].m_logicalSpace);
        strOut << "ConstantBuffer<" << rootConstClassForEmission << "> " << rootCBForEmission << " : register(b" << bindInfo.m_registerBinding.m_pair[signatureQuery].m_registerIndex << spaceX << ");\n\n";

        return strOut.str();
    }

    std::pair<string, string> PlatformEmitter::GetDataViewHeaderFooter(
        const CodeEmitter& codeEmitter,
        const IdentifierUID& symbol,
        uint32_t bindInfoRegisterIndex, 
        string_view registerTypeLetter, 
        optional<string> stringifiedLogicalSpace,
        const Options& options) const
    {
        // in the general case, we output normal HLSL `var decl : register(b0, space0);`
        // no special header, but the post colon part is the footer
        string bindingSpaceStringlet { stringifiedLogicalSpace ? ", space" + *stringifiedLogicalSpace : "" };
        string footer {ConcatString(" : register(", registerTypeLetter, bindInfoRegisterIndex, bindingSpaceStringlet, ")")};
        return { {}, footer };
    }

    uint32_t PlatformEmitter::AlignRootConstants(uint32_t size) const
    {
        return size;
    }

    string PlatformEmitter::GetSpecializationConstant(const CodeEmitter& codeEmitter, const IdentifierUID& symbol, const Options& options) const
    {
        return "";
    }

    SubpassInputSupportFlag PlatformEmitter::GetSubpassInputSupport() const
    {
        return SubpassInputSupportFlag::None;
    }
}
