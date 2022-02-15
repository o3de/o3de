/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <Document/ShaderManagementConsoleDocument.h>
#include <Document/ShaderManagementConsoleDocumentRequestBus.h>
#include <ShaderManagementConsoleApplication.h>
#include <ShaderManagementConsole_Traits_Platform.h>

#include <QDesktopServices>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QUrl>

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

        QApplication::setApplicationName("O3DE Shader Management Console");

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

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ShaderManagementConsoleDocumentRequestBus>("ShaderManagementConsoleDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "shadermanagementconsole")
                ->Event("SetShaderVariantListSourceData", &ShaderManagementConsoleDocumentRequestBus::Events::SetShaderVariantListSourceData)
                ->Event("GetShaderVariantListSourceData", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantListSourceData)
                ->Event("GetShaderOptionCount", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionCount)
                ->Event("GetShaderOptionDescriptor", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptor)
                ->Event("GetShaderVariantCount", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantCount)
                ->Event("GetShaderVariantInfo", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantInfo)
                ;
        }
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

        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType,
            [](const AZ::Crc32& toolId) { return aznew ShaderManagementConsoleDocument(toolId); });

        m_window.reset(aznew ShaderManagementConsoleWindow(m_toolId));

        m_assetBrowserInteractions.reset(aznew AtomToolsFramework::AtomToolsAssetBrowserInteractions);

        m_assetBrowserInteractions->RegisterContextMenuActions(
            [](const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                return entries.front()->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source;
            },
            [this]([[maybe_unused]] QWidget* caller, QMenu* menu, const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                if (AzFramework::StringFunc::Path::IsExtension(
                        entries.front()->GetFullPath().c_str(), AZ::RPI::ShaderSourceData::Extension))
                {
                    menu->addAction("Generate Shader Variant List", [entries]()
                        {
                            const QString script =
                                "@engroot@/Gems/Atom/Tools/ShaderManagementConsole/Scripts/GenerateShaderVariantListForMaterials.py";
                            AZStd::vector<AZStd::string_view> pythonArgs{ entries.front()->GetFullPath() };
                            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
                                &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs, script.toUtf8().constData(),
                                pythonArgs);
                        });
                }

                if (AzFramework::StringFunc::Path::IsExtension(
                        entries.front()->GetFullPath().c_str(), AZ::RPI::ShaderSourceData::Extension) ||
                    AzFramework::StringFunc::Path::IsExtension(
                        entries.front()->GetFullPath().c_str(), AZ::RPI::ShaderVariantListSourceData::Extension))
                {
                    menu->addAction(QObject::tr("Open"), [entries, this]()
                        {
                            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
                                m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::OpenDocument,
                                entries.front()->GetFullPath());
                        });
                }
                else
                {
                    menu->addAction(QObject::tr("Open"), [entries]()
                        {
                            QDesktopServices::openUrl(QUrl::fromLocalFile(entries.front()->GetFullPath().c_str()));
                        });
                }
            });
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
