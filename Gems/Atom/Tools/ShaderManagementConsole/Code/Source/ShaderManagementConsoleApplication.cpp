/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <ShaderManagementConsoleApplication.h>

#include <Document/ShaderManagementConsoleDocument.h>
#include <Window/ShaderManagementConsoleTableView.h>
#include <Window/ShaderManagementConsoleWindow.h>

void InitShaderManagementConsoleResources()
{
    // Must register qt resources from other modules
    Q_INIT_RESOURCE(ShaderManagementConsole);
    Q_INIT_RESOURCE(InspectorWidget);
    Q_INIT_RESOURCE(AtomToolsAssetBrowser);
}

namespace ShaderManagementConsole
{
    static const char* GetBuildTargetName()
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return LY_CMAKE_TARGET;
    }

    ShaderManagementConsoleApplication::ShaderManagementConsoleApplication(int* argc, char*** argv)
        : Base(GetBuildTargetName(), argc, argv)
    {
        InitShaderManagementConsoleResources();

        QApplication::setOrganizationName("O3DE");
        QApplication::setApplicationName("O3DE Shader Management Console");
        QApplication::setWindowIcon(QIcon(":/Icons/application.svg"));

        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
    }

    ShaderManagementConsoleApplication::~ShaderManagementConsoleApplication()
    {
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();
        m_window.reset();
    }

    void ShaderManagementConsoleApplication::Reflect(AZ::ReflectContext* context)
    {
        Base::Reflect(context);
        ShaderManagementConsoleDocument::Reflect(context);
    }

    const char* ShaderManagementConsoleApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseShaderManagementConsole";
#elif defined(_DEBUG)
        return "DebugShaderManagementConsole";
#else
        return "ProfileShaderManagementConsole";
#endif
    }

    void ShaderManagementConsoleApplication::StartCommon(AZ::Entity* systemEntity)
    {
        Base::StartCommon(systemEntity);

        // Overriding default document type info to provide a custom view
        auto documentTypeInfo = ShaderManagementConsoleDocument::BuildDocumentTypeInfo();
        documentTypeInfo.m_documentViewFactoryCallback = [this](const AZ::Crc32& toolId, const AZ::Uuid& documentId) {
            return m_window->AddDocumentTab(documentId, new ShaderManagementConsoleTableView(toolId, documentId, m_window.get()));
        };
        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType, documentTypeInfo);

        m_window.reset(aznew ShaderManagementConsoleWindow(m_toolId));
        m_window->show();
    }

    void ShaderManagementConsoleApplication::Destroy()
    {
        m_window.reset();
        Base::Destroy();
    }

    AZStd::vector<AZStd::string> ShaderManagementConsoleApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/" });
    }

    QWidget* ShaderManagementConsoleApplication::GetAppMainWindow()
    {
        return m_window.get();
    }
} // namespace ShaderManagementConsole
