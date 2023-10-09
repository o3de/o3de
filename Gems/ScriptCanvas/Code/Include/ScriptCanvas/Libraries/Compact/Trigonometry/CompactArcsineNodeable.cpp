/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompactArcsineNodeable.h"

namespace ScriptCanvas::Nodeables
{
    float CompactArcsineNodeable::In(float a)
    {
        AZ_Error("ScriptCanvas", a >= -1, "Attempted to get the arcsine of a number less than -1");
        AZ_Error("ScriptCanvas", a <= 1, "Attempted to get the arcsine of a number greater than 1");
        return asin(a);
    }
}
