/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <AtomToolsFramework/Document/AtomToolsAnyDocument.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeUtil.h>
#include <AtomToolsFramework/Graph/GraphDocument.h>
#include <AtomToolsFramework/Graph/GraphDocumentView.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <Document/MaterialGraphCompiler.h>
#include <Editor/MaterialCanvasEditorSystemComponent.h>
#include <Editor/MaterialCanvasPaneWindow.h>
#include <GraphModel/Model/DataType.h>
#include <LyViewPaneNames.h>

#include <QLabel>

namespace MaterialCanvas
{
    // MUST match the standalone application's tool id, which AtomToolsApplication derives from the build target name
    // ("MaterialCanvas", via LY_CMAKE_TARGET). This is not cosmetic and it is not free to change.
    //
    // DynamicNode serializes its tool id into the graph file (DynamicNode::Reflect, Field("toolId")), and on load asks
    // DynamicNodeManagerRequestBus at *that* address for its configuration. A graph saved by the standalone tool therefore
    // carries Crc32("MaterialCanvas") in every node. If the pane registers its node manager under any other id, those lookups
    // reach an address with no handler, every node loses its config, and the graph opens as a field of unknown nodes -- while
    // the node palette still works, because the palette is built from the pane's own manager rather than from the file.
    //
    // Sharing the id is safe because the two never run in the same process: the standalone tool is its own executable, and
    // MaterialCanvasEditorSystemComponent deliberately builds nothing outside NotifyRegisterViews, which only the Editor
    // broadcasts. It is also what makes graphs interchangeable between the two front ends, which is the point.
    const AZ::Crc32 MaterialCanvasEditorSystemComponent::ToolId = AZ_CRC_CE("MaterialCanvas");

    MaterialCanvasEditorSystemComponent* MaterialCanvasEditorSystemComponent::s_instance = nullptr;

    static constexpr const char* MaterialCanvasPaneName = "Material Canvas (Pane)";
    static constexpr AZStd::string_view MaterialCanvasActionContextIdentifier = "o3de.context.editor.materialcanvas";
    static constexpr AZStd::string_view MaterialCanvasSaveActionIdentifier = "o3de.action.materialcanvas.save";

    MaterialCanvasEditorSystemComponent* MaterialCanvasEditorSystemComponent::GetInstance()
    {
        return s_instance;
    }

    void MaterialCanvasEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        MaterialGraphCompiler::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaterialCanvasEditorSystemComponent, AZ::Component>()->Version(0);

