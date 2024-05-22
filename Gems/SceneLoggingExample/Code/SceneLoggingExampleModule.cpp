/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <IGem.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <Behaviors/LoggingGroupBehavior.h>
#include <Processors/LoadingTrackingProcessor.h>
#include <Processors/ExportTrackingProcessor.h>

namespace SceneLoggingExample
{
    // The SceneLoggingExampleModule is the entry point for gems. To extend the SceneAPI, the  
    // logging, loading, and export components must be registered here.
    //
    // NOTE: The gem system currently does not support registering file extensions through the  
    // AssetImportRequest EBus.
    class SceneLoggingExampleModule
        : public CryHooksModule
    {
    public:
        AZ_CLASS_ALLOCATOR(SceneLoggingExampleModule, AZ::SystemAllocator)
        AZ_RTTI(SceneLoggingExampleModule, "{36AA9C0F-7976-40C7-AF54-C492AC5B16F6}", CryHooksModule);

        SceneLoggingExampleModule()
            : CryHooksModule()
        {
            // The SceneAPI libraries require specialized initialization. As early as possible, be 
            // sure to repeat the following two lines for any SceneAPI you want to use. Omitting these 
            // calls or making them too late can cause problems such as missing EBus events.
            m_sceneCoreModule = AZ::DynamicModuleHandle::Create("SceneCore");
            m_sceneCoreModule->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired);
            m_descriptors.insert(m_descriptors.end(), 
            {
                LoggingGroupBehavior::CreateDescriptor(),
                LoadingTrackingProcessor::CreateDescriptor(),
                ExportTrackingProcessor::CreateDescriptor()
            });
        }

        // In this example, no system components are added. You can use system components 
        // to set global settings for this gem.
        // For functionality that should always be available to the SceneAPI, we recommend 
        // that you use a BehaviorComponent instead.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {};
        }

    private:
        AZStd::unique_ptr<AZ::DynamicModuleHandle> m_sceneCoreModule;
    };
} // namespace SceneLoggingExample

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), SceneLoggingExample::SceneLoggingExampleModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_SceneLoggingExample, SceneLoggingExample::SceneLoggingExampleModule)
#endif
