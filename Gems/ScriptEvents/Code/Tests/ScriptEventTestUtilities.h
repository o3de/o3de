/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    class ReflectContext;
}

namespace ScriptEventsTests
{
    namespace Utilities
    {
        void Reflect(AZ::ReflectContext* context);

        void ScriptExpectTrue(bool check, const char* msg);
        void ScriptTrace(const char* txt);
    }
}
