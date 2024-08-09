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
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Document/MaterialGraphCompiler.h>
#include <GraphModel/Model/DataType.h>
#include <MaterialCanvasApplication.h>
#include <Window/MaterialCanvasMainWindow.h>

#include <QLabel>

void InitMaterialCanvasResources()
{
    // Must register qt resources from other modules
    Q_INIT_RESOURCE(MaterialCanvas);
    Q_INIT_RESOURCE(InspectorWidget);
    Q_INIT_RESOURCE(AtomToolsAssetBrowser);
    Q_INIT_RESOURCE(GraphView);
}

namespace MaterialCanvas
{
    static const char* GetBuildTargetName()
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return LY_CMAKE_TARGET;
    }

    MaterialCanvasApplication::MaterialCanvasApplication(int* argc, char*** argv)
        : Base(GetBuildTargetName(), argc, argv)
    {
        InitMaterialCanvasResources();

        QApplication::setOrganizationName("O3DE");
        QApplication::setApplicationName("O3DE Material Canvas");
        QApplication::setWindowIcon(QIcon(":/Icons/application.svg"));

        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
        AZ::RHI::FactoryManagerNotificationBus::Handler::BusConnect();
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
    }

    MaterialCanvasApplication::~MaterialCanvasApplication()
    {
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();
        AZ::RHI::FactoryManagerNotificationBus::Handler::BusDisconnect();
        m_window.reset();
    }

    void MaterialCanvasApplication::Reflect(AZ::ReflectContext* context)
    {
        Base::Reflect(context);
        MaterialGraphCompiler::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->RegisterGenericType<AZStd::array<AZ::Vector2, 2>>();
            serialize->RegisterGenericType<AZStd::array<AZ::Vector3, 3>>();
            serialize->RegisterGenericType<AZStd::array<AZ::Vector4, 3>>();
            serialize->RegisterGenericType<AZStd::array<AZ::Vector4, 4>>();
        }
    }

    const char* MaterialCanvasApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseMaterialCanvas";
#elif defined(_DEBUG)
        return "DebugMaterialCanvas";
#else
        return "ProfileMaterialCanvas";
