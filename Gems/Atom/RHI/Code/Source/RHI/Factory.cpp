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

#if defined(USE_RENDERDOC) || defined(USE_PIX)
#include <AzCore/Module/DynamicModuleHandle.h>
#include <Atom_RHI_Traits_Platform.h>
#endif

#if defined(USE_RENDERDOC)
static AZStd::unique_ptr<AZ::DynamicModuleHandle> s_renderDocModule;
static RENDERDOC_API_1_1_2* s_renderDocApi = nullptr;
static bool s_isRenderDocDllLoaded = false;
#endif

#if defined(USE_PIX)
static AZStd::unique_ptr<AZ::DynamicModuleHandle> s_pixModule;
static bool s_isPixGpuCaptureDllLoaded = false;
static bool s_pixGpuMarkersEnabled = false;
#endif

static bool s_usingWarpDevice = false;

namespace AZ
{
    namespace RHI
    {
        namespace Platform
        {
            bool IsPixDllInjected(const char* dllName);
            AZStd::wstring GetLatestWinPixGpuCapturerPath();
        }

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
            AZStd::string preferredUserAdapterName = RHI::GetCommandLineValue("forceAdapter");
            s_usingWarpDevice = preferredUserAdapterName == "Microsoft Basic Render Driver";
#if defined(USE_RENDERDOC)
            // If RenderDoc is requested, we need to load the library as early as possible (before device queries/factories are made)
            bool enableRenderDoc = RHI::QueryCommandLineOption("enableRenderDoc");
#if defined(USE_PIX)
            s_pixGpuMarkersEnabled = s_pixGpuMarkersEnabled || enableRenderDoc;
#endif
            if (enableRenderDoc && AZ_TRAIT_RENDERDOC_MODULE && !s_renderDocModule)
            {
                s_renderDocModule = DynamicModuleHandle::Create(AZ_TRAIT_RENDERDOC_MODULE);
                if (s_renderDocModule)
                {
                    if (s_renderDocModule->Load(false))
                    {
                        s_isRenderDocDllLoaded = true;
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
#endif

#if defined(USE_PIX)
            // If GPU capture is requested, we need to load the pix library as early as possible (before device queries/factories are made)
            bool enablePixGPU = RHI::QueryCommandLineOption("enablePixGPU");
            if (enablePixGPU && AZ_TRAIT_PIX_MODULE && !s_pixModule)
            {
                //Get the path to the latest pix install directory
                AZStd::wstring pixGpuDllPath = Platform::GetLatestWinPixGpuCapturerPath();
                AZStd::string dllPath;
                AZStd::to_string(dllPath, pixGpuDllPath);
                s_pixModule = DynamicModuleHandle::Create(dllPath.c_str());
                if (s_pixModule)
                {
                    if (!s_pixModule->Load(false))
                    {
                        AZ_Printf("RHISystem", "Pix capture requested but module failed to load.\n");
                    }
                }
            }

            //Pix dll can still be injected even if we do not pass in enablePixGPU. This can be done if we launch the app from Pix.
            s_isPixGpuCaptureDllLoaded = Platform::IsPixDllInjected(AZ_TRAIT_PIX_MODULE);

            s_pixGpuMarkersEnabled = s_pixGpuMarkersEnabled || RHI::QueryCommandLineOption("enablePixGpuMarkers");
#endif
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
#if defined(USE_PIX)
            if (s_pixModule)
            {
                s_pixModule->Unload();
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

        bool Factory::IsRenderDocModuleLoaded()
        {
#if defined(USE_RENDERDOC)
            return s_isRenderDocDllLoaded;
#else
            return false;
#endif
        }

        bool Factory::IsPixModuleLoaded()
        {
#if defined(USE_PIX)
            return s_isPixGpuCaptureDllLoaded;
#else
            return false;
#endif
        }

        bool Factory::PixGpuEventsEnabled()
        {
#if defined(USE_PIX)
            return s_pixGpuMarkersEnabled;
#else
            return false;
#endif
        }

        bool Factory::UsingWarpDevice()
        {
            return s_usingWarpDevice;
        }
    }
}
