/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <Source/SceneProcessingModule.h>

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/string/regex.h>

#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>
#include <Config/SettingsObjects/NodeSoftNameSetting.h>
#include <Config/SettingsObjects/FileSoftNameSetting.h>
#include <Config/Components/SceneProcessingConfigSystemComponent.h>
#include <Config/Widgets/GraphTypeSelector.h>

namespace AZ
{
    namespace SceneProcessingConfig
    {
        void SceneProcessingConfigSystemComponentSerializationEvents::OnWriteBegin(void* classPtr)
        {
            SceneProcessingConfigSystemComponent* component = reinterpret_cast<SceneProcessingConfigSystemComponent*>(classPtr);
            component->Clear();
        }

        SceneProcessingConfigSystemComponent::SceneProcessingConfigSystemComponent()
        {
            using namespace AZ::SceneAPI::SceneCore;

            ActivateSceneModule(SceneProcessing::s_sceneCoreModule);
            ActivateSceneModule(SceneProcessing::s_sceneDataModule);
            ActivateSceneModule(SceneProcessing::s_sceneBuilderModule);

            PopulateSoftNameSettings();
            m_UseCustomNormals = true;
        }

        void SceneProcessingConfigSystemComponent::PopulateSoftNameSettings()
        {
            bool softNameSettingsFound = false;
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                softNameSettingsFound =
                    AddSoftNameSettingsFromSettingsRegistry<NodeSoftNameSetting>(settingsRegistry, AssetProcessorDefaultNodeSoftNameSettingsKey);
                softNameSettingsFound = softNameSettingsFound &&
                    AddSoftNameSettingsFromSettingsRegistry<FileSoftNameSetting>(settingsRegistry, AssetProcessorDefaultFileSoftNameSettingsKey);
            }

            AZ_Warning("SceneProcessing", softNameSettingsFound,
                "No soft name settings are found from the settings registry. If you want to customize the soft naming conventions, "
                "Please override the default values using the Project User Registry or the global machine registry (~/.o3de/Registry/) "
                "instead of modifying the default Gems/SceneSettings/Registry/SoftNameSettings.setreg.");
        }

        template<class SoftNameSettingsType>
        bool SceneProcessingConfigSystemComponent::AddSoftNameSettingsFromSettingsRegistry(
            AZ::SettingsRegistryInterface* settingsRegistry, AZStd::string_view settingRegistryKey)
        {
            AZStd::vector<AZStd::unique_ptr<SoftNameSettingsType>> softNameSettings;
            if (!settingsRegistry->GetObject(softNameSettings, settingRegistryKey))
            {
                return false;
            }

            for (AZStd::unique_ptr<SoftNameSettingsType>& softNameSetting : softNameSettings)
            {
                AddSoftName(AZStd::move(softNameSetting));
            }

            return true;
        }

        void SceneProcessingConfigSystemComponent::Activate()
        {
            SceneProcessingConfigRequestBus::Handler::BusConnect();
            AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusConnect();
            AZ::SceneAPI::Events::ScriptConfigEventBus::Handler::BusConnect();
            SceneProcessingConfig::GraphTypeSelector::Register();
            LoadScriptSettings();
        }

        void SceneProcessingConfigSystemComponent::Deactivate()
        {
            SceneProcessingConfig::GraphTypeSelector::Unregister();
            AZ::SceneAPI::Events::ScriptConfigEventBus::Handler::BusDisconnect();
            AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusDisconnect();
            SceneProcessingConfigRequestBus::Handler::BusDisconnect();
        }

        SceneProcessingConfigSystemComponent::~SceneProcessingConfigSystemComponent()
        {
            Clear();

            DeactivateSceneModule(SceneProcessing::s_sceneBuilderModule);
            DeactivateSceneModule(SceneProcessing::s_sceneDataModule);
            DeactivateSceneModule(SceneProcessing::s_sceneCoreModule);
        }

        void SceneProcessingConfigSystemComponent::Clear()
        {
            m_softNames.clear();
            m_softNames.shrink_to_fit();
            m_scriptConfigList.clear();
            m_scriptConfigList.shrink_to_fit();
            m_UseCustomNormals = true;
        }

        const AZStd::vector<AZStd::unique_ptr<SoftNameSetting>>* SceneProcessingConfigSystemComponent::GetSoftNames()
        {
            return &m_softNames;
        }

