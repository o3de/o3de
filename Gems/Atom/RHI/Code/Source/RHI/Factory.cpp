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

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ResourceInvalidateBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Component/TickBus.h>

namespace AZ
{
    namespace RHI
    {
        uint32_t Factory::GetComponentService()
        {
            return AZ_CRC("RHIService", 0x45d8e053);
        }

        uint32_t Factory::GetManagerComponentService()
        {
            return AZ_CRC("RHIManagerService", 0x0849eda9);
        }

        uint32_t Factory::GetPlatformService()
        {
            return AZ_CRC("RHIPlatformService", 0xfff2cea4);
        }

        void Factory::Register(Factory* instance)
        {
            Interface<Factory>::Register(instance);

            ResourceInvalidateBus::AllowFunctionQueuing(true);

            // We delay the printf of which RHI we are going to use until the logging system is
            // up and running so the message is logged into the game/editor log file.
            AZStd::string rhiName = instance->GetName().GetCStr();
            auto logFunc = [rhiName]()
            {
                AZ_Printf("RHI", "****************************************************************\n");
                AZ_Printf("RHI", "                    Registering %s RHI                          \n", rhiName.c_str());
                AZ_Printf("RHI", "****************************************************************\n");
            };

            if (AZ::SystemTickBus::FindFirstHandler()) // resolving limitations in unittests
            {
                AZ::SystemTickBus::QueueFunction(logFunc);
            }
        }

        void Factory::Unregister(Factory* instance)
        {
            ResourceInvalidateBus::AllowFunctionQueuing(false);
            ResourceInvalidateBus::ClearQueuedEvents();

            Interface<Factory>::Unregister(instance);
        }

        bool Factory::IsReady()
        {
            return Interface<Factory>::Get() != nullptr;
        }

        Factory& Factory::Get()
        {
            Factory* factory = Interface<Factory>::Get();
            AZ_Assert(factory, "RHI::Factory is not connected to a platform. Call IsReady() to get the status of the platform. A null de-reference is imminent.");
            return *factory;
        }
    }
}
