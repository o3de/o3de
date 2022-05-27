/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

namespace PhysX
{
    // Reflection of legacy Physics material classes.
    // Used when converting old material asset to new one.
    void ReflectLegacyMaterialClasses(AZ::ReflectContext* context);

    AZStd::optional<AZStd::string> GetFullSourceAssetPathById(AZ::Data::AssetId assetId);
} // namespace PhysX
