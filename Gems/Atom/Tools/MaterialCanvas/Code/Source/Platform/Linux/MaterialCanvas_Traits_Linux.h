/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// No tool-constructible ShaderPlatformInterface here, so the preview builds through the Asset Processor (see the Windows traits).
#define AZ_TRAIT_MATERIALCANVAS_IN_MEMORY_SHADER_COMPILATION_SUPPORTED 0
