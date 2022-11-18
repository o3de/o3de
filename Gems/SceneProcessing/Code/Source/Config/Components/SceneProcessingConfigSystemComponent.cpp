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
            
            // Defaults in case there's no config setup
            m_softNames.push_back(aznew NodeSoftNameSetting("^.*_[Ll][Oo][Dd]1(_optimized)?$", PatternMatcher::MatchApproach::Regex, "LODMesh1", true));
            m_softNames.push_back(aznew NodeSoftNameSetting("^.*_[Ll][Oo][Dd]2(_optimized)?$", PatternMatcher::MatchApproach::Regex, "LODMesh2", true));
            m_softNames.push_back(aznew NodeSoftNameSetting("^.*_[Ll][Oo][Dd]3(_optimized)?$", PatternMatcher::MatchApproach::Regex, "LODMesh3", true));
            m_softNames.push_back(aznew NodeSoftNameSetting("^.*_[Ll][Oo][Dd]4(_optimized)?$", PatternMatcher::MatchApproach::Regex, "LODMesh4", true));
            m_softNames.push_back(aznew NodeSoftNameSetting("^.*_[Ll][Oo][Dd]5(_optimized)?$", PatternMatcher::MatchApproach::Regex, "LODMesh5", true));
            m_softNames.push_back(aznew NodeSoftNameSetting("^.*_[Pp][Hh][Yy][Ss](_optimized)?$", PatternMatcher::MatchApproach::Regex, "PhysicsMesh", true));
            m_softNames.push_back(aznew NodeSoftNameSetting("_ignore", PatternMatcher::MatchApproach::PostFix, "Ignore", false));
            // If the filename ends with "_anim" this will mark all nodes as "Ignore" unless they're derived from IAnimationData. This will
            // cause only animations to be exported from the source scene file even if there's other data available.
            m_softNames.push_back(aznew FileSoftNameSetting("_anim", PatternMatcher::MatchApproach::PostFix, "Ignore", false,
                { FileSoftNameSetting::GraphType(SceneAPI::DataTypes::IAnimationData::TYPEINFO_Name()) }));

            m_UseCustomNormals = true;
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
            for (auto* softName : m_softNames)
            {
                delete softName;
            }
            m_softNames.clear();
            m_softNames.shrink_to_fit();
            m_scriptConfigList.clear();
            m_scriptConfigList.shrink_to_fit();
            m_UseCustomNormals = true;
        }

        const AZStd::vector<SoftNameSetting*>* SceneProcessingConfigSystemComponent::GetSoftNames()
        {
            return &m_softNames;
        }

        bool SceneProcessingConfigSystemComponent::AddSoftName(SoftNameSetting* newSoftname)
        {
            bool success = true;
            Crc32 newHash = newSoftname->GetVirtualTypeHash();
            for (SoftNameSetting* softName : m_softNames)
            {
                //First check whether an item with the same CRC value already exists.
                if (newHash == softName->GetVirtualTypeHash())
                {
                    AZ_Error("SceneProcessing", false, "newSoftname(%s) and existing softName(%s) have the same hash: 0x%X",
                             newSoftname->GetVirtualType().c_str(), softName->GetVirtualType().c_str(), newHash);
                    success = false;
                    break;
                }
            }
            if (success)
            {
                m_softNames.push_back(newSoftname);
            }
            return success;
        }

        bool SceneProcessingConfigSystemComponent::AddNodeSoftName(const char* pattern,
            SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
            const char* virtualType, bool includeChildren)
        {
            SoftNameSetting* newSoftname = aznew NodeSoftNameSetting(pattern, approach, virtualType, includeChildren);
            bool success = AddSoftName(newSoftname);
            if (!success)
            {
                delete newSoftname;
            }
            return success;
        }

        bool SceneProcessingConfigSystemComponent::AddFileSoftName(const char* pattern,
            SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
            const char* virtualType, bool inclusive, const AZStd::string& graphObjectTypeName)
        {
            SoftNameSetting* newSoftname = aznew FileSoftNameSetting(pattern, approach, virtualType, inclusive,
                { FileSoftNameSetting::GraphType(graphObjectTypeName) });
            bool success = AddSoftName(newSoftname);
            if (!success)
            {
                delete newSoftname;
            }
            return success;
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
                    ->Version(2)
                    ->EventHandler<SceneProcessingConfigSystemComponentSerializationEvents>() 
                    ->Field("softNames", &SceneProcessingConfigSystemComponent::m_softNames)
                    ->Field("useCustomNormals", &SceneProcessingConfigSystemComponent::m_UseCustomNormals);

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<SceneProcessingConfigSystemComponent>("Scene Processing Config", "Use this component to fine tune the defaults for processing of scene files like FBX.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Assets")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
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
