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

#include "AddFailure.h"

#include "UnitTestBus.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace UnitTesting
        {
            void AddFailure::OnInputSignal([[maybe_unused]] const SlotId& slotId)
            {
                const auto report = FindDatum(GetSlotId("Report"))->GetAs<Data::StringType>();
                ScriptCanvas::UnitTesting::Bus::Event
                ( GetOwningScriptCanvasId()
                    , &ScriptCanvas::UnitTesting::BusTraits::AddFailure
                    , *report);

                SignalOutput(GetSlotId("Out"));
            }
        } // namespace UnitTesting
    } // namespace Nodes
} // namespace ScriptCanvas
