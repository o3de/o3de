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
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodePaletteItem.h>
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
    // Must match the standalone tool id ("MaterialCanvas"): DynamicNode serializes it into graphs and looks up its config there.
    const AZ::Crc32 MaterialCanvasEditorSystemComponent::ToolId = AZ_CRC_CE("MaterialCanvas");

    MaterialCanvasEditorSystemComponent* MaterialCanvasEditorSystemComponent::s_instance = nullptr;

    static constexpr const char* MaterialCanvasPaneName = "Material Canvas (Pane)";
    static constexpr AZStd::string_view MaterialCanvasActionContextIdentifier = "o3de.context.editor.materialcanvas";
    static constexpr AZStd::string_view MaterialCanvasSaveActionIdentifier = "o3de.action.materialcanvas.save";
    static constexpr AZStd::string_view MaterialCanvasAddRerouteActionIdentifier = "o3de.action.materialcanvas.addReroute";

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

            // Registered by MaterialCanvasApplication::Reflect in the standalone tool; needed to load graphs with matrix constants.
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
        // The node manager and viewport settings load assets, so the RPI must be up first.
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

            actionProperties.m_name = "Add Reroute to Selected Connection";
            actionProperties.m_description = "Insert a universal reroute at the midpoint of the selected Material Canvas connection.";
            actionProperties.m_category = "Material Canvas";
            actionManagerInterface->RegisterAction(
                MaterialCanvasActionContextIdentifier,
                MaterialCanvasAddRerouteActionIdentifier,
                actionProperties,
                [this]()
                {
                    if (m_paneWindow)
                    {
                        m_paneWindow->AddRerouteToSelectedConnection();
                    }
                });

            if (auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get())
            {
                hotKeyManagerInterface->SetActionHotKey(MaterialCanvasSaveActionIdentifier, "Ctrl+S");
                hotKeyManagerInterface->SetActionHotKey(MaterialCanvasAddRerouteActionIdentifier, "R");
            }
        }
    }

    void MaterialCanvasEditorSystemComponent::Activate()
    {
        s_instance = this;

        // Deliberately no work here: the standalone app loads Tools gems too, so initialization waits for NotifyRegisterViews.
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void MaterialCanvasEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();

        // Capture the dock layout while the widget still exists; Qt may delete it after the registry is written.
        if (m_paneWindow)
        {
            m_paneWindow->SaveLayout();
        }

        AzToolsFramework::CloseViewPane(MaterialCanvasPaneName);
        AzToolsFramework::UnregisterViewPane(MaterialCanvasPaneName);

        ReleaseSystems();

        // Same order as MaterialCanvasApplication::Destroy: apply the toggles, then save the registry.
        ApplyShaderBuildSettings();
        ApplyPreviewMaterialPipelineSettings();
        SaveSettings();

        s_instance = nullptr;
    }

    void MaterialCanvasEditorSystemComponent::SaveSettings()
    {
        // Same file and filters as AtomToolsApplication::Destroy, plus "/O3DE/Atom/GraphView" so graph view state actually persists.
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
        // Mirrors MaterialCanvasApplication::ApplyShaderBuildSettings; changes need an Editor and Asset Processor restart.
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
        // Material types now declare the preview pipeline themselves; just remove the setreg older builds copied into the project.
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

        // The Editor's level viewport owns the default viewport context, so opt out of renaming before a pane viewport exists.
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

        // Persist graph view configuration as MaterialCanvasApplication::Destroy does, so panning, zoom and palette state survive.
        if (m_graphViewSettingsPtr)
        {
            AtomToolsFramework::SetSettingsObject("/O3DE/Atom/GraphView/ViewSettings", m_graphViewSettingsPtr);
        }

        // Reverse construction order; the document system first, since documents reference the context and it stops compiles.
        m_documentSystem.reset();
        m_viewportSettingsSystem.reset();
        m_graphViewSettingsPtr.reset();
        m_graphTemplateFileDataCache.reset();
        m_graphContext.reset();

        // Owns an Asset Processor polling thread, so don't leave it alive after the pane closes.
        m_assetStatusReporterSystem.reset();

        m_dynamicNodeManager.reset();
    }

    void MaterialCanvasEditorSystemComponent::NotifyRegisterViews()
    {
        // Broadcast once by the Editor at startup; only register the pane here, the tool systems are built when it first opens.
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
            // Deferred a tick: this runs from the pane's destructor, and the lambda re-resolves the component so it can't dangle.
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

        // Load .materialgraphnode configs from the project and gems, the same node library the standalone tool uses.
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
        m_graphViewSettingsPtr->m_createSplicingNodeActionName = "Reroute";
        m_graphViewSettingsPtr->m_createSplicingNodeMimeEventFn = []()
        {
            return aznew AtomToolsFramework::CreateDynamicNodeMimeEvent(
                ToolId, AZ::Uuid::CreateString("{A4D0A1B1-0E1C-4E3B-9E5A-000000000013}"));
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

        // The Editor owns the window and it only exists while the pane is open, so resolve it at call time.
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
