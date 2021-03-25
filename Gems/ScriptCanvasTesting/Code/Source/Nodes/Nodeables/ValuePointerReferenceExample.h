/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    }
}
