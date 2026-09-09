/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// Whether Material Canvas can compile a preview shader in process instead of waiting for the Asset Processor. It needs a
// ShaderPlatformInterface to drive, and the only one a tool can construct is DX12's, which is behind
// PAL_TRAIT_ATOM_RHI_DX12_SUPPORTED. shader_dependencies_<platform>.cmake links the library this trait speaks for, so the two
// have to agree: where the trait is 0 the library is not linked, and CreateInMemoryShaderAsset reports itself unavailable.
#define AZ_TRAIT_MATERIALCANVAS_IN_MEMORY_SHADER_COMPILATION_SUPPORTED 1
