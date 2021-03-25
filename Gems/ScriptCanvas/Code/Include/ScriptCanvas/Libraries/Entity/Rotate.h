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

#include <ScriptCanvas/Core/Node.h>

#include <AzCore/Math/Vector3.h>

#include <Include/ScriptCanvas/Libraries/Entity/Rotate.generated.h>

// LY-103055 Temporary workaround to fix compile error when building Android NDK 19 and above due to AzCodeGenerator
// not receiving the correct clang system includes
namespace AZ
{
    class EntityId;
    class Vector3;
}

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Entity
        {
            //! Deprecated: see Entity Transform class' reflection of method "Rotate"
            class Rotate 
                : public Node
            {

            public:

                SCRIPTCANVAS_NODE(Rotate);

                void OnInputSignal(const SlotId&) override;

            };
        }
    }
}
