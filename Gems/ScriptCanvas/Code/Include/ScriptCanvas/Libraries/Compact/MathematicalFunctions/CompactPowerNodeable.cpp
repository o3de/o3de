/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompactPowerNodeable.h"

namespace ScriptCanvas::Nodeables
{
    float CompactPowerNodeable::In(float a, float b)
    {
        return pow(a, b);
    }
}
