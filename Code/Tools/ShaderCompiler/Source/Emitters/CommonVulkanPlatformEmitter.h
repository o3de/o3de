/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzslcPlatformEmitter.h>

namespace AZ::ShaderCompiler
{
    // PlatformEmitter is not a Backend by design. It's a supplement to CodeEmitter, not a replacement
    struct CommonVulkanPlatformEmitter : PlatformEmitter
    {
    public:
        [[nodiscard]]
        string GetSpecializationConstant(const CodeEmitter& codeEmitter, const IdentifierUID& symbol, const Options& options) const override;

        [[nodiscard]]
        std::pair<string, string> GetDataViewHeaderFooter(
            const CodeEmitter& codeEmitter,
            const IdentifierUID& symbol,
            uint32_t bindInfoRegisterIndex,
            string_view registerTypeLetter,
            optional<string> stringifiedLogicalSpace,
            const Options& options) const override;

    protected:
        CommonVulkanPlatformEmitter() : PlatformEmitter {} {};
    };
}
