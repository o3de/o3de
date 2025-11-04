/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompactArccosineNodeable.h"

namespace ScriptCanvas::Nodeables
{
    float CompactArccosineNodeable::In(float a)
    {
        AZ_Error("ScriptCanvas", a >= -1, "Attempted to get the arccosine of a number less than -1");
        AZ_Error("ScriptCanvas", a <= 1, "Attempted to get the arccosine of a number greater than 1");
        return acos(a);
    }
}
