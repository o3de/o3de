/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorLerpNodeable.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        float CalculateLerpBetweenNodeableDuration(float speed, float difference)
        {
            return AZ::IsClose(speed, 0.0f, AZ::Constants::FloatEpsilon) ? -1.0f : fabsf(difference / speed);
        }
    }
}