        bool SceneProcessingConfigSystemComponent::AddSoftName(AZStd::unique_ptr<SoftNameSetting> newSoftname)
        {
            bool success = true;
            Crc32 newHash = newSoftname->GetVirtualTypeHash();
            for (AZStd::unique_ptr<SoftNameSetting>& softName : m_softNames)
            {
                //First check whether an item with the same CRC value already exists.
                if (newHash == softName->GetVirtualTypeHash() && softName->GetTypeId() == newSoftname->GetTypeId())
                {
                    AZ_Error("SceneProcessing", false, "newSoftname(%s) and existing softName(%s) have the same hash: 0x%X",
                             newSoftname->GetVirtualType().c_str(), softName->GetVirtualType().c_str(), newHash);
                    success = false;
                    break;
                }
            }
            if (success)
            {
                m_softNames.push_back(AZStd::move(newSoftname));
            }
            return success;
        }

        bool SceneProcessingConfigSystemComponent::AddNodeSoftName(const char* pattern,
            SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
            const char* virtualType, bool includeChildren)
        {
            AZStd::unique_ptr<SoftNameSetting> newSoftname = AZStd::make_unique<NodeSoftNameSetting>(pattern, approach, virtualType, includeChildren);
            return AddSoftName(AZStd::move(newSoftname));
        }

        bool SceneProcessingConfigSystemComponent::AddFileSoftName(const char* pattern,
            SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
            const char* virtualType, bool inclusive, const AZStd::string& graphObjectTypeName)
        {
            AZStd::unique_ptr<SoftNameSetting> newSoftname = AZStd::make_unique<FileSoftNameSetting>(
                pattern, approach, virtualType, inclusive,
                std::initializer_list<FileSoftNameSetting::GraphType>{FileSoftNameSetting::GraphType(graphObjectTypeName)});
            return AddSoftName(AZStd::move(newSoftname));
        }

        void SceneProcessingConfigSystemComponent::LoadScriptSettings()
        {
            auto* registry = AZ::SettingsRegistry::Get();
            AZ_Assert(registry, "AZ::SettingsRegistryInterface should already be active!");

            auto* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::FileIOBase should already be active!");

            using VisitArgs = AZ::SettingsRegistryInterface::VisitArgs;
            using VisitResponse = AZ::SettingsRegistryInterface::VisitResponse;
            using VisitAction = AZ::SettingsRegistryInterface::VisitAction;
            auto vistor = [this, registry, fileIO](const VisitArgs& args, VisitAction action) -> VisitResponse
            {
                if (action == VisitAction::Begin)
                {
                    return VisitResponse::Continue;
                }
                else if (action == VisitAction::End)
                {
                    return VisitResponse::Done;
                }
                else if (args.m_type != AZ::SettingsRegistryInterface::Type::String)
                {
                    return VisitResponse::Continue;
                }

                AZStd::string value;
                registry->Get(value, args.m_jsonKeyPath);
                auto fullPath = fileIO->ResolvePath(value.c_str());
                if (!fullPath)
                {
                    AZ_Warning("SceneProcessing", false,
                        "FileIO could not resolve default builder script path %s for pattern key " AZ_STRING_FORMAT,
                        value.c_str(),
                        AZ_STRING_ARG(args.m_jsonKeyPath));

                    return VisitResponse::Continue;
                }
                else if (!fileIO->Exists(fullPath->c_str()))
                {
                    AZ_Warning("SceneProcessing", false,
                        "The full script path %s does not exist when resolving default scene building script name %s for key " AZ_STRING_FORMAT,
                        fullPath->c_str(),
                        value.c_str(),
                        AZ_STRING_ARG(args.m_jsonKeyPath));

                    return VisitResponse::Continue;
                }
                AZ::SceneAPI::Events::ScriptConfig scriptConfig;
                scriptConfig.m_pattern = args.m_fieldName;
                scriptConfig.m_scriptPath = AZStd::move(fullPath.value());
                m_scriptConfigList.emplace_back(scriptConfig);
                return VisitResponse::Continue;
            };
            registry->Visit(vistor, AssetProcessorDefaultScriptsKey);
        }

