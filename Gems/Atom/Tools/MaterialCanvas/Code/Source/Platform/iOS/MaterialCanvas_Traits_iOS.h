/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// Material Canvas itself is not built here (PAL_TRAIT_ATOM_MATERIAL_CANVAS_APPLICATION_SUPPORTED is FALSE), so this is only
// here to keep the trait defined on every platform. See MaterialCanvas_Traits_Windows.h.
#define AZ_TRAIT_MATERIALCANVAS_IN_MEMORY_SHADER_COMPILATION_SUPPORTED 0
