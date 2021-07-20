/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ
{
    class ReflectContext;

    namespace Debug
    {
        /**
         * Reflect trace events(errors, warnings, asserts, etc)
         */
        void TraceReflect(AZ::ReflectContext* context);
    }
}
