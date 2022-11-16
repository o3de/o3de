/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZ_MONOLITHIC_BUILD)

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneData/ManifestMetaInfoHandler.h>
#include <SceneAPI/SceneData/ReflectionRegistrar.h>
#include <SceneAPI/SceneData/Behaviors/Registry.h>

namespace AZ {
    namespace SceneAPI {
        namespace SceneData {

            static AZ::SceneAPI::SceneData::ManifestMetaInfoHandler* g_manifestMetaInfoHandler = nullptr;
            static AZ::SceneAPI::SceneData::Registry::ComponentDescriptorList g_componentDescriptors;
            static AZ::BehaviorContext* g_behaviorContext = nullptr;

            void Initialize()
            {
                if (!g_manifestMetaInfoHandler)
                {
                    g_manifestMetaInfoHandler = aznew AZ::SceneAPI::SceneData::ManifestMetaInfoHandler();
                }
            }

            void Reflect(AZ::SerializeContext* context)
            {
                if (!context)
                {
                    EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
                }
                if (context)
                {
                    AZ::SceneAPI::RegisterDataTypeReflection(context);
                }

                // Descriptor registration is done in Reflect instead of Initialize because the ResourceCompilerScene initializes the libraries before
                // there's an application.
                if (g_componentDescriptors.empty())
                {
                    AZ::SceneAPI::SceneData::Registry::RegisterComponents(g_componentDescriptors);
                    for (AZ::ComponentDescriptor* descriptor : g_componentDescriptors)
                    {
                        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Handler::RegisterComponentDescriptor, descriptor);
                    }
                }
            }

            void ReflectBehavior(AZ::BehaviorContext* context)
            {
                if (!g_behaviorContext)
                {
                    // Reflect instead of Initialize because ResourceCompilerScene initializes the libraries before there's an application
                    if (context)
                    {
                        g_behaviorContext = context;
                        AZ::SceneAPI::RegisterDataTypeBehaviorReflection(g_behaviorContext);
                    }
                }
            }

            void Activate()
            {
            }
            
            void Deactivate()
            {
            }

            void Uninitialize()
            {
                AZ::SerializeContext* context = nullptr;
                EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
                if (context)
                {
                    context->EnableRemoveReflection();
                    Reflect(context);
                    context->DisableRemoveReflection();
                    context->CleanupModuleGenericClassInfo();
                }

                if (!g_componentDescriptors.empty())
                {
                    for (AZ::ComponentDescriptor* descriptor : g_componentDescriptors)
                    {
                        descriptor->ReleaseDescriptor();
                    }
                    g_componentDescriptors.clear();
                    g_componentDescriptors.shrink_to_fit();
                }

                delete g_manifestMetaInfoHandler;
                g_manifestMetaInfoHandler = nullptr;
            }
        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ

static bool g_sceneDataInitialized = false;

extern "C" AZ_DLL_EXPORT void InitializeDynamicModule()
{
    if (g_sceneDataInitialized)
    {
        return;
    }
    g_sceneDataInitialized = true;

    AZ::SceneAPI::SceneData::Initialize();
}

extern "C" AZ_DLL_EXPORT void Reflect(AZ::SerializeContext* context)
{
    AZ::SceneAPI::SceneData::Reflect(context);
}

extern "C" AZ_DLL_EXPORT void ReflectBehavior(AZ::BehaviorContext * context)
{
    AZ::SceneAPI::SceneData::ReflectBehavior(context);
}

extern "C" AZ_DLL_EXPORT void CleanUpSceneDataGenericClassInfo()
{
    AZ::GetCurrentSerializeContextModule().Cleanup();
}

extern "C" AZ_DLL_EXPORT void UninitializeDynamicModule()
{
    if (!g_sceneDataInitialized)
    {
        return;
    }
    g_sceneDataInitialized = false;

    AZ::SceneAPI::SceneData::Uninitialize();

    // This module does not own these allocators, but must clear its cached EnvironmentVariables
    // because it is linked into other modules, and thus does not get unloaded from memory always
    if (AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }
    if (AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
    {
        AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
    }
}

#endif // !defined(AZ_MONOLITHIC_BUILD)
