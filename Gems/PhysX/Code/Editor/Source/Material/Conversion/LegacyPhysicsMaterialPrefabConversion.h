/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Editor/Source/Material/Conversion/LegacyPhysicsMaterialAssetsConversion.h>

namespace PhysX
{
    void FixPrefabsWithPhysicsLegacyMaterials(const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap);
} // namespace PhysX
