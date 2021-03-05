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

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            class Error
                : public Node
            {
            public:
                AZ_COMPONENT(Error, "{C6928F30-87BA-4FFE-A3C0-B6096C161DD0}", Node);
                
                static void Reflect(AZ::ReflectContext* reflection);
              
            protected:
                void OnInit() override;
                void OnInputSignal(const SlotId&) override;
            };
        } // namespace Core
    } // namespace Nodes
} // namespace ScriptCanvas