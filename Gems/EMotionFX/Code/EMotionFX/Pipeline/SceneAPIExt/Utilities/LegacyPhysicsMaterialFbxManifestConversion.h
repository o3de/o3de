/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/Physics/Material/Legacy/LegacyPhysicsMaterialConversionUtils.h>

namespace EMotionFX::Pipeline::Utilities
{
    void FixFbxManifestsWithPhysicsLegacyMaterials(const Physics::Utils::LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap);
} // namespace EMotionFX::Pipeline::Utilities
