/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <Document/MaterialCanvasDocument.h>
#include <Document/MaterialCanvasGraphDataTypes.h>
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
        QApplication::setApplicationName("O3DE Material Canvas PROTOTYPE WIP");
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

        m_graphContext = AZStd::make_shared<GraphModel::GraphContext>("Material Canvas", ".materialcanvas", CreateAllDataTypes());
        m_graphContext->CreateModuleGraphManager();

        // Instantiate the dynamic node manager, giving it an extension to enumerate and load node configurations
        m_dynamicNodeManager.reset(aznew AtomToolsFramework::DynamicNodeManager());
        m_dynamicNodeManager->LoadMatchingConfigFiles({ "materialcanvasnode.azasset" });

        // This callback is passed into the main window and views to populate nude palette items from the dynamic node manager
        auto createNodePaletteFn = [this](const AZ::Crc32& toolId)
        {
            return m_dynamicNodeManager->CreateNodePaletteRootTreeItem(toolId);
        };

        // Overriding default document type info to provide a custom view
        auto documentTypeInfo = MaterialCanvasDocument::BuildDocumentTypeInfo();

        // Overriding default document factory function to pass in a shared graph context
        documentTypeInfo.m_documentFactoryCallback = [this](const AZ::Crc32& toolId, const AtomToolsFramework::DocumentTypeInfo& documentTypeInfo)
        {
            return aznew MaterialCanvasDocument(toolId, documentTypeInfo, m_graphContext);
        };

        // Overriding documentview factory function to create graph view
        documentTypeInfo.m_documentViewFactoryCallback = [this, createNodePaletteFn](const AZ::Crc32& toolId, const AZ::Uuid& documentId)
        {
            return m_window->AddDocumentTab(documentId, aznew MaterialCanvasGraphView(toolId, documentId, createNodePaletteFn));
        };

        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType, documentTypeInfo);

        m_viewportSettingsSystem.reset(aznew AtomToolsFramework::EntityPreviewViewportSettingsSystem(m_toolId));

        m_window.reset(aznew MaterialCanvasMainWindow(m_toolId, createNodePaletteFn));
        m_window->show();
    }

    void MaterialCanvasApplication::Destroy()
    {
        m_window.reset();
        m_viewportSettingsSystem.reset();
        Base::Destroy();
    }

    AZStd::vector<AZStd::string> MaterialCanvasApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/", "MaterialCanvas/" });
    }

    QWidget* MaterialCanvasApplication::GetAppMainWindow()
    {
        return m_window.get();
    }
} // namespace MaterialCanvas
