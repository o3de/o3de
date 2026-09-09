/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// No ShaderPlatformInterface a tool can construct here -- see MaterialCanvas_Traits_Windows.h and
// shader_dependencies_linux.cmake, which links nothing for this reason. The preview still builds through the Asset Processor.
#define AZ_TRAIT_MATERIALCANVAS_IN_MEMORY_SHADER_COMPILATION_SUPPORTED 0
