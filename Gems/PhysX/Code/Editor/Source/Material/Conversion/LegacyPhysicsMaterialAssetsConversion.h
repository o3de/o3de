/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Uuid.h>

namespace PhysX
{
    using LegacyMaterialIdToNewAssetIdMap = AZStd::unordered_map<AZ::Uuid, AZ::Data::AssetId>;
} // namespace PhysX