#endif
    }

    void MaterialCanvasApplication::StartCommon(AZ::Entity* systemEntity)
    {
        Base::StartCommon(systemEntity);

        InitDynamicNodeManager();
        InitDynamicNodeEditData();
        InitSharedGraphContext();
        InitGraphViewSettings();
        InitMaterialGraphDocumentType();
        InitMaterialGraphNodeDocumentType();
        InitShaderSourceDataDocumentType();
        InitMainWindow();
        InitDefaultDocument();
    }

    void MaterialCanvasApplication::Destroy()
    {
        // Save all of the graph view configuration settings to the settings registry.
        AtomToolsFramework::SetSettingsObject("/O3DE/Atom/GraphView/ViewSettings", m_graphViewSettingsPtr);

        m_graphViewSettingsPtr.reset();
        m_window.reset();
        m_viewportSettingsSystem.reset();
        m_graphContext.reset();
        m_graphTemplateFileDataCache.reset();
        m_dynamicNodeManager.reset();

        ApplyShaderBuildSettings();
        Base::Destroy();
    }

    AZStd::vector<AZStd::string> MaterialCanvasApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/", "MaterialEditor/", "MaterialCanvas/" });
    }

    QWidget* MaterialCanvasApplication::GetAppMainWindow()
    {
        return m_window.get();
    }

    void MaterialCanvasApplication::FactoryRegistered()
    {
        ApplyShaderBuildSettings();
    }

    void MaterialCanvasApplication::InitDynamicNodeManager()
    {
        // Instantiate the dynamic node manager to register all dynamic node configurations and data types used in this tool
        m_dynamicNodeManager.reset(aznew AtomToolsFramework::DynamicNodeManager(m_toolId));

        // Creating default sampler state with settings common to pre-existing material types.
        AZ::RHI::SamplerState defaultSamplerState{};
        defaultSamplerState.m_filterMin = AZ::RHI::FilterMode::Linear;
        defaultSamplerState.m_filterMag = AZ::RHI::FilterMode::Linear;
        defaultSamplerState.m_filterMip = AZ::RHI::FilterMode::Linear;
        defaultSamplerState.m_anisotropyMax = 16;

        // Register all data types required by Material Canvas nodes with the dynamic node manager
        m_dynamicNodeManager->RegisterDataTypes({
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("bool"), bool{}, "bool"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("int"), int32_t{}, "int"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("uint"), uint32_t{}, "uint"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float"), float{}, "float"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float2"), AZ::Vector2{}, "float2"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float3"), AZ::Vector3{}, "float3"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float4"), AZ::Vector4{}, "float4"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float2x2"), AZStd::array<AZ::Vector2, 2>{ AZ::Vector2(1.0f, 0.0f), AZ::Vector2(0.0f, 1.0f) }, "float2x2"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float3x3"), AZStd::array<AZ::Vector3, 3>{ AZ::Vector3(1.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 1.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 1.0f) }, "float3x3"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float4x3"), AZStd::array<AZ::Vector4, 3>{ AZ::Vector4(1.0f, 0.0f, 0.0f, 0.0f), AZ::Vector4(0.0f, 1.0f, 0.0f, 0.0f), AZ::Vector4(0.0f, 0.0f, 1.0f, 0.0f) }, "float4x3"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float4x4"), AZStd::array<AZ::Vector4, 4>{ AZ::Vector4(1.0f, 0.0f, 0.0f, 0.0f), AZ::Vector4(0.0f, 1.0f, 0.0f, 0.0f), AZ::Vector4(0.0f, 0.0f, 1.0f, 0.0f), AZ::Vector4(0.0f, 0.0f, 0.0f, 1.0f) }, "float4x4"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("color"), AZ::Color::CreateOne(), "color"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("string"), AZStd::string{}, "string"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("image"), AZ::Data::Asset<AZ::RPI::StreamingImageAsset>{ AZ::Data::AssetLoadBehavior::NoLoad }, "image"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("sampler"), defaultSamplerState, "sampler"),
        });

        // Search the project and gems for dynamic node configurations and register them with the manager
        m_dynamicNodeManager->LoadConfigFiles("materialgraphnode");
    }

    void MaterialCanvasApplication::InitDynamicNodeEditData()
    {
        // Registering custom property handlers for dynamic node configuration settings. The settings are just a map of string data.
        // Recognized settings will need special controls for selecting files or editing large blocks of text without taking up much real
        // estate in the property editor. In the future, this will likely be replaced with a more specialized node configuration editor. 
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
            editData, AZ_CRC_CE("Extensions"), AZStd::vector<AZStd::string>{ "azsl", "azsli", "material", "materialtype", "shader" });
        m_dynamicNodeManager->RegisterEditDataForSetting("templatePaths", editData);

        editData = {};
        editData.m_elementId = AZ_CRC_CE("StringFilePath");
        AtomToolsFramework::AddEditDataAttribute(editData, AZ_CRC_CE("Title"), AZStd::string("Include File"));
        AtomToolsFramework::AddEditDataAttribute(editData, AZ_CRC_CE("Extensions"), AZStd::vector<AZStd::string>{ "azsli" });
        m_dynamicNodeManager->RegisterEditDataForSetting("includePaths", editData);
    }

    void MaterialCanvasApplication::InitSharedGraphContext()
    {
        // Each graph document creates its own graph context but we want to use a shared graph context instead to avoid data duplication
        m_graphContext = AZStd::make_shared<GraphModel::GraphContext>(
            "Material Graph", ".materialgraph", m_dynamicNodeManager->GetRegisteredDataTypes());
        m_graphContext->CreateModuleGraphManager();
    }

    void MaterialCanvasApplication::InitGraphViewSettings()
    {
        // This configuration data is passed through the main window and graph views to setup translation data, styling, and node palettes
        m_graphViewSettingsPtr = AtomToolsFramework::GetSettingsObject(
            "/O3DE/Atom/GraphView/ViewSettings", AZStd::make_shared<AtomToolsFramework::GraphViewSettings>());

        // Initialize the application specific graph view settings that are not serialized.
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

        // Initialize the default group preset names and colors needed by the graph canvas view to create node groups.
        const AZStd::map<AZStd::string, AZ::Color> defaultGroupPresets = AtomToolsFramework::GetSettingsObject(
            "/O3DE/Atom/GraphView/DefaultGroupPresets",
            AZStd::map<AZStd::string, AZ::Color>{ { "Logic", AZ::Color(0.188f, 0.972f, 0.243f, 1.0f) },
                                                  { "Function", AZ::Color(0.396f, 0.788f, 0.788f, 1.0f) },
                                                  { "Output", AZ::Color(0.866f, 0.498f, 0.427f, 1.0f) },
                                                  { "Input", AZ::Color(0.396f, 0.788f, 0.549f, 1.0f) } });

        // Connect the graph view settings to the required buses so that they can be accessed throughout the application.
        m_graphViewSettingsPtr->Initialize(m_toolId, defaultGroupPresets);
    }

    void MaterialCanvasApplication::InitMaterialGraphDocumentType()
    {
        // Initialize system to asynchronously report material and shader related asset processing status for open documents
        m_assetStatusReporterSystem.reset(aznew AtomToolsFramework::AssetStatusReporterSystem(m_toolId));

        // Initialize system to load and store material graph template files and only reload them if modified
        m_graphTemplateFileDataCache.reset(aznew AtomToolsFramework::GraphTemplateFileDataCache(m_toolId));

        // Acquiring default Material Canvas document type info so that it can be customized before registration
        auto documentTypeInfo = AtomToolsFramework::GraphDocument::BuildDocumentTypeInfo(
            "Material Graph",
            { "materialgraph" },
            { "materialgraphtemplate" },
            AtomToolsFramework::GetPathWithoutAlias(AtomToolsFramework::GetSettingsValue<AZStd::string>(
                "/O3DE/Atom/MaterialCanvas/DefaultMaterialGraphTemplate",
                "@gemroot:MaterialCanvas@/Assets/MaterialCanvas/GraphData/blank_graph.materialgraphtemplate")),
            m_graphContext,
            [toolId = m_toolId](){ return AZStd::make_shared<MaterialGraphCompiler>(toolId); });

        // Overriding documentview factory function to create graph view
        documentTypeInfo.m_documentViewFactoryCallback = [this](const AZ::Crc32& toolId, const AZ::Uuid& documentId)
        {
            m_window->AddDocumentTab(
                documentId, aznew AtomToolsFramework::GraphDocumentView(toolId, documentId, m_graphViewSettingsPtr, m_window.get()));
            return true;
        };

        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType, documentTypeInfo);
    }

    void MaterialCanvasApplication::InitMaterialGraphNodeDocumentType()
    {
        // Register document type for editing Material Canvas node configurations. This document type does not have a central view widget
        // and will show a label directing users to the inspector.
        auto documentTypeInfo = AtomToolsFramework::AtomToolsAnyDocument::BuildDocumentTypeInfo(
            "Material Graph Node Config",
            { "materialgraphnode" },
            { "materialgraphnodetemplate" },
            AZStd::any(AtomToolsFramework::DynamicNodeConfig()),
            AZ::Uuid::CreateNull()); // Null ID because JSON file contains type info and can be loaded directly into AZStd::any

        documentTypeInfo.m_documentViewFactoryCallback = [this]([[maybe_unused]] const AZ::Crc32& toolId, const AZ::Uuid& documentId)
        {
            auto viewWidget = new QLabel("Material Graph Node Config properties can be edited in the inspector.", m_window.get());
            viewWidget->setAlignment(Qt::AlignCenter);
            return m_window->AddDocumentTab(documentId, viewWidget);
        };

        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType, documentTypeInfo);
    }

    void MaterialCanvasApplication::InitShaderSourceDataDocumentType()
    {
        // Register document type for editing shader source data files. This document type does not have a central view
        // and will display a label widget that directs users to edit using the inspector.
        auto documentTypeInfo = AtomToolsFramework::AtomToolsAnyDocument::BuildDocumentTypeInfo(
            "Shader Source Data",
            { "shader" },
            {},
            AZStd::any(AZ::RPI::ShaderSourceData()),
            AZ::RPI::ShaderSourceData::TYPEINFO_Uuid()); // Supplying ID because it is not included in the JSON file

        documentTypeInfo.m_documentViewFactoryCallback = [this]([[maybe_unused]] const AZ::Crc32& toolId, const AZ::Uuid& documentId)
        {
            auto viewWidget = new QLabel("Shader Source Data properties can be edited in the inspector.", m_window.get());
            viewWidget->setAlignment(Qt::AlignCenter);
            return m_window->AddDocumentTab(documentId, viewWidget);
        };

        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType, documentTypeInfo);
    }

    void MaterialCanvasApplication::InitMainWindow()
    {
        m_viewportSettingsSystem.reset(aznew AtomToolsFramework::EntityPreviewViewportSettingsSystem(m_toolId));

        m_window.reset(aznew MaterialCanvasMainWindow(m_toolId, m_graphViewSettingsPtr));
        m_window->show();
    }

    void MaterialCanvasApplication::InitDefaultDocument()
    {
        // Create an untitled, empty graph document as soon as the application starts so the user can begin creating immediately.
        if (AtomToolsFramework::GetSettingsValue("/O3DE/Atom/MaterialCanvas/CreateDefaultDocumentOnStart", true))
        {
            AZ::Uuid documentId = AZ::Uuid::CreateNull();
            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::EventResult(
                documentId,
                m_toolId,
                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::CreateDocumentFromTypeName,
                "Material Graph");

            AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
                m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::OnDocumentOpened, documentId);
        }
    }

    void MaterialCanvasApplication::ApplyShaderBuildSettings()
    {
        // If faster shader build settings are enabled, copy a settings registry file stub into the user settings folder. This will
        // override AP and shader build settings, disabling shaders and shader variant building for inactive platforms and RHI. Copying any
        // of these settings files requires restarting the application and the asset processor for the changes to be picked up.
        if (auto fileIO = AZ::IO::FileIOBase::GetInstance())
        {
            const AZ::IO::FixedMaxPath materialCanvasGemPath = AZ::Utils::GetGemPath("MaterialCanvas");
            const auto settingsPathStub(
                materialCanvasGemPath / AZ::SettingsRegistryInterface::RegistryFolder / "user_minimal_shader_build.setregstub");
            const auto settingsPathDx12Stub(
                materialCanvasGemPath / AZ::SettingsRegistryInterface::RegistryFolder / "user_minimal_shader_build_dx12.setregstub");

            const AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
            const auto settingsPath(
                projectPath / AZ::SettingsRegistryInterface::DevUserRegistryFolder / "user_minimal_shader_build.setreg");
            const auto settingsPathDx12(
                projectPath / AZ::SettingsRegistryInterface::DevUserRegistryFolder / "user_minimal_shader_build_dx12.setreg");

            const bool enableFasterShaderBuilds =
                AtomToolsFramework::GetSettingsValue<bool>("/O3DE/Atom/MaterialCanvas/EnableFasterShaderBuilds", false);

            if (enableFasterShaderBuilds)
            {
                // Windows is the only platform with multiple, non-null RHI, supporting Vulkan and DX12. If DX12 is the active RHI then it
                // will require copying its own settings file. Settings files for inactive RHI will be deleted from the user folder. 
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
} // namespace MaterialCanvas
