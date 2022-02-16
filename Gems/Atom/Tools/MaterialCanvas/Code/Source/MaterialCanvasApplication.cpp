/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <Document/MaterialCanvasDocument.h>
#include <Document/MaterialCanvasDocumentRequestBus.h>
#include <MaterialCanvasApplication.h>
#include <MaterialCanvas_Traits_Platform.h>
#include <Viewport/MaterialCanvasViewportModule.h>
#include <Window/MaterialCanvasMainWindow.h>
#include <Window/MaterialCanvasMainWindowSettings.h>

#include <QDesktopServices>
#include <QDialog>
#include <QMenu>
#include <QUrl>

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

        QApplication::setApplicationName("O3DE Material Canvas");

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
        MaterialCanvasMainWindowSettings::Reflect(context);
    }

    void MaterialCanvasApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        Base::CreateStaticModules(outModules);
        outModules.push_back(aznew MaterialCanvasViewportModule);
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

        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType,
            [](const AZ::Crc32& toolId) { return aznew MaterialCanvasDocument(toolId); });

        m_window.reset(aznew MaterialCanvasMainWindow(m_toolId));

        m_assetBrowserInteractions.reset(aznew AtomToolsFramework::AtomToolsAssetBrowserInteractions);

        m_assetBrowserInteractions->RegisterContextMenuActions(
            [](const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                return entries.front()->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source;
            },
            []([[maybe_unused]] QWidget* caller, QMenu* menu, const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                menu->addAction(QObject::tr("Open"), [entries]()
                    {
                        QDesktopServices::openUrl(QUrl::fromLocalFile(entries.front()->GetFullPath().c_str()));
                    });
            });
    }

    void MaterialCanvasApplication::Destroy()
    {
        m_window.reset();
        Base::Destroy();
    }

    AZStd::vector<AZStd::string> MaterialCanvasApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/" });
    }

    QWidget* MaterialCanvasApplication::GetAppMainWindow()
    {
        return m_window.get();
    }
} // namespace MaterialCanvas
