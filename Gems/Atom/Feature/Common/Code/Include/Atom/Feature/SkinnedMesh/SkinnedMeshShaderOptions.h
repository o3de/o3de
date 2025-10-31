/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <stdint.h>

namespace AZ
{
    namespace Render
    {
        enum class SkinningMethod : uint8_t
        {
            DualQuaternion = 0,
            LinearSkinning,
            NoSkinning
        };

        struct SkinnedMeshShaderOptions
        {
            SkinningMethod m_skinningMethod = SkinningMethod::LinearSkinning;
            bool m_applyMorphTargets = false;
        };
    }
}
