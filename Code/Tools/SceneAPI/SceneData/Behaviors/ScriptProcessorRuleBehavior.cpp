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
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneData/Rules/ScriptProcessorRule.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>

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
            AZStd::string result;
            CallResult(result, FN_OnUpdateManifest, scene);
            return result;
        }

        ExportProductList OnPrepareForExport(
            const Containers::Scene& scene,
            AZStd::string_view outputDirectory,
            AZStd::string_view platformIdentifier,
            const ExportProductList& productList) override
        {
            ExportProductList result;
            CallResult(result, FN_OnPrepareForExport, scene, outputDirectory, platformIdentifier, productList);
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
                    ->Event("OnUpdateManifest", &ScriptBuildingNotificationBus::Events::OnUpdateManifest)
                    ->Event("OnPrepareForExport", &ScriptBuildingNotificationBus::Events::OnPrepareForExport);
            }
        }
    };

    struct ScriptProcessorRuleBehavior::ExportEventHandler final
        : public AZ::SceneAPI::SceneCore::ExportingComponent
    {
        using PreExportEventContextFunction = AZStd::function<bool(Events::PreExportEventContext&)>;
        PreExportEventContextFunction m_preExportEventContextFunction;

        ExportEventHandler(PreExportEventContextFunction preExportEventContextFunction)
            : m_preExportEventContextFunction(preExportEventContextFunction)
        {
            BindToCall(&ExportEventHandler::PrepareForExport);
            AZ::SceneAPI::SceneCore::ExportingComponent::Activate();
        }

        ~ExportEventHandler()
        {
            AZ::SceneAPI::SceneCore::ExportingComponent::Deactivate();
        }

        // this allows a Python script to add product assets on "scene export"
        Events::ProcessingResult PrepareForExport(Events::PreExportEventContext& context)
        {
            return m_preExportEventContextFunction(context) ? Events::ProcessingResult::Success : Events::ProcessingResult::Failure;
        }
    };

    void ScriptProcessorRuleBehavior::Activate()
    {
        Events::AssetImportRequestBus::Handler::BusConnect();
        m_exportEventHandler = AZStd::make_shared<ExportEventHandler>([this](Events::PreExportEventContext& context)
        {
            return this->DoPrepareForExport(context);
        });
    }

    void ScriptProcessorRuleBehavior::Deactivate()
    {
        m_exportEventHandler.reset();
        Events::AssetImportRequestBus::Handler::BusDisconnect();
        UnloadPython();
    }

    bool ScriptProcessorRuleBehavior::LoadPython(const AZ::SceneAPI::Containers::Scene& scene)
    {
        if (m_editorPythonEventsInterface && !m_scriptFilename.empty())
        {
            return true;
        }

        // get project folder
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        AZ::IO::FixedMaxPath projectPath;
        if (!settingsRegistry->Get(projectPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath))
        {
            return false;
        }

        const AZ::SceneAPI::Containers::SceneManifest& manifest = scene.GetManifest();
        auto view = Containers::MakeDerivedFilterView<DataTypes::IScriptProcessorRule>(manifest.GetValueStorage());
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
            if (editorPythonEventsInterface == nullptr || scriptFilename.empty())
            {
                AZ_Warning("scene", false,"The scene manifest (%s) attempted to use script(%s) but Python is not enabled;"
                    "please add the EditorPythonBinding gem & PythonAssetBuilder gem to your project.",
                    scene.GetManifestFilename().c_str(), scriptFilename.c_str());

                return false;
            }

            m_editorPythonEventsInterface = editorPythonEventsInterface;
            m_scriptFilename = scriptFilename.c_str();
            return true;
        }
        return false;
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

        auto executeCallback = [this, &context]()
        {
            // set up script's hook callback
            EditorPythonRunnerRequestBus::Broadcast(&EditorPythonRunnerRequestBus::Events::ExecuteByFilename,
                m_scriptFilename.c_str());

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

        if (LoadPython(context.GetScene()))
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

        // This behavior persists on the same AssetBuilder. Clear the script file name so that if
        // this builder processes a scene file with a script file name, and then later processes
        // a scene without a script file name, it won't run the old script on the new scene.
        m_scriptFilename.clear();

        if (action != ManifestAction::Update)
        {
            return Events::ProcessingResult::Ignored;
        }

        if (LoadPython(scene))
        {
            AZStd::string manifestUpdate;
            auto executeCallback = [this, &scene, &manifestUpdate]()
            {
                EditorPythonRunnerRequestBus::Broadcast(&EditorPythonRunnerRequestBus::Events::ExecuteByFilename,
                    m_scriptFilename.c_str());

                ScriptBuildingNotificationBus::BroadcastResult(manifestUpdate, &ScriptBuildingNotificationBus::Events::OnUpdateManifest,
                    scene);
            };

            EditorPythonConsoleNotificationHandler logger;
            m_editorPythonEventsInterface->ExecuteWithLock(executeCallback);

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
        }
        return Events::ProcessingResult::Ignored;
    }

} // namespace AZ