            // The standalone application registers these through MaterialCanvasApplication::Reflect. Graph documents
            // serialize slot values into AZStd::any, and the matrix types are stored as arrays of vectors, so the generic
            // types have to be registered before any graph containing a matrix constant can be loaded.
            serialize->RegisterGenericType<AZStd::array<AZ::Vector2, 2>>();
            serialize->RegisterGenericType<AZStd::array<AZ::Vector3, 3>>();
            serialize->RegisterGenericType<AZStd::array<AZ::Vector4, 3>>();
            serialize->RegisterGenericType<AZStd::array<AZ::Vector4, 4>>();
        }
    }

    void MaterialCanvasEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MaterialCanvasEditorService"));
    }

    void MaterialCanvasEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MaterialCanvasEditorService"));
    }

    void MaterialCanvasEditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // The dynamic node manager loads node configurations through the asset system, and the viewport settings system
        // resolves preset assets, so both need the RPI up before this component activates.
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    MaterialCanvasEditorSystemComponent::MaterialCanvasEditorSystemComponent()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    MaterialCanvasEditorSystemComponent::~MaterialCanvasEditorSystemComponent()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
    }

    void MaterialCanvasEditorSystemComponent::OnActionContextRegistrationHook()
    {
        if (auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get())
        {
            AzToolsFramework::ActionContextProperties contextProperties;
            contextProperties.m_name = "O3DE Material Canvas";
            actionManagerInterface->RegisterActionContext(MaterialCanvasActionContextIdentifier, contextProperties);
        }
    }

    void MaterialCanvasEditorSystemComponent::OnActionRegistrationHook()
    {
        if (auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get())
        {
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = "Material Canvas Save";
            actionManagerInterface->RegisterAction(
                MaterialCanvasActionContextIdentifier,
                MaterialCanvasSaveActionIdentifier,
                actionProperties,
                [this]()
                {
                    if (m_paneWindow)
                    {
                        m_paneWindow->SaveCurrentDocument();
                    }
                });

            if (auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get())
            {
                hotKeyManagerInterface->SetActionHotKey(MaterialCanvasSaveActionIdentifier, "Ctrl+S");
            }
        }
    }

    void MaterialCanvasEditorSystemComponent::Activate()
    {
        s_instance = this;

        // Deliberately does no real work. All initialization happens in NotifyRegisterViews, which only the Editor
        // broadcasts. The standalone MaterialCanvas application loads Tools-variant gems as well, so it would otherwise
        // construct a second, entirely unused copy of every system MaterialCanvasApplication already owns.
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void MaterialCanvasEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();

        // Capture the dock layout first, while the widget definitely still exists and is still laid out. CloseViewPane below hands it
        // to Qt to delete, and that deletion can land after this function has already written the registry to disk, which is why the
        // layout never made it into the settings file even on a clean shutdown.
        if (m_paneWindow)
        {
            m_paneWindow->SaveLayout();
        }

        AzToolsFramework::CloseViewPane(MaterialCanvasPaneName);
        AzToolsFramework::UnregisterViewPane(MaterialCanvasPaneName);

        ReleaseSystems();

        // Order matches MaterialCanvasApplication::Destroy: act on the toggles first so the stub files reflect their final state, then
        // write the registry out. ReleaseSystems has already put the graph view configuration into the registry by this point, and the
        // dock layout was captured above, so both are picked up by the save below.
        ApplyShaderBuildSettings();
        ApplyPreviewMaterialPipelineSettings();
        SaveSettings();

        s_instance = nullptr;
    }

    void MaterialCanvasEditorSystemComponent::SaveSettings()
    {
        // The target file and filters match AtomToolsApplication::Destroy so that the pane and the standalone tool read and write the
        // same settings rather than drifting apart. SaveSettingsToFile dumps the whole filtered registry rather than a delta, so
        // whichever of the two exits last still writes a complete file.
        //
        // "/O3DE/Atom/GraphView" is added to the filters. The standalone omits it, which means the graph view configuration it carefully
        // writes to the registry in Destroy is then dropped on the floor by the very next call. Including it here is what actually makes
        // panning, zoom and node palette state survive.
        const AZ::IO::FixedMaxPath settingsFilePath(
            AZStd::string::format("%s/user/Registry/usersettings.materialcanvas.setreg", AZ::Utils::GetProjectPath().c_str()));

        const AZStd::vector<AZStd::string> filters = {
            "/O3DE/AtomToolsFramework", "/O3DE/Atom/Tools", "/O3DE/Atom/GraphView", "/O3DE/Atom/MaterialCanvas"
        };

        if (auto registry = AZ::SettingsRegistry::Get())
        {
            registry->Remove("/O3DE/Atom/MaterialCanvas/PaneWindowState");
        }

        AtomToolsFramework::SaveSettingsToFile(settingsFilePath, filters);
    }

    void MaterialCanvasEditorSystemComponent::ApplyShaderBuildSettings()
    {
        // Reproduced from MaterialCanvasApplication::ApplyShaderBuildSettings. Copying any of these files requires restarting the Editor
        // and the Asset Processor before the change is picked up.
        if (auto fileIO = AZ::IO::FileIOBase::GetInstance())
        {
            const AZ::IO::FixedMaxPath materialCanvasGemPath = AZ::Utils::GetGemPath("MaterialCanvas");
            const auto settingsPathStub(
                materialCanvasGemPath / AZ::SettingsRegistryConstants::RegistryFolder / "user_minimal_shader_build.setregstub");
            const auto settingsPathDx12Stub(
                materialCanvasGemPath / AZ::SettingsRegistryConstants::RegistryFolder / "user_minimal_shader_build_dx12.setregstub");

            const AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
            const auto settingsPath(
                projectPath / AZ::SettingsRegistryConstants::DevUserRegistryFolder / "user_minimal_shader_build.setreg");
            const auto settingsPathDx12(
                projectPath / AZ::SettingsRegistryConstants::DevUserRegistryFolder / "user_minimal_shader_build_dx12.setreg");

            if (AtomToolsFramework::GetSettingsValue<bool>("/O3DE/Atom/MaterialCanvas/EnableFasterShaderBuilds", false))
            {
                // Windows is the only platform with more than one non-null RHI. Whichever is not active has its file removed.
                if (const AZ::Name apiName = AZ::RHI::Factory::Get().GetName(); apiName == AZ::Name("dx12"))
                {
                    fileIO->Copy(settingsPathDx12Stub.c_str(), settingsPathDx12.c_str());
                    fileIO->Remove(settingsPath.c_str());
                }
                else
                {
                    fileIO->Copy(settingsPathStub.c_str(), settingsPath.c_str());
                    fileIO->Remove(settingsPathDx12.c_str());
                }
            }
            else
            {
                fileIO->Remove(settingsPath.c_str());
                fileIO->Remove(settingsPathDx12.c_str());
            }
        }
    }

    void MaterialCanvasEditorSystemComponent::ApplyPreviewMaterialPipelineSettings()
    {
        // The preview-only material pipeline is no longer selected by swapping the project's pipeline list. Material types generated by
        // Material Canvas now declare the preview pipeline themselves, in their build settings, so the choice applies to the graphs being
        // edited instead of to every material type in the project, and it takes effect on the next compile rather than the next restart.
        //
        // All that remains here is removing the settings registry file older builds copied into the project. Left behind it would still
        // replace /O3DE/Atom/RPI/MaterialPipelineFiles for everything, which is exactly the behaviour being retired, and it would do so
        // silently because nothing writes it any more.
        if (auto fileIO = AZ::IO::FileIOBase::GetInstance())
        {
            const AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
            const auto settingsPath(
                projectPath / AZ::SettingsRegistryConstants::DevUserRegistryFolder / "user_preview_material_pipeline.setreg");

            if (fileIO->Exists(settingsPath.c_str()))
            {
                fileIO->Remove(settingsPath.c_str());
            }
        }
    }

    void MaterialCanvasEditorSystemComponent::EnsureSystemsInitialized()
    {
        if (m_dynamicNodeManager)
        {
            return;
        }

        // Inside the Editor the default viewport context belongs to the level viewport. EntityPreviewViewportScene renames
        // its own context to the default name so that frame capture and PostFX can find it, which is correct for a
        // standalone tool that owns the only viewport and actively harmful here. Opt out before any pane viewport exists.
        AtomToolsFramework::SetSettingsValue<bool>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewport/RenameToDefaultViewportContext", false);

        LoadSettings();

        InitDynamicNodeManager();
        InitDynamicNodeEditData();
        InitSharedGraphContext();
        InitGraphViewSettings();

        m_documentSystem.reset(aznew AtomToolsFramework::AtomToolsDocumentSystem(ToolId));

        InitMaterialGraphDocumentType();
        InitMaterialGraphNodeDocumentType();
        InitShaderSourceDataDocumentType();

        m_viewportSettingsSystem.reset(aznew AtomToolsFramework::EntityPreviewViewportSettingsSystem(ToolId));
    }

    void MaterialCanvasEditorSystemComponent::LoadSettings()
    {
        if (auto registry = AZ::SettingsRegistry::Get())
        {
            const AZ::IO::FixedMaxPath settingsFilePath(
                AZStd::string::format("%s/user/Registry/usersettings.materialcanvas.setreg", AZ::Utils::GetProjectPath().c_str()));
            registry->MergeSettingsFile(
                settingsFilePath.c_str(), AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        }
    }

    void MaterialCanvasEditorSystemComponent::ReleaseSystems()
    {
        if (!m_dynamicNodeManager)
        {
            return;
        }

        // Persist the graph view configuration the same way MaterialCanvasApplication::Destroy does, so panning, zoom and
        // node palette state survive closing the pane.
        if (m_graphViewSettingsPtr)
        {
            AtomToolsFramework::SetSettingsObject("/O3DE/Atom/GraphView/ViewSettings", m_graphViewSettingsPtr);
        }

        // Reverse construction order. The document system goes first because open documents hold references to the graph
        // context and the template cache, and because destroying it is what stops any graph compiler still queueing work at
        // the Asset Processor.
        m_documentSystem.reset();
        m_viewportSettingsSystem.reset();
        m_graphViewSettingsPtr.reset();
        m_graphTemplateFileDataCache.reset();
        m_graphContext.reset();

        // Owns a polling thread that talks to the Asset Processor. Leaving it alive after the pane closes is a large part of
        // why the Asset Processor appeared to still have Material Canvas open.
        m_assetStatusReporterSystem.reset();

        m_dynamicNodeManager.reset();
    }

    void MaterialCanvasEditorSystemComponent::NotifyRegisterViews()
    {
        // Broadcast by the Editor once during startup, which makes it the reliable signal that we are actually running
        // inside the Editor. Only the pane registration happens here -- the tool systems are built lazily when the pane is
        // first opened, so an Editor session that never opens Material Canvas pays nothing for it.
        AzToolsFramework::ViewPaneOptions options;
        options.paneRect = QRect(100, 100, 1280, 1024);
        options.showOnToolsToolbar = true;
        options.isPreview = true;
        options.canHaveMultipleInstances = false;
        options.toolbarIcon = ":/Icons/materialtype.svg";

        AzToolsFramework::RegisterViewPane<MaterialCanvasPaneWindow>(
            MaterialCanvasPaneName, LyViewPane::CategoryTools, options);
    }

    void MaterialCanvasEditorSystemComponent::SetPaneWindow(MaterialCanvasPaneWindow* paneWindow)
    {
        m_paneWindow = paneWindow;

        if (!paneWindow)
        {
            // Deferred to the next system tick rather than run inline. This is reached from the pane widget's destructor,
            // and the document system owns views parented to that window, so tearing it down here would destroy objects
            // Qt is still unwinding through. The lambda captures nothing and re-resolves the component, so it cannot
            // dangle if the component is deactivated before the tick arrives.
            AZ::SystemTickBus::QueueFunction(
                []()
                {
                    if (auto systemComponent = MaterialCanvasEditorSystemComponent::GetInstance())
                    {
                        systemComponent->ReleaseSystems();
                    }
                });
        }
    }

    AtomToolsFramework::GraphViewSettingsPtr MaterialCanvasEditorSystemComponent::GetGraphViewSettings()
    {
        EnsureSystemsInitialized();
        return m_graphViewSettingsPtr;
    }

    void MaterialCanvasEditorSystemComponent::InitDynamicNodeManager()
    {
        m_dynamicNodeManager.reset(aznew AtomToolsFramework::DynamicNodeManager(ToolId));

        AZ::RHI::SamplerState defaultSamplerState{};
        defaultSamplerState.m_filterMin = AZ::RHI::FilterMode::Linear;
        defaultSamplerState.m_filterMag = AZ::RHI::FilterMode::Linear;
        defaultSamplerState.m_filterMip = AZ::RHI::FilterMode::Linear;
        defaultSamplerState.m_anisotropyMax = 16;

        // Mirrors MaterialCanvasApplication::InitDynamicNodeManager. Any change there must be repeated here.
        m_dynamicNodeManager->RegisterDataTypes({
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("bool"), bool{}, "bool"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("int"), int32_t{}, "int"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("uint"), uint32_t{}, "uint"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float"), float{}, "float"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float2"), AZ::Vector2{}, "float2"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float3"), AZ::Vector3{}, "float3"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float4"), AZ::Vector4{}, "float4"),
            AZStd::make_shared<GraphModel::DataType>(
                AZ_CRC_CE("float2x2"),
                AZStd::array<AZ::Vector2, 2>{ AZ::Vector2(1.0f, 0.0f), AZ::Vector2(0.0f, 1.0f) },
                "float2x2"),
            AZStd::make_shared<GraphModel::DataType>(
                AZ_CRC_CE("float3x3"),
                AZStd::array<AZ::Vector3, 3>{ AZ::Vector3(1.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 1.0f, 0.0f),
                                              AZ::Vector3(0.0f, 0.0f, 1.0f) },
                "float3x3"),
            AZStd::make_shared<GraphModel::DataType>(
                AZ_CRC_CE("float4x3"),
                AZStd::array<AZ::Vector4, 3>{ AZ::Vector4(1.0f, 0.0f, 0.0f, 0.0f), AZ::Vector4(0.0f, 1.0f, 0.0f, 0.0f),
                                              AZ::Vector4(0.0f, 0.0f, 1.0f, 0.0f) },
                "float4x3"),
            AZStd::make_shared<GraphModel::DataType>(
                AZ_CRC_CE("float4x4"),
                AZStd::array<AZ::Vector4, 4>{ AZ::Vector4(1.0f, 0.0f, 0.0f, 0.0f), AZ::Vector4(0.0f, 1.0f, 0.0f, 0.0f),
                                              AZ::Vector4(0.0f, 0.0f, 1.0f, 0.0f), AZ::Vector4(0.0f, 0.0f, 0.0f, 1.0f) },
                "float4x4"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("color"), AZ::Color::CreateOne(), "color"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("string"), AZStd::string{}, "string"),
            AZStd::make_shared<GraphModel::DataType>(
                AZ_CRC_CE("image"),
                AZ::Data::Asset<AZ::RPI::StreamingImageAsset>{ AZ::Data::AssetLoadBehavior::NoLoad },
                "image"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("sampler"), defaultSamplerState, "sampler"),
        });

        // Scans the project and all enabled gems for .materialgraphnode files, which is how the node palette is populated.
        // Both this and the standalone tool read the same configs out of the MaterialCanvas gem, so the node library never
        // needs duplicating.
        m_dynamicNodeManager->LoadConfigFiles("materialgraphnode");
    }

    void MaterialCanvasEditorSystemComponent::InitDynamicNodeEditData()
    {
        AZ::Edit::ElementData editData;
        editData.m_elementId = AZ_CRC_CE("MultilineStringDialog");
        m_dynamicNodeManager->RegisterEditDataForSetting("instructions", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("classDefinitions", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("functionDefinitions", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertySrgMember", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyDescription", editData);

        editData = {};
        editData.m_elementId = AZ::Edit::UIHandlers::LineEdit;
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyName", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyDisplayName", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyConnectionName", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyGroupName", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyGroup", editData);

        editData = {};
        editData.m_elementId = AZ::Edit::UIHandlers::ComboBox;
        AtomToolsFramework::AddEditDataAttribute(
            editData, AZ::Edit::Attributes::StringList, AZStd::vector<AZStd::string>{ "", "0", "1", "2", "3", "4" });
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyMinVectorSize", editData);

        editData = {};
        editData.m_elementId = AZ::Edit::UIHandlers::ComboBox;
        AtomToolsFramework::AddEditDataAttribute(
            editData,
            AZ::Edit::Attributes::StringList,
            AZStd::vector<AZStd::string>{ "None", "ShaderInput", "ShaderOption", "ShaderEnabled", "InternalProperty", "" });
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyConnectionType", editData);

        editData = {};
        editData.m_elementId = AZ_CRC_CE("StringFilePath");
        AtomToolsFramework::AddEditDataAttribute(editData, AZ_CRC_CE("Title"), AZStd::string("Template File"));
        AtomToolsFramework::AddEditDataAttribute(
            editData,
            AZ_CRC_CE("Extensions"),
            AZStd::vector<AZStd::string>{ "azsl", "azsli", "material", "materialtype", "shader" });
        m_dynamicNodeManager->RegisterEditDataForSetting("templatePaths", editData);

        editData = {};
        editData.m_elementId = AZ_CRC_CE("StringFilePath");
        AtomToolsFramework::AddEditDataAttribute(editData, AZ_CRC_CE("Title"), AZStd::string("Include File"));
        AtomToolsFramework::AddEditDataAttribute(editData, AZ_CRC_CE("Extensions"), AZStd::vector<AZStd::string>{ "azsli" });
        m_dynamicNodeManager->RegisterEditDataForSetting("includePaths", editData);
    }

    void MaterialCanvasEditorSystemComponent::InitSharedGraphContext()
    {
        m_graphContext = AZStd::make_shared<GraphModel::GraphContext>(
            "Material Graph", ".materialgraph", m_dynamicNodeManager->GetRegisteredDataTypes());
        m_graphContext->CreateModuleGraphManager();
    }

    void MaterialCanvasEditorSystemComponent::InitGraphViewSettings()
    {
        m_graphViewSettingsPtr = AtomToolsFramework::GetSettingsObject(
            "/O3DE/Atom/GraphView/ViewSettings", AZStd::make_shared<AtomToolsFramework::GraphViewSettings>());

        m_graphViewSettingsPtr->m_translationPath = "@products@/materialcanvas/translation/materialcanvas_en_us.qm";
        m_graphViewSettingsPtr->m_styleManagerPath = "MaterialCanvas/StyleSheet/materialcanvas_style.json";
        m_graphViewSettingsPtr->m_nodeMimeType = "MaterialCanvas/node-palette-mime-event";
        m_graphViewSettingsPtr->m_nodeSaveIdentifier = "MaterialCanvas/ContextMenu";
        m_graphViewSettingsPtr->m_createNodeTreeItemsFn = [](const AZ::Crc32& toolId)
        {
            GraphCanvas::GraphCanvasTreeItem* rootTreeItem = {};
            AtomToolsFramework::DynamicNodeManagerRequestBus::EventResult(
                rootTreeItem, toolId, &AtomToolsFramework::DynamicNodeManagerRequestBus::Events::CreateNodePaletteTree);
            return rootTreeItem;
        };

        const AZStd::map<AZStd::string, AZ::Color> defaultGroupPresets = AtomToolsFramework::GetSettingsObject(
            "/O3DE/Atom/GraphView/DefaultGroupPresets",
            AZStd::map<AZStd::string, AZ::Color>{ { "Logic", AZ::Color(0.188f, 0.972f, 0.243f, 1.0f) },
                                                  { "Function", AZ::Color(0.396f, 0.788f, 0.788f, 1.0f) },
                                                  { "Output", AZ::Color(0.866f, 0.498f, 0.427f, 1.0f) },
                                                  { "Input", AZ::Color(0.396f, 0.788f, 0.549f, 1.0f) } });

        m_graphViewSettingsPtr->Initialize(ToolId, defaultGroupPresets);
    }

    void MaterialCanvasEditorSystemComponent::InitMaterialGraphDocumentType()
    {
        m_assetStatusReporterSystem.reset(aznew AtomToolsFramework::AssetStatusReporterSystem(ToolId));
        m_graphTemplateFileDataCache.reset(aznew AtomToolsFramework::GraphTemplateFileDataCache(ToolId));

        auto documentTypeInfo = AtomToolsFramework::GraphDocument::BuildDocumentTypeInfo(
            "Material Graph",
            { "materialgraph" },
            { "materialgraphtemplate" },
            AtomToolsFramework::GetPathWithoutAlias(AtomToolsFramework::GetSettingsValue<AZStd::string>(
                "/O3DE/Atom/MaterialCanvas/DefaultMaterialGraphTemplate",
                "@gemroot:MaterialCanvas@/Assets/MaterialCanvas/GraphData/blank_graph.materialgraphtemplate")),
            m_graphContext,
            [](){ return AZStd::make_shared<MaterialGraphCompiler>(ToolId); });

        // Unlike the standalone application, the window is owned by the Editor and only exists while the pane is open, so the
        // factory has to resolve it at call time and cope with it being absent.
        documentTypeInfo.m_documentViewFactoryCallback = [this](const AZ::Crc32& toolId, const AZ::Uuid& documentId)
        {
            if (!m_paneWindow)
            {
                AZ_Warning("MaterialCanvasEditor", false, "Cannot create a document view while the pane is closed.");
                return false;
            }

            return m_paneWindow->AddDocumentView(
                documentId,
                aznew AtomToolsFramework::GraphDocumentView(toolId, documentId, m_graphViewSettingsPtr, m_paneWindow));
        };

        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
            ToolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::RegisterDocumentType, documentTypeInfo);
    }

    void MaterialCanvasEditorSystemComponent::InitMaterialGraphNodeDocumentType()
    {
        auto documentTypeInfo = AtomToolsFramework::AtomToolsAnyDocument::BuildDocumentTypeInfo(
            "Material Graph Node Config",
            { "materialgraphnode" },
            { "materialgraphnodetemplate" },
            AZStd::any(AtomToolsFramework::DynamicNodeConfig()),
            AZ::Uuid::CreateNull());

        documentTypeInfo.m_documentViewFactoryCallback = [this]([[maybe_unused]] const AZ::Crc32& toolId, const AZ::Uuid& documentId)
        {
            if (!m_paneWindow)
            {
                return false;
            }

            auto viewWidget = new QLabel("Material Graph Node Config properties can be edited in the inspector.", m_paneWindow);
            viewWidget->setAlignment(Qt::AlignCenter);
            return m_paneWindow->AddDocumentView(documentId, viewWidget);
        };

        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
            ToolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::RegisterDocumentType, documentTypeInfo);
    }

    void MaterialCanvasEditorSystemComponent::InitShaderSourceDataDocumentType()
    {
        auto documentTypeInfo = AtomToolsFramework::AtomToolsAnyDocument::BuildDocumentTypeInfo(
            "Shader Source Data",
            { "shader" },
            {},
            AZStd::any(AZ::RPI::ShaderSourceData()),
            AZ::RPI::ShaderSourceData::TYPEINFO_Uuid());

        documentTypeInfo.m_documentViewFactoryCallback = [this]([[maybe_unused]] const AZ::Crc32& toolId, const AZ::Uuid& documentId)
        {
            if (!m_paneWindow)
            {
                return false;
            }

            auto viewWidget = new QLabel("Shader Source Data properties can be edited in the inspector.", m_paneWindow);
            viewWidget->setAlignment(Qt::AlignCenter);
            return m_paneWindow->AddDocumentView(documentId, viewWidget);
        };

        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
            ToolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::RegisterDocumentType, documentTypeInfo);
    }
} // namespace MaterialCanvas
