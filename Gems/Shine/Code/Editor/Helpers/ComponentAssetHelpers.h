/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utility/pair.h>

namespace ComponentAssetHelpers
{
    using ComponentAssetPair = AZStd::pair<AZ::TypeId, AZ::Data::AssetId>;
    using ComponentAssetPairs = AZStd::vector<ComponentAssetPair>;
} // namespace ComponentAssetHelpers
