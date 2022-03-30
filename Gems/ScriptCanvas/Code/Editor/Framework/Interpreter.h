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
    class Interpreter
    {
    public:
        AZ_TYPE_INFO(Interpreter, "{B77E5BC8-766A-4657-A30F-67797D04D10E}");
        AZ_CLASS_ALLOCATOR(Interpreter, AZ::SystemAllocator, 0);

        // will need a source handle version, with builder overrides
        // use the data system, use a different class for that, actually
        // the Interpreter

       


    private:
        ScriptCanvas::Executor m_executor;
    };
}
