/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompactSquareNodeable.h"

namespace ScriptCanvas::Nodeables
{
    float CompactSquareNodeable::In(float a)
    {
        return pow(a, 2.0f);
    }
}
