/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
#include <Document/PassGraphCompiler.h>
#include <GraphModel/Model/DataType.h>
#include <PassCanvasApplication.h>
#include <Window/PassCanvasMainWindow.h>

#include <QLabel>

void InitPassCanvasResources()
{
    // Must register qt resources from other modules
    Q_INIT_RESOURCE(PassCanvas);
    Q_INIT_RESOURCE(InspectorWidget);
    Q_INIT_RESOURCE(AtomToolsAssetBrowser);
    Q_INIT_RESOURCE(GraphView);
}

namespace PassCanvas
{
    static const char* GetBuildTargetName()
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return LY_CMAKE_TARGET;
    }

    PassCanvasApplication::PassCanvasApplication(int* argc, char*** argv)
        : Base(GetBuildTargetName(), argc, argv)
    {
        InitPassCanvasResources();

        QApplication::setOrganizationName("O3DE");
        QApplication::setApplicationName("O3DE Pass Canvas");
        QApplication::setWindowIcon(QIcon(":/Icons/application.svg"));

        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
    }

    PassCanvasApplication::~PassCanvasApplication()
    {
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();
        m_window.reset();
    }

    void PassCanvasApplication::Reflect(AZ::ReflectContext* context)
    {
        Base::Reflect(context);
        PassGraphCompiler::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->RegisterGenericType<AZStd::array<AZ::Vector2, 2>>();
            serialize->RegisterGenericType<AZStd::array<AZ::Vector3, 3>>();
            serialize->RegisterGenericType<AZStd::array<AZ::Vector4, 3>>();
            serialize->RegisterGenericType<AZStd::array<AZ::Vector4, 4>>();
        }
    }

    const char* PassCanvasApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleasePassCanvas";
#elif defined(_DEBUG)
        return "DebugPassCanvas";
#else
        return "ProfilePassCanvas";
#endif
    }

    void PassCanvasApplication::StartCommon(AZ::Entity* systemEntity)
    {
        Base::StartCommon(systemEntity);

        InitDynamicNodeManager();
        InitDynamicNodeEditData();
        InitSharedGraphContext();
        InitGraphViewSettings();
        InitPassGraphDocumentType();
        InitMainWindow();
        InitDefaultDocument();
    }

    void PassCanvasApplication::Destroy()
    {
        // Save all of the graph view configuration settings to the settings registry.
        AtomToolsFramework::SetSettingsObject("/O3DE/Atom/GraphView/ViewSettings", m_graphViewSettingsPtr);

        m_graphViewSettingsPtr.reset();
        m_window.reset();
        m_viewportSettingsSystem.reset();
        m_graphContext.reset();
        m_dynamicNodeManager.reset();

        Base::Destroy();
    }

    AZStd::vector<AZStd::string> PassCanvasApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/", "MaterialEditor/", "PassCanvas/" });
    }

    QWidget* PassCanvasApplication::GetAppMainWindow()
    {
        return m_window.get();
    }

    void PassCanvasApplication::InitDynamicNodeManager()
    {
        // Instantiate the dynamic node manager to register all dynamic node configurations and data types used in this tool
        m_dynamicNodeManager.reset(aznew AtomToolsFramework::DynamicNodeManager(m_toolId));

        // Register all data types required by Pass Canvas nodes with the dynamic node manager
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
        });

        // Search the project and gems for dynamic node configurations and register them with the manager
        m_dynamicNodeManager->LoadConfigFiles("passgraphnode");
    }

    void PassCanvasApplication::InitDynamicNodeEditData()
    {
    }

    void PassCanvasApplication::InitSharedGraphContext()
    {
        // Each graph document creates its own graph context but we want to use a shared graph context instead to avoid data duplication
        m_graphContext = AZStd::make_shared<GraphModel::GraphContext>(
            "Pass Graph", ".passgraph", m_dynamicNodeManager->GetRegisteredDataTypes());
        m_graphContext->CreateModuleGraphManager();
    }

    void PassCanvasApplication::InitGraphViewSettings()
    {
        // This configuration data is passed through the main window and graph views to setup translation data, styling, and node palettes
        m_graphViewSettingsPtr = AtomToolsFramework::GetSettingsObject(
            "/O3DE/Atom/GraphView/ViewSettings", AZStd::make_shared<AtomToolsFramework::GraphViewSettings>());

        // Initialize the application specific graph view settings that are not serialized.
        m_graphViewSettingsPtr->m_translationPath = "@products@/passcanvas/translation/passcanvas_en_us.qm";
        m_graphViewSettingsPtr->m_styleManagerPath = "PassCanvas/StyleSheet/passcanvas_style.json";
        m_graphViewSettingsPtr->m_nodeMimeType = "PassCanvas/node-palette-mime-event";
        m_graphViewSettingsPtr->m_nodeSaveIdentifier = "PassCanvas/ContextMenu";
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
            AZStd::map<AZStd::string, AZ::Color>{});

        // Connect the graph view settings to the required buses so that they can be accessed throughout the application.
        m_graphViewSettingsPtr->Initialize(m_toolId, defaultGroupPresets);
    }

    void PassCanvasApplication::InitPassGraphDocumentType()
    {
        // Acquiring default Pass Canvas document type info so that it can be customized before registration
        auto documentTypeInfo = AtomToolsFramework::GraphDocument::BuildDocumentTypeInfo(
            "Pass Graph",
            { "passgraph" },
            { "passgraphtemplate" },
            AtomToolsFramework::GetPathWithoutAlias(AtomToolsFramework::GetSettingsValue<AZStd::string>(
                "/O3DE/Atom/PassCanvas/DefaultPassGraphTemplate",
                "@gemroot:PassCanvas@/Assets/PassCanvas/GraphData/blank_graph.passgraphtemplate")),
            m_graphContext,
            [toolId = m_toolId](){ return AZStd::make_shared<PassGraphCompiler>(toolId); });

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

    void PassCanvasApplication::InitMainWindow()
    {
        m_viewportSettingsSystem.reset(aznew AtomToolsFramework::EntityPreviewViewportSettingsSystem(m_toolId));

        m_window.reset(aznew PassCanvasMainWindow(m_toolId, m_graphViewSettingsPtr));
        m_window->show();
    }

    void PassCanvasApplication::InitDefaultDocument()
    {
        // Create an untitled, empty graph document as soon as the application starts so the user can begin creating immediately.
        if (AtomToolsFramework::GetSettingsValue("/O3DE/Atom/PassCanvas/CreateDefaultDocumentOnStart", true))
        {
            AZ::Uuid documentId = AZ::Uuid::CreateNull();
            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::EventResult(
                documentId,
                m_toolId,
                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::CreateDocumentFromTypeName,
                "Pass Graph");

            AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
                m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::OnDocumentOpened, documentId);
        }
    }
} // namespace PassCanvas
