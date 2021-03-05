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

#include <SceneAPI/SceneCore/Events/CallProcessorBinder.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            CallProcessorBinder::~CallProcessorBinder()
            {
                BusDisconnect();
            }

            ProcessingResult CallProcessorBinder::Process(ICallContext* context)
            {
                ProcessingResultCombiner result;
                for (auto& it : m_bindings)
                {
                    result += it->Process(this, context);
                }
                return result.GetResult();
            }

            void CallProcessorBinder::ActivateBindings()
            {
                CallProcessorBus::Handler::BusConnect();
            }

            void CallProcessorBinder::DeactivateBindings()
            {
                CallProcessorBus::Handler::BusDisconnect();
            }

            void CallProcessorBinder::ClearBindings()
            {
                m_bindings.clear();
            }

        } // namespace Events
    } // namespace SceneAPI
} // namespace AZ
