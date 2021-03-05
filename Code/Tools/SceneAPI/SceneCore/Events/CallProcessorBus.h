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

#include <AzCore/std/containers/vector.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/EBus/EBus.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            class ICallContext
            {
            public:
                AZ_RTTI(ICallContext, "{525ED64B-9425-4F88-8E6B-D02FF61429B7}");
                virtual ~ICallContext() = 0;
            };

            class SCENE_CORE_CLASS CallProcessor
                : public AZ::EBusTraits
            {
            public:
                enum ProcessingPriority : uint8_t
                {
                    EarliestProcessing = 0,
                    EarlyProcessing = 64,
                    NormalProcessing = 128,
                    LateProcessing = 192,
                    LatestProcessing = 255
                };

                static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;
                using MutexType = AZStd::recursive_mutex;

                virtual ~CallProcessor() = 0;

                // Request to process the event for the given context.
                virtual ProcessingResult Process(ICallContext* context) = 0;

                // The order of the calling processors is undetermined, but sometimes a context needs to be
                //      processed before another. In these situations the priority of a processor can be 
                //      reduced or increased to make sure it gets called before or after normal processing
                //      has happened. Note that if two or more processors are raised to the same priority
                //      there will still not be a guarantee which will gets to do work first.
                SCENE_CORE_API virtual uint8_t GetPriority() const;

                SCENE_CORE_API bool Compare(const CallProcessor* rhs) const;
            };

            using CallProcessorBus = AZ::EBus<CallProcessor>;

            // Utility function to call the CallProcessor EBus.
            SCENE_CORE_API ProcessingResult Process(ICallContext& context);
            // Utility function to all the CallProcessor EBus.
            // Usage:
            //      Process<Context>(ContextArg1, ContextArg2, ContextArg3);
            template<typename Context, typename... Args>
            ProcessingResult Process(Args&&... args);

            inline ICallContext::~ICallContext()
            {
            }

            inline CallProcessor::~CallProcessor()
            {
            }
        } // namespace Events
    } // namespace SceneAPI
} // namespace AZ

#include <SceneAPI/SceneCore/Events/CallProcessorBus.inl>
