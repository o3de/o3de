/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableNode.h>
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>

#include <AzCore/std/containers/vector.h>

#include <Source/Nodes/Nodeables/ValuePointerReferenceExample.generated.h>

namespace ScriptCanvasTesting
{
    namespace Nodeables
    {
        class ReturnTypeExample
            : public ScriptCanvas::Nodeable
        {
            SCRIPTCANVAS_NODE(ReturnTypeExample);

        private:
            AZStd::vector<ScriptCanvas::Data::NumberType> m_internalVector{ 1.0, 2.0, 3.0 };
        };

        class BranchInputTypeExample
            : public ScriptCanvas::Nodeable
        {
            SCRIPTCANVAS_NODE(BranchInputTypeExample);

        private:
            AZStd::vector<ScriptCanvas::Data::NumberType> m_internalVector{ 1.0, 2.0, 3.0 };
        };


        class InputTypeExample
            : public ScriptCanvas::Nodeable
        {
            SCRIPTCANVAS_NODE(InputTypeExample);
        };

        class PropertyExample
            : public ScriptCanvas::Nodeable
        {
            SCRIPTCANVAS_NODE(PropertyExample);
        };
    }
}