        void SceneProcessingConfigSystemComponent::GetScriptConfigList(AZStd::vector<SceneAPI::Events::ScriptConfig>& scriptConfigList) const
        {
            scriptConfigList = m_scriptConfigList;
        }

        AZStd::optional<SceneAPI::Events::ScriptConfig> SceneProcessingConfigSystemComponent::MatchesScriptConfig(const AZStd::string& sourceFile) const
        {
            for (const auto& scriptConfig : m_scriptConfigList)
            {
                AZStd::regex comparer(scriptConfig.m_pattern, AZStd::regex::extended);
                AZStd::smatch match;
                if (AZStd::regex_search(sourceFile, match, comparer))
                {
                    return AZStd::make_optional(scriptConfig);
                }
            }
            return AZStd::nullopt;
        }

        void SceneProcessingConfigSystemComponent::AreCustomNormalsUsed(bool &value)
        {
            value = m_UseCustomNormals;
        }

        void SceneProcessingConfigSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            ReflectSceneModule(context, SceneProcessing::s_sceneCoreModule);
            ReflectSceneModule(context, SceneProcessing::s_sceneDataModule);
            ReflectSceneModule(context, SceneProcessing::s_sceneBuilderModule);

            SoftNameSetting::Reflect(context);
            NodeSoftNameSetting::Reflect(context);
            FileSoftNameSetting::Reflect(context);

            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<SceneProcessingConfigSystemComponent, AZ::Component>()
                    ->Version(3)
                    ->EventHandler<SceneProcessingConfigSystemComponentSerializationEvents>() 
                    ->Field("softNames", &SceneProcessingConfigSystemComponent::m_softNames)
                    ->Field("useCustomNormals", &SceneProcessingConfigSystemComponent::m_UseCustomNormals);

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<SceneProcessingConfigSystemComponent>("Scene Processing Config", "Use this component to fine tune the defaults for processing of scene files like FBX.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Assets")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SceneProcessingConfigSystemComponent::m_softNames,
                            "Soft naming conventions", "Update the naming conventions to suit your project.")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SceneProcessingConfigSystemComponent::m_UseCustomNormals,
                            "Use Custom Normals", "When enabled, Open 3D Engine will use the DCC assets custom or tangent space normals. When disabled, the normals will be averaged. This setting can be overridden on an individual scene file's asset settings.")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false);
                }
            }
        }

        void SceneProcessingConfigSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("SceneProcessingConfigService"));
        }

        void SceneProcessingConfigSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("SceneProcessingConfigService"));
        }

        void SceneProcessingConfigSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            AZ_UNUSED(required);
        }

        void SceneProcessingConfigSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void SceneProcessingConfigSystemComponent::ReflectSceneModule(AZ::ReflectContext* context, 
            const AZStd::unique_ptr<AZ::DynamicModuleHandle>& module)
        {
            using ReflectFunc = void(*)(AZ::SerializeContext*);

            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                if (module)
                {
                    ReflectFunc reflect = module->GetFunction<ReflectFunc>("Reflect");
                    if (reflect)
                    {
                        (*reflect)(serialize);
                    }
                }
            }

            using ReflectBehaviorFunc = void(*)(AZ::BehaviorContext*);

            AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behavior)
            {
                if (module)
                {
                    ReflectBehaviorFunc reflectBehavior = module->GetFunction<ReflectBehaviorFunc>("ReflectBehavior");
                    if (reflectBehavior)
                    {
                        (*reflectBehavior)(behavior);
                    }
                }
            }
        }

        void SceneProcessingConfigSystemComponent::ActivateSceneModule(const AZStd::unique_ptr<AZ::DynamicModuleHandle>& module)
        {
            using ActivateFunc = void(*)();

            if (module)
            {
                ActivateFunc activate = module->GetFunction<ActivateFunc>("Activate");
                if (activate)
                {
                    (*activate)();
                }
            }
        }

        void SceneProcessingConfigSystemComponent::DeactivateSceneModule(const AZStd::unique_ptr<AZ::DynamicModuleHandle>& module)
        {
            using DeactivateFunc = void(*)();

            if (module)
            {
                DeactivateFunc deactivate = module->GetFunction<DeactivateFunc>("Deactivate");
                if (deactivate)
                {
                    (*deactivate)();
                }
            }
        }
    } // namespace SceneProcessingConfig
} // namespace AZ
