/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompactDivideNodeable.h"

namespace ScriptCanvas::Nodeables
{
    float CompactDivideNodeable::In(float a, float b)
    {
        AZ_Error("ScriptCanvas", b != 0, "Attempted to divide by zero");
        return a / b;
    }
}
