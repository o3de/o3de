/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Module.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>
#include <SceneBuilder/SceneBuilderComponent.h>
#include <SceneBuilder/SceneSerializationHandler.h>
#include <Config/Components/SceneProcessingConfigSystemComponent.h>
#include <Config/Components/SoftNameBehavior.h>
#include <Config/Widgets/GraphTypeSelector.h>
#include <Generation/Components/TangentGenerator/TangentGenerateComponent.h>
#include <Generation/Components/TangentGenerator/TangentPreExportComponent.h>
#include <Generation/Components/UVsGenerator/UVsGenerateComponent.h>
#include <Generation/Components/UVsGenerator/UVsPreExportComponent.h>
#include <Generation/Components/MeshOptimizer/MeshOptimizerComponent.h>
#include <Source/SceneProcessingModule.h>

namespace AZ
{
    namespace SceneProcessing
    {
        class SceneProcessingModule
            : public Module
        {
        public:
            AZ_RTTI(SceneProcessingModule, "{13DCFEF2-BB25-4DBB-A69B-22958CAD6885}", Module);

            SceneProcessingModule()
                : Module()
            {
                LoadSceneModule(s_sceneCoreModule, "SceneCore");
                LoadSceneModule(s_sceneDataModule, "SceneData");
                LoadSceneModule(s_sceneBuilderModule, "SceneBuilder");

                m_descriptors.insert(m_descriptors.end(),
                {
                    SceneProcessingConfig::SceneProcessingConfigSystemComponent::CreateDescriptor(),
                    SceneProcessingConfig::SoftNameBehavior::CreateDescriptor(),
                    SceneBuilder::BuilderPluginComponent::CreateDescriptor(),
                    SceneBuilder::SceneSerializationHandler::CreateDescriptor(),
                    AZ::SceneGenerationComponents::TangentPreExportComponent::CreateDescriptor(),
                    AZ::SceneGenerationComponents::TangentGenerateComponent::CreateDescriptor(),
                    AZ::SceneGenerationComponents::CreateUVsGenerateComponentDescriptor(),
                    AZ::SceneGenerationComponents::CreateUVsPreExportComponentDescriptor(),
                    AZ::SceneGenerationComponents::MeshOptimizerComponent::CreateDescriptor(),
                });

                // This is an internal Amazon gem, so register it's components for metrics tracking, otherwise the name of the component won't get sent back.
                // IF YOU ARE A THIRDPARTY WRITING A GEM, DO NOT REGISTER YOUR COMPONENTS WITH EditorMetricsComponentRegistrationBus
                AZStd::vector<AZ::Uuid> typeIds;
                typeIds.reserve(m_descriptors.size());
                for (AZ::ComponentDescriptor* descriptor : m_descriptors)
                {
                    typeIds.emplace_back(descriptor->GetUuid());
                }
                AzFramework::MetricsPlainTextNameRegistrationBus::Broadcast(&AzFramework::MetricsPlainTextNameRegistrationBus::Events::RegisterForNameSending, typeIds);
            }

            ~SceneProcessingModule()
            {
                UnloadModule(s_sceneBuilderModule);
                UnloadModule(s_sceneDataModule);
                UnloadModule(s_sceneCoreModule);
            }

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList
                {
                    azrtti_typeid<SceneProcessingConfig::SceneProcessingConfigSystemComponent>(),
                };
            }

            void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC_CE("SceneConfiguration"));
            }

            void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC_CE("SceneConfiguration"));
            }

        protected:
            void LoadSceneModule(AZStd::unique_ptr<DynamicModuleHandle>& module, const char* name)
            {
                if (!module)
                {
                    module = DynamicModuleHandle::Create(name);
                    if (module)
                    {
                        module->Load();
                        auto init = module->GetFunction<InitializeDynamicModuleFunction>(InitializeDynamicModuleFunctionName);
                        if (init)
                        {
                            (*init)();
                        }
                    }
                }
            }

            void UnloadModule(AZStd::unique_ptr<DynamicModuleHandle>& module)
            {
                if (module)
                {
                    auto uninit = module->GetFunction<UninitializeDynamicModuleFunction>(UninitializeDynamicModuleFunctionName);
                    if (uninit)
                    {
                        (*uninit)();
                    }
                    module.reset();
                }
            }
        };
    } // namespace SceneProcessing
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), AZ::SceneProcessing::SceneProcessingModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_SceneProcessing_Editor, AZ::SceneProcessing::SceneProcessingModule)
#endif
