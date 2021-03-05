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

#include <ScriptCanvas/CodeGen/CodeGen.h>
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
            class Rotate 
                : public Node
            {            
                ScriptCanvas_Node(Rotate,
                    ScriptCanvas_Node::Uuid("{6F802B53-E726-430D-9F41-63CFC783F91D}")
                    ScriptCanvas_Node::Description("Rotates the given entity by the specified amount.")
                    ScriptCanvas_Node::Category("Entity/Transform")
                    ScriptCanvas_Node::EditAttributes(AZ::Edit::Attributes::CategoryStyle(".method"), ScriptCanvas::Attributes::Node::TitlePaletteOverride("MethodNodeTitlePalette"))
                    ScriptCanvas_Node::Version(4)
                );

            public:
                
                void OnInputSignal(const SlotId&) override;

            protected:
                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", ""));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", ""));

                // Data Inputs
                ScriptCanvas_PropertyWithDefaults(AZ::EntityId, ScriptCanvas::GraphOwnerId,
                    ScriptCanvas_Property::Name("Entity", "The entity to apply the rotation on.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_Property(AZ::Vector3,
                    ScriptCanvas_Property::Name("Euler Angles", "Euler angles, Pitch/Yaw/Roll.")
                    ScriptCanvas_Property::Input
                );
            };
        }
    }
}