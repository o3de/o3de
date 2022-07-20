/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    using SourceHandle = SourceHandle;

    void SetRecentAssetId(SourceHandle assetId);
    void ClearRecentAssetId();
}
