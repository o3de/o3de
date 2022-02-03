/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        //! Reflects the profiler bus script bindings
        void ProfilerReflect(AZ::ReflectContext* context);
    } // namespace Debug
} // namespace AZ
