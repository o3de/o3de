/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ResourceInvalidateBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Component/TickBus.h>

#if defined(USE_RENDERDOC)
#include <AzCore/Module/DynamicModuleHandle.h>
#include <Atom/RHI/RHIUtils.h>

static AZStd::unique_ptr<AZ::DynamicModuleHandle> s_renderDocModule;
static RENDERDOC_API_1_1_2* s_renderDocApi = nullptr;
#endif


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

        Factory::Factory()
        {
#if defined(USE_RENDERDOC)
            // If RenderDoc is requested, we need to load the library as early as possible (before device queries/factories are made)
            bool enableRenderDoc = RHI::QueryCommandLineOption("enableRenderDoc");

            if (enableRenderDoc && RENDERDOC_MODULE && !s_renderDocModule)
            {
                s_renderDocModule = DynamicModuleHandle::Create(RENDERDOC_MODULE);
                if (s_renderDocModule)
                {
                    if (s_renderDocModule->Load(false))
                    {
                        pRENDERDOC_GetAPI renderDocGetAPI = s_renderDocModule->GetFunction<pRENDERDOC_GetAPI>("RENDERDOC_GetAPI");
                        if (renderDocGetAPI)
                        {
                            if (!renderDocGetAPI(eRENDERDOC_API_Version_1_1_2, reinterpret_cast<void**>(&s_renderDocApi)))
                            {
                                s_renderDocApi = nullptr;
                            }
                        }

                        if (s_renderDocApi)
                        {
                            // Prevent RenderDoc from handling any exceptions that may interfere with the O3DE exception handler
                            s_renderDocApi->UnloadCrashHandler();
                        }
                        else
                        {
                            AZ_Printf("RHISystem", "RenderDoc module loaded but failed to retrieve API function pointer.\n");
                        }
                    }
                    else
                    {
                        AZ_Printf("RHISystem", "RenderDoc module requested but module failed to load.\n");
                    }
                }
            }
#endif // defined(USE_RENDERDOC)

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

#if defined(USE_RENDERDOC)
            if (s_renderDocModule)
            {
                s_renderDocModule->Unload();
            }
#endif
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

#if defined(USE_RENDERDOC)
        RENDERDOC_API_1_1_2* Factory::GetRenderDocAPI()
        {
            return s_renderDocApi;
        }
#endif
    }
}
