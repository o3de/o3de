/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Document/MaterialCanvasDocument.h>
#include <GraphModel/Model/DataType.h>
#include <MaterialCanvasApplication.h>
#include <Window/MaterialCanvasGraphView.h>
#include <Window/MaterialCanvasMainWindow.h>

void InitMaterialCanvasResources()
{
    // Must register qt resources from other modules
    Q_INIT_RESOURCE(MaterialCanvas);
    Q_INIT_RESOURCE(InspectorWidget);
    Q_INIT_RESOURCE(AtomToolsAssetBrowser);
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
        QApplication::setApplicationName("O3DE Material Canvas (Experimental)");
        QApplication::setWindowIcon(QIcon(":/Icons/application.svg"));

        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
    }

    MaterialCanvasApplication::~MaterialCanvasApplication()
    {
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();
        m_window.reset();
    }

    void MaterialCanvasApplication::Reflect(AZ::ReflectContext* context)
    {
        Base::Reflect(context);
        MaterialCanvasDocument::Reflect(context);
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

        // Instantiate the dynamic node manager to register all dynamic node configurations and data types used in this tool
        m_dynamicNodeManager.reset(aznew AtomToolsFramework::DynamicNodeManager(m_toolId));

        // Register all data types required by material canvas nodes with the dynamic node manager
        m_dynamicNodeManager->RegisterDataTypes({
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("bool"), bool{}, "bool"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("int"), int32_t{}, "int"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("uint"), uint32_t{}, "uint"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float"), float{}, "float"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float2"), AZ::Vector2::CreateZero(), "float2"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float3"), AZ::Vector3::CreateZero(), "float3"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float4"), AZ::Vector4::CreateZero(), "float4"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("color"), AZ::Color::CreateOne(), "color"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("string"), AZStd::string{}, "string"),
        });

        // Search the project and gems for dynamic node configurations and register them with the manager
        m_dynamicNodeManager->LoadConfigFiles("materialcanvasnode");

        // Each graph document creates its own graph context but we want to use a shared graph context instead to avoid data duplication
        m_graphContext = AZStd::make_shared<GraphModel::GraphContext>(
            "Material Canvas", ".materialcanvas.azasset", m_dynamicNodeManager->GetRegisteredDataTypes());
        m_graphContext->CreateModuleGraphManager();

        // This configuration data is passed through the main window and graph views to setup translation data, styling, and node palettes
        AtomToolsFramework::GraphViewConfig graphViewConfig;
        graphViewConfig.m_translationPath = "@products@/translation/materialcanvas_en_us.qm";
        graphViewConfig.m_styleManagerPath = "MaterialCanvas/StyleSheet/materialcanvas_style.json";
        graphViewConfig.m_nodeMimeType = "MaterialCanvas/node-palette-mime-event";
        graphViewConfig.m_nodeSaveIdentifier = "MaterialCanvas/ContextMenu";
        graphViewConfig.m_createNodeTreeItemsFn = [](const AZ::Crc32& toolId)
        {
            GraphCanvas::GraphCanvasTreeItem* rootTreeItem = {};
            AtomToolsFramework::DynamicNodeManagerRequestBus::EventResult(
                rootTreeItem, toolId, &AtomToolsFramework::DynamicNodeManagerRequestBus::Events::CreateNodePaletteTree);
            return rootTreeItem;
        };

        // Acquiring default material canvas document type info so that it can be customized before registration
        auto documentTypeInfo = MaterialCanvasDocument::BuildDocumentTypeInfo();

        // Overriding default document factory function to pass in a shared graph context
        documentTypeInfo.m_documentFactoryCallback = [this](const AZ::Crc32& toolId, const AtomToolsFramework::DocumentTypeInfo& documentTypeInfo)
        {
            return aznew MaterialCanvasDocument(toolId, documentTypeInfo, m_graphContext);
        };

        // Overriding documentview factory function to create graph view
        documentTypeInfo.m_documentViewFactoryCallback = [this, graphViewConfig](const AZ::Crc32& toolId, const AZ::Uuid& documentId)
        {
            return m_window->AddDocumentTab(documentId, aznew MaterialCanvasGraphView(toolId, documentId, graphViewConfig));
        };

        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType, documentTypeInfo);

        m_viewportSettingsSystem.reset(aznew AtomToolsFramework::EntityPreviewViewportSettingsSystem(m_toolId));

        m_window.reset(aznew MaterialCanvasMainWindow(m_toolId, graphViewConfig));
        m_window->show();
    }

    void MaterialCanvasApplication::Destroy()
    {
        m_window.reset();
        m_viewportSettingsSystem.reset();
        m_graphContext.reset();
        m_dynamicNodeManager.reset();
        Base::Destroy();
    }

    AZStd::vector<AZStd::string> MaterialCanvasApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/"});
    }

    QWidget* MaterialCanvasApplication::GetAppMainWindow()
    {
        return m_window.get();
    }
} // namespace MaterialCanvas
