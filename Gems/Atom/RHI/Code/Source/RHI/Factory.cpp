/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ResourceInvalidateBus.h>
#include <Atom/RHI/RHIUtils.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Component/TickBus.h>

static bool s_usingWarpDevice = false;
AZ_CVAR(bool, r_gpuMarkersMergeGroups, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Enable merging of gpu markers in order to track payload (i.e all the scopes) per command list.");

AZ_CVAR(
    bool,
    r_enablePsoCaching,
    false,
    nullptr,
    AZ::ConsoleFunctorFlags::DontReplicate,
    "If true the active RHI backend will try to write out PSO cache (as long as it is able to). By default it is false.");


namespace AZ::RHI
{
    uint32_t Factory::GetComponentService()
    {
        return AZ_CRC_CE("RHIService");
    }

    uint32_t Factory::GetManagerComponentService()
    {
        return AZ_CRC_CE("RHIManagerService");
    }

    uint32_t Factory::GetPlatformService()
    {
        return AZ_CRC_CE("RHIPlatformService");
    }

    Factory::Factory()
    {
        AZStd::string preferredUserAdapterName = RHI::GetCommandLineValue("forceAdapter");
        s_usingWarpDevice = preferredUserAdapterName == "Microsoft Basic Render Driver";
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

    bool Factory::UsingWarpDevice()
    {
        return s_usingWarpDevice;
    }
}
