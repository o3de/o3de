/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace WhiteBox
{
    struct WhiteBoxRenderData;

    //! Encode render data as glTF 2.0, one primitive per material, with smoothed normals, UVs and paint as COLOR_0.
    //! Binary gives a .glb; otherwise a .gltf with its buffer embedded. Z-up becomes glTF's Y-up.
    AZStd::vector<AZ::u8> BuildGltf(const WhiteBoxRenderData& renderData, const AZStd::string& name, bool binary);

    //! Write BuildGltf's output; a path ending in .glb is written binary.
    bool SaveToGltf(const WhiteBoxRenderData& renderData, const AZStd::string& name, const AZStd::string& filePath, AZStd::string& error);
} // namespace WhiteBox
