/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneData/Behaviors/ScriptProcessorRuleBehavior.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneData/Rules/ScriptProcessorRule.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Events/ImportEventContext.h>

namespace AZ::SceneAPI::Behaviors
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

    using ExportProductList = AZ::SceneAPI::Events::ExportProductList;

    // a event bus to signal during scene building
    struct ScriptBuildingNotifications
        : public AZ::EBusTraits
    {
        virtual AZStd::string OnUpdateManifest(Containers::Scene& scene) = 0;
        virtual ExportProductList OnPrepareForExport(
            const Containers::Scene& scene,
            AZStd::string_view outputDirectory,
            AZStd::string_view platformIdentifier,
            const ExportProductList& productList) = 0;
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
            OnUpdateManifest,
            OnPrepareForExport);

        virtual ~ScriptBuildingNotificationBusHandler() = default;

        AZStd::string OnUpdateManifest(Containers::Scene& scene) override
        {
            ScriptScope onUpdateManifestScope(this);
            AZStd::string result;
            CallResult(result, FN_OnUpdateManifest, scene);
            ScriptBuildingNotificationBusHandler::BusDisconnect();
            return result;
        }

        ExportProductList OnPrepareForExport(
            const Containers::Scene& scene,
            AZStd::string_view outputDirectory,
            AZStd::string_view platformIdentifier,
            const ExportProductList& productList) override
        {
            ScriptScope onPrepareForExportScope(this);
            ExportProductList result;
            CallResult(result, FN_OnPrepareForExport, scene, outputDirectory, platformIdentifier, productList);
            ScriptBuildingNotificationBusHandler::BusDisconnect();
            return result;
        }

        AZStd::atomic_int m_count = 1;

        static BehaviorEBusHandler* Create()
        {
            return aznew ScriptBuildingNotificationBusHandler();
        }

        static void Destroy(BehaviorEBusHandler* behaviorEBusHandler)
        {
            auto* handler =
                static_cast<ScriptBuildingNotificationBusHandler*>(behaviorEBusHandler);

            --handler->m_count;
            if (handler->m_count == 0)
            {
                delete handler;
            }
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<ScriptBuildingNotificationBus>("ScriptBuildingNotificationBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Module, "scene")
                    ->Handler<ScriptBuildingNotificationBusHandler>(&ScriptBuildingNotificationBusHandler::Create, &ScriptBuildingNotificationBusHandler::Destroy)
                    ->Event("OnUpdateManifest", &ScriptBuildingNotificationBus::Events::OnUpdateManifest)
                    ->Event("OnPrepareForExport", &ScriptBuildingNotificationBus::Events::OnPrepareForExport);
            }
        }

        struct ScriptScope final
        {
            ScriptScope(ScriptBuildingNotificationBusHandler* self)
                : m_self(self)
            {
                m_self->m_count++;
            }

            ~ScriptScope()
            {
                m_self->m_count--;
                if (m_self->m_count == 0)
                {
                    // the script released the handler (i.e. set to None)
                    BehaviorEBusHandler* self = m_self;
                    AZStd::function<void()> destroySelf = [self]()
                    {
                        ScriptBuildingNotificationBusHandler::Destroy(self);
                    };
                    // Delay to delete self until the end of the scene pipeline
                    m_self->m_count = 1;
                    AZ::SceneAPI::Events::AssetPostImportRequestBus::QueueBroadcast(
                        &AZ::SceneAPI::Events::AssetPostImportRequestBus::Events::CallAfterSceneExport, destroySelf);
                }
            }

            ScriptBuildingNotificationBusHandler* m_self = {};
        };
    };

    struct ScriptProcessorRuleBehavior::EventHandler final
        : public AZ::SceneAPI::SceneCore::ExportingComponent
    {
        using PreExportEventContextFunction = AZStd::function<bool(Events::PreExportEventContext&)>;
        PreExportEventContextFunction m_preExportEventContextFunction;

        EventHandler(PreExportEventContextFunction preExportEventContextFunction)
            : m_preExportEventContextFunction(preExportEventContextFunction)
        {
            BindToCall(&EventHandler::PrepareForExport);
            BindToCall(&EventHandler::PreImportEventContext);
            AZ::SceneAPI::SceneCore::ExportingComponent::Activate();
        }

        ~EventHandler()
        {
            AZ::SceneAPI::SceneCore::ExportingComponent::Deactivate();
        }

        // this allows a Python script to add product assets on "scene export"
        Events::ProcessingResult PrepareForExport(Events::PreExportEventContext& context)
        {
            return m_preExportEventContextFunction(context) ? Events::ProcessingResult::Success : Events::ProcessingResult::Failure;
        }

        // used to detect that the "next" source scene is starting to be processed
        Events::ProcessingResult PreImportEventContext([[maybe_unused]] Events::PreImportEventContext& context)
        {
            m_pythonScriptStack.clear();
            return Events::ProcessingResult::Success;
        }

        AZStd::vector<AZStd::string> m_pythonScriptStack;
    };

    void ScriptProcessorRuleBehavior::Activate()
    {
        Events::AssetImportRequestBus::Handler::BusConnect();
        m_eventHandler = AZStd::make_shared<EventHandler>([this](Events::PreExportEventContext& context)
        {
            return this->DoPrepareForExport(context);
        });
    }

    void ScriptProcessorRuleBehavior::Deactivate()
    {
        m_eventHandler.reset();
        Events::AssetImportRequestBus::Handler::BusDisconnect();
        UnloadPython();
    }

    bool ScriptProcessorRuleBehavior::LoadPython(const AZ::SceneAPI::Containers::Scene& scene, AZStd::string& scriptPath, Events::ProcessingResult& fallbackResult)
    {
        using namespace AZ::SceneAPI;

        fallbackResult = Events::ProcessingResult::Failure;
        int scriptDiscoveryAttempts = 0;
        const Containers::SceneManifest& manifest = scene.GetManifest();
        auto view = Containers::MakeDerivedFilterView<DataTypes::IScriptProcessorRule>(manifest.GetValueStorage());
        for (const auto& scriptItem : view)
        {
            AZ::IO::FixedMaxPath scriptFilename(scriptItem.GetScriptFilename());
            if (scriptFilename.empty())
            {
                AZ_Warning("scene", false, "Skipping an empty script filename in (%s)", scene.GetManifestFilename().c_str());
                continue;
            }

            ++scriptDiscoveryAttempts;
            fallbackResult = (scriptItem.GetScriptProcessorFallbackLogic() == DataTypes::ScriptProcessorFallbackLogic::ContinueBuild) ?
                Events::ProcessingResult::Ignored : Events::ProcessingResult::Failure;

            // check for file exist via absolute path
            if (!IO::FileIOBase::GetInstance()->Exists(scriptFilename.c_str()))
            {
                // get project folder
                auto settingsRegistry = AZ::SettingsRegistry::Get();
                AZ::IO::FixedMaxPath projectPath;
                if (!settingsRegistry->Get(projectPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath))
                {
                    AZ_Error("scene", false, "With (%s) could not find Project Path during script discovery.",
                        scene.GetManifestFilename().c_str());
                    return false;
                }

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

            scriptPath = scriptFilename.c_str();
            break;
        }

        if (scriptDiscoveryAttempts == 0)
        {
            if (!m_eventHandler->m_pythonScriptStack.empty())
            {
                scriptPath = m_eventHandler->m_pythonScriptStack.back();
            }
        }

        if (scriptPath.empty())
        {
            AZ_Warning("scene", scriptDiscoveryAttempts == 0,
                "The scene manifest (%s) attempted to use script rule, but no script file path could be found.",
                scene.GetManifestFilename().c_str());
            return false;
        }
        else
        {
            m_eventHandler->m_pythonScriptStack.push_back(scriptPath);
        }

        // already prepared the Python VM?
        if (m_editorPythonEventsInterface)
        {
            return true;
        }

        // lazy load the Python interface
        auto editorPythonEventsInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get();
        if (editorPythonEventsInterface->IsPythonActive() == false)
        {
            const bool silenceWarnings = false;
            if (editorPythonEventsInterface->StartPython(silenceWarnings) == false)
            {
                editorPythonEventsInterface = nullptr;
            }
        }

        // both Python and the script need to be ready
        if (editorPythonEventsInterface == nullptr)
        {
            AZ_Warning("scene", false,
                "The scene manifest (%s) attempted to prepare Python but Python can not start",
                scene.GetManifestFilename().c_str());

            return false;
        }

        m_editorPythonEventsInterface = editorPythonEventsInterface;
        return true;
    }

    void ScriptProcessorRuleBehavior::UnloadPython()
    {
        if (m_editorPythonEventsInterface)
        {
            const bool silenceWarnings = true;
            m_editorPythonEventsInterface->StopPython(silenceWarnings);
            m_editorPythonEventsInterface = nullptr;
        }
    }

    bool ScriptProcessorRuleBehavior::DoPrepareForExport(Events::PreExportEventContext& context)
    {
        using namespace AzToolsFramework;

        AZStd::string scriptPath;

        auto executeCallback = [&context, &scriptPath]()
        {
            // set up script's hook callback
            EditorPythonRunnerRequestBus::Broadcast(&EditorPythonRunnerRequestBus::Events::ExecuteByFilename,
                scriptPath.c_str());

            // call script's callback to allow extra products
            ExportProductList extraProducts;
            ScriptBuildingNotificationBus::BroadcastResult(extraProducts, &ScriptBuildingNotificationBus::Events::OnPrepareForExport,
                context.GetScene(),
                context.GetOutputDirectory(),
                context.GetPlatformIdentifier(),
                context.GetProductList()      
            );

            // add new products
            for (const auto& product : extraProducts.GetProducts())
            {
                context.GetProductList().AddProduct(
                    product.m_filename,
                    product.m_id,
                    product.m_assetType,
                    product.m_lod,
                    product.m_subId,
                    product.m_dependencyFlags);
            }
        };

        [[maybe_unused]] Events::ProcessingResult fallbackResult;
        if (LoadPython(context.GetScene(), scriptPath, fallbackResult))
        {
            EditorPythonConsoleNotificationHandler logger;
            m_editorPythonEventsInterface->ExecuteWithLock(executeCallback);
        }

        return true;
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

        Events::ProcessingResult fallbackResult;
        AZStd::string scriptPath;
        if (LoadPython(scene, scriptPath, fallbackResult))
        {
            AZStd::string manifestUpdate;
            auto executeCallback = [&scene, &manifestUpdate, &scriptPath]()
            {
                EditorPythonRunnerRequestBus::Broadcast(&EditorPythonRunnerRequestBus::Events::ExecuteByFilename,
                    scriptPath.c_str());

                ScriptBuildingNotificationBus::BroadcastResult(manifestUpdate, &ScriptBuildingNotificationBus::Events::OnUpdateManifest,
                    scene);
            };

            EditorPythonConsoleNotificationHandler logger;
            m_editorPythonEventsInterface->ExecuteWithLock(executeCallback);

            // if the returned scene manifest is empty then ignore the script update
            if (manifestUpdate.empty())
            {
                return Events::ProcessingResult::Ignored;
            }

            // attempt to load the manifest string back to a JSON-scene-manifest
            auto sceneManifestLoader = AZStd::make_unique<AZ::SceneAPI::Containers::SceneManifest>();
            auto loadOutcome = sceneManifestLoader->LoadFromString(manifestUpdate);
            if (loadOutcome.IsSuccess())
            {
                scene.GetManifest().Clear();
                for (size_t entryIndex = 0; entryIndex < sceneManifestLoader->GetEntryCount(); ++entryIndex)
                {
                    scene.GetManifest().AddEntry(sceneManifestLoader->GetValue(entryIndex));
                }
                return Events::ProcessingResult::Success;
            }
            else
            {
                // if the manifest was not updated by the script, then return back the fallback result
                return fallbackResult;
            }
        }
        return Events::ProcessingResult::Ignored;
    }

    void ScriptProcessorRuleBehavior::GetManifestDependencyPaths(AZStd::vector<AZStd::string>& paths)
    {
        paths.emplace_back("/scriptFilename");
    }
} // namespace AZ
