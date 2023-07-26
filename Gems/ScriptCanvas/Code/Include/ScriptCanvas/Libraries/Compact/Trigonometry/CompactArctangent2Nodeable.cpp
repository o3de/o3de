/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompactArctangent2Nodeable.h"

namespace ScriptCanvas::Nodeables
{
    float CompactArctangent2Nodeable::In(float y, float x)
    {
        return atan2(y, x);
    }
}
