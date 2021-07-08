/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Source/Viewport/InputController/Behavior.h>

namespace MaterialEditor
{
    //! No action taken
    class IdleBehavior final
        : public Behavior
    {
    public:
        IdleBehavior() = default;
        virtual ~IdleBehavior() = default;
    };
} // namespace MaterialEditor
