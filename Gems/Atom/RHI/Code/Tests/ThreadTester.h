/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once 

#include <AzCore/std/functional.h>

namespace UnitTest
{
    class ThreadTester
    {
    public:
        using ThreadFunction = AZStd::function<void(size_t)>;

        static void Dispatch(size_t threadCountMax, ThreadFunction threadFunction);
    };
}
