/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
