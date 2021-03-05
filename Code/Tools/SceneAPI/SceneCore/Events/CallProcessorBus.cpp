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

#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            uint8_t CallProcessor::GetPriority() const
            {
                return CallProcessor::NormalProcessing;
            }

            bool CallProcessor::Compare(const CallProcessor* rhs) const
            {
                AZ_Assert(rhs, "Invalid argument for ProcessingEvents::Compare.");
                return GetPriority() < rhs->GetPriority();
            }

            ProcessingResult Process(ICallContext& context)
            {
                ProcessingResultCombiner result;
                CallProcessorBus::BroadcastResult(result, &CallProcessorBus::Events::Process, &context);
                return result.GetResult();
            }
        } // namespace Events
    } // namespace SceneAPI
} // namespace AZ