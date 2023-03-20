/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Primitives.h"
#include "PrimitivesExecution.h"

namespace ScriptCanvas
{
    namespace Grammar
    {
        void MarkUserFunctionCallLocallyDefined(ExecutionTreePtr execution);
    }
}
