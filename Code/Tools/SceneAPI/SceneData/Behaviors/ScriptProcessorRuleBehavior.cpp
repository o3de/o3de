/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneData/Behaviors/ScriptProcessorRuleBehavior.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneData/Rules/ScriptProcessorRule.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Behaviors
        {
            class EditorPythonConsoleNotificationHandler final
                : protected AzToolsFramework::EditorPythonConsoleNotificationBus::Handler
            {
            public:
                EditorPythonConsoleNotificationHandler()
                {
                    BusConnect();
                }

                ~EditorPythonConsoleNotificationHandler()
                {
                    BusDisconnect();
                }

                ////////////////////////////////////////////////////////////////////////////////////////////
                // AzToolsFramework::EditorPythonConsoleNotifications
                void OnTraceMessage([[maybe_unused]] AZStd::string_view message) override
                {
                    using namespace AZ::SceneAPI::Utilities;
                    AZ_TracePrintf(LogWindow, "%.*s \n", AZ_STRING_ARG(message));
                }

                void OnErrorMessage([[maybe_unused]] AZStd::string_view message) override
                {
                    using namespace AZ::SceneAPI::Utilities;
                    AZ_TracePrintf(ErrorWindow, "[ERROR] %.*s \n", AZ_STRING_ARG(message));
                }

                void OnExceptionMessage([[maybe_unused]] AZStd::string_view message) override
                {
                    using namespace AZ::SceneAPI::Utilities;
                    AZ_TracePrintf(ErrorWindow, "[EXCEPTION] %.*s \n", AZ_STRING_ARG(message));
                }
            };                        

            // a event bus to signal during scene building
            struct ScriptBuildingNotifications
                : public AZ::EBusTraits
            {
                virtual AZStd::string OnUpdateManifest(Containers::Scene& scene) = 0;
            };
            using ScriptBuildingNotificationBus = AZ::EBus<ScriptBuildingNotifications>;

            // a back end to handle scene builder events for a script
            struct ScriptBuildingNotificationBusHandler final
                : public ScriptBuildingNotificationBus::Handler
                , public AZ::BehaviorEBusHandler
            {
                AZ_EBUS_BEHAVIOR_BINDER(
                    ScriptBuildingNotificationBusHandler,
                    "{DF2B51DE-A4D0-4139-B5D0-DF185832380D}",
                    AZ::SystemAllocator,
                    OnUpdateManifest);

                virtual ~ScriptBuildingNotificationBusHandler() = default;

                AZStd::string OnUpdateManifest(Containers::Scene& scene) override
                {
                    AZStd::string result;
                    CallResult(result, FN_OnUpdateManifest, scene);
                    return result;
                }

                static void Reflect(AZ::ReflectContext* context)
                {
                    if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
                    {
                        behaviorContext->EBus<ScriptBuildingNotificationBus>("ScriptBuildingNotificationBus")
                            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                            ->Attribute(AZ::Script::Attributes::Module, "scene")
                            ->Handler<ScriptBuildingNotificationBusHandler>()
                            ->Event("OnUpdateManifest", &ScriptBuildingNotificationBus::Events::OnUpdateManifest);
                    }
                }
            };

            void ScriptProcessorRuleBehavior::Activate()
            {
                Events::AssetImportRequestBus::Handler::BusConnect();
            }

            void ScriptProcessorRuleBehavior::Deactivate()
            {
                Events::AssetImportRequestBus::Handler::BusDisconnect();
                if (m_editorPythonEventsInterface)
                {
                    const bool silenceWarnings = true;
                    m_editorPythonEventsInterface->StopPython(silenceWarnings);
                    m_editorPythonEventsInterface = nullptr;
                        
                }
            }

            void ScriptProcessorRuleBehavior::Reflect(ReflectContext* context)
            {
                ScriptBuildingNotificationBusHandler::Reflect(context);

                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<ScriptProcessorRuleBehavior, BehaviorComponent>()->Version(1);
                }
            }

            Events::ProcessingResult ScriptProcessorRuleBehavior::UpdateManifest(
                Containers::Scene& scene,
                Events::AssetImportRequest::ManifestAction action,
                [[maybe_unused]] Events::AssetImportRequest::RequestingApplication requester)
            {
                using namespace AzToolsFramework;

                if (action != ManifestAction::Update)
                {
                    return Events::ProcessingResult::Ignored;
                }

                // get project folder
                auto settingsRegistry = AZ::SettingsRegistry::Get();
                AZ::IO::FixedMaxPath projectPath;
                if (!settingsRegistry->Get(projectPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath))
                {
                    return Events::ProcessingResult::Ignored;
                }

                auto& sceneManifest = scene.GetManifest();
                auto view = Containers::MakeDerivedFilterView<DataTypes::IScriptProcessorRule>(sceneManifest.GetValueStorage());
                for (const auto& scriptItem : view)
                {
                    AZ::IO::FixedMaxPath scriptFilename(scriptItem.GetScriptFilename());
                    if (scriptFilename.empty())
                    {
                        AZ_Warning("scene", false, "Skipping an empty script filename in (%s)", scene.GetManifestFilename().c_str());
                        continue;
                    }

                    // check for file exist via absolute path
                    if (!IO::FileIOBase::GetInstance()->Exists(scriptFilename.c_str()))
                    {
                        // check for script in the project folder
                        AZ::IO::FixedMaxPath projectScriptPath = projectPath / scriptFilename;
                        if (!IO::FileIOBase::GetInstance()->Exists(projectScriptPath.c_str()))
                        {
                            AZ_Warning("scene", false, "Skipping a missing script (%s) in manifest file (%s)",
                                scriptFilename.c_str(),
                                scene.GetManifestFilename().c_str());

                            continue;
                        }
                        scriptFilename = AZStd::move(projectScriptPath);
                    }

                    // lazy load the Python interface
                    if (!m_editorPythonEventsInterface)
                    {
                        m_editorPythonEventsInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get();
                        const bool silenceWarnings = true;
                        m_editorPythonEventsInterface->StartPython(silenceWarnings);
                    }

                    if (!m_editorPythonEventsInterface && !scriptFilename.empty())
                    {
                        AZ_Warning("scene", false,
                            "The scene manifest (%s) attempted to use script(%s) but Python is not enabled;"
                            "please add the EditorPythonBinding gem & PythonAssetBuilder gem to your project.",
                            scene.GetManifestFilename().c_str(), scriptFilename.c_str());

                        return Events::ProcessingResult::Ignored;
                    }

                    AZStd::string manifestUpdate;
                    auto executeCallback = [&scene, &scriptFilename, &manifestUpdate]()
                    {
                        EditorPythonRunnerRequestBus::Broadcast(
                            &EditorPythonRunnerRequestBus::Events::ExecuteByFilename,
                            scriptFilename.c_str());

                        ScriptBuildingNotificationBus::BroadcastResult(
                            manifestUpdate,
                            &ScriptBuildingNotificationBus::Events::OnUpdateManifest,
                            scene);
                    };
                    EditorPythonConsoleNotificationHandler logger;
                    m_editorPythonEventsInterface->ExecuteWithLock(executeCallback);

                    // attempt to load the manifest string back to a JSON-scene-manifest
                    auto sceneManifestLoader = AZStd::make_unique<AZ::SceneAPI::Containers::SceneManifest>();
                    auto loadOutcome = sceneManifestLoader->LoadFromString(manifestUpdate);
                    if (loadOutcome.IsSuccess())
                    {
                        sceneManifest.Clear();
                        for (size_t entryIndex = 0; entryIndex < sceneManifestLoader->GetEntryCount(); ++entryIndex)
                        {
                            sceneManifest.AddEntry(sceneManifestLoader->GetValue(entryIndex));
                        }
                        return Events::ProcessingResult::Success;
                    }
                }
                return Events::ProcessingResult::Ignored;
            }

        } // namespace Behaviors
    } // namespace SceneAPI
} // namespace AZ
