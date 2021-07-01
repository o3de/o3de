/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                AZ::Outcome<void, AZStd::string> SlotAcceptsType(const SlotId&, const Data::Type&) const override;
                
            };
        }
    }
}
