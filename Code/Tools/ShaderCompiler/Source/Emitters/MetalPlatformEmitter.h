/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzslcPlatformEmitter.h>
#include "CommonVulkanPlatformEmitter.h"

namespace AZ::ShaderCompiler
{
    // PlatformEmitter is not a Backend by design. It's a supplement to CodeEmitter, not a replacement
    struct MetalPlatformEmitter : CommonVulkanPlatformEmitter
    {
    public:
        //! This method will be called once and only once when the platform emitter registers itself to the system.
        //! Returns a singleton object of this class.
        static const PlatformEmitter* RegisterPlatformEmitter() noexcept(false);

        [[nodiscard]]
        std::string GetRootConstantsView(const CodeEmitter& codeEmitter, const RootSigDesc& rootSig, const Options& options, BindingPair::Set signatureQuery) const override final;

        uint32_t AlignRootConstants(uint32_t size) const override final;

        SubpassInputSupportFlag GetSubpassInputSupport() const override;

    private:
        MetalPlatformEmitter() : CommonVulkanPlatformEmitter {} {};
    };
}
