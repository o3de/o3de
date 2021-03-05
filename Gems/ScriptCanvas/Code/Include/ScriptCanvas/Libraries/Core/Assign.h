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

#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            class Assign
                : public Node
            {
            public:
                AZ_COMPONENT(Assign, "{E734ADCE-D822-4487-9681-5A80D8E4D263}", Node);

                static void Reflect(AZ::ReflectContext* reflectContext);

            protected:
                static const int k_sourceInputIndex = 0; 
                static const int k_targetSlotIndex = 3;

                Data::Type GetSlotDataType(const SlotId& slotId) const override;
                void OnInit() override;
                void OnInputSignal(const SlotId&) override;
                bool SlotAcceptsType(const SlotId&, const Data::Type&) const override;
                
            };
        }
    }
}