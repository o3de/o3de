/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#define AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED 1

// size of the sampler array in the SceneMaterialSrg
// Note: this must be the same value as the AZ_TRAITS_SCENE_MATERIALS_MAX_SAMPLERS define in the AzslcPlatformHeader.azsli
#define AZ_TRAITS_SCENE_MATERIALS_MAX_SAMPLERS 8
