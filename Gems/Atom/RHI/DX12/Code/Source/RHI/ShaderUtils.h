/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI.Reflect/DX12/ShaderStageFunction.h>

namespace AZ::DX12
{
    namespace ShaderUtils
    {
        ShaderByteCode PatchShaderFunction(
            const ShaderStageFunction& shaderFunction,
            const RHI::PipelineStateDescriptor& descriptor);

        ShaderByteCodeView PatchShaderFunction(
            const ShaderStageFunction& shaderFunction,
            const RHI::PipelineStateDescriptor& descriptor,
            AZStd::vector < ShaderByteCode>& patchedShaderContainer);

        bool SignByteCode(ShaderByteCode& bytecode);
    }
} // namespace AZ
