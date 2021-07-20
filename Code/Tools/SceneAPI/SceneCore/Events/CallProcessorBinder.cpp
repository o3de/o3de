/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
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

            void CallProcessorBinder::Reflect(AZ::ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<CallProcessorBinder>()->Version(1);
                }
            }
        } // namespace Events
    } // namespace SceneAPI
} // namespace AZ
