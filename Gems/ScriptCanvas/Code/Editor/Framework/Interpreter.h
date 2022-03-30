/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Execution/Executor.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvasEditor
{
    class Interpreter final
    {
    public:
        AZ_TYPE_INFO(Interpreter, "{B77E5BC8-766A-4657-A30F-67797D04D10E}");
        AZ_CLASS_ALLOCATOR(Interpreter, AZ::SystemAllocator, 0);



        // will need a source handle version, with builder overrides
        // use the data system, use a different class for that, actually
        // the Interpreter

        // \todo consider making a template version with return values, similar to execution out
        // or perhaps safety checked versions with an array / table of any. something parsable
        // or consider just having users make ebuses that the graphs will handle
        // and wrapping the whole thing in a single class
        // interpreter + ebus, and calling it EZ SC Hook or something like that

       


    private:
        // all calls to the executor are safety checked!
        ScriptCanvas::Executor m_executor;
    };
}
