/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <Document/MaterialDocument.h>
#include <Document/MaterialDocumentRequestBus.h>
#include <MaterialEditorApplication.h>
#include <MaterialEditor_Traits_Platform.h>
#include <Viewport/MaterialViewportModule.h>
#include <Window/CreateMaterialDialog/CreateMaterialDialog.h>
#include <Window/MaterialEditorWindow.h>
#include <Window/MaterialEditorWindowSettings.h>

#include <QDesktopServices>
#include <QDialog>
#include <QMenu>
#include <QUrl>

void InitMaterialEditorResources()
{
    // Must register qt resources from other modules
    Q_INIT_RESOURCE(MaterialEditor);
    Q_INIT_RESOURCE(InspectorWidget);
    Q_INIT_RESOURCE(AtomToolsAssetBrowser);
}

namespace MaterialEditor
{
    MaterialEditorApplication::MaterialEditorApplication(int* argc, char*** argv)
        : Base(argc, argv)
    {
        InitMaterialEditorResources();

        QApplication::setApplicationName("O3DE Material Editor");

        // The settings registry has been created at this point, so add the CMake target
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
            *AZ::SettingsRegistry::Get(), GetBuildTargetName());

        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
        AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Handler::BusConnect();
    }

    MaterialEditorApplication::~MaterialEditorApplication()
    {
        AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();
        m_window.reset();
    }

    void MaterialEditorApplication::Reflect(AZ::ReflectContext* context)
    {
        Base::Reflect(context);
        MaterialEditorWindowSettings::Reflect(context);

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MaterialDocumentRequestBus>("MaterialDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Event("SetPropertyValue", &MaterialDocumentRequestBus::Events::SetPropertyValue)
                ->Event("GetPropertyValue", &MaterialDocumentRequestBus::Events::GetPropertyValue);
        }
    }

    void MaterialEditorApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        Base::CreateStaticModules(outModules);
        outModules.push_back(aznew MaterialViewportModule);
    }

    const char* MaterialEditorApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseMaterialEditor";
#elif defined(_DEBUG)
        return "DebugMaterialEditor";
#else
        return "ProfileMaterialEditor";
#endif
    }

    void MaterialEditorApplication::StartCommon(AZ::Entity* systemEntity)
    {
        Base::StartCommon(systemEntity);

        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
            &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType,
            []() { return aznew MaterialDocument(); });
    }

    AZStd::string MaterialEditorApplication::GetBuildTargetName() const
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        //! Returns the build system target name of "MaterialEditor"
        return AZStd::string{ LY_CMAKE_TARGET };
    }

    AZStd::vector<AZStd::string> MaterialEditorApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/", "MaterialEditor/" });
    }

    void MaterialEditorApplication::CreateMainWindow()
    {
        m_window.reset(aznew MaterialEditorWindow);
        m_assetBrowserInteractions.reset(aznew AtomToolsFramework::AtomToolsAssetBrowserInteractions);
        m_assetBrowserInteractions->RegisterContextMenuActions(
            [](const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                return entries.front()->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source;
            },
            []([[maybe_unused]] QWidget* caller, QMenu* menu, const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                const bool isMaterial = AzFramework::StringFunc::Path::IsExtension(
                    entries.front()->GetFullPath().c_str(), AZ::RPI::MaterialSourceData::Extension);
                const bool isMaterialType = AzFramework::StringFunc::Path::IsExtension(
                    entries.front()->GetFullPath().c_str(), AZ::RPI::MaterialTypeSourceData::Extension);
                if (isMaterial || isMaterialType)
                {
                    menu->addAction(QObject::tr("Open"), [entries]()
                        {
                            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
                                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::OpenDocument,
                                entries.front()->GetFullPath());
                        });

                    const QString createActionName =
                        isMaterialType ? QObject::tr("Create Material...") : QObject::tr("Create Child Material...");

                    menu->addAction(createActionName, [entries]()
                        {
                            const QString defaultPath = AtomToolsFramework::GetUniqueFileInfo(
                                QString(AZ::Utils::GetProjectPath().c_str()) +
                                AZ_CORRECT_FILESYSTEM_SEPARATOR + "Assets" +
                                AZ_CORRECT_FILESYSTEM_SEPARATOR + "untitled." +
                                AZ::RPI::MaterialSourceData::Extension).absoluteFilePath();

                            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
                                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFile,
                                entries.front()->GetFullPath(),
                                AtomToolsFramework::GetSaveFileInfo(defaultPath).absoluteFilePath().toUtf8().constData());
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

        m_assetBrowserInteractions->RegisterContextMenuActions(
            [](const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                return entries.front()->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Folder;
            },
            [](QWidget* caller, QMenu* menu, const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                menu->addAction(QObject::tr("Create Material..."), [caller, entries]()
                    {
                        CreateMaterialDialog createDialog(entries.front()->GetFullPath().c_str(), caller);
                        createDialog.adjustSize();

                        if (createDialog.exec() == QDialog::Accepted &&
                            !createDialog.m_materialFileInfo.absoluteFilePath().isEmpty() &&
                            !createDialog.m_materialTypeFileInfo.absoluteFilePath().isEmpty())
                        {
                            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
                                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFile,
                                createDialog.m_materialTypeFileInfo.absoluteFilePath().toUtf8().constData(),
                                createDialog.m_materialFileInfo.absoluteFilePath().toUtf8().constData());
                        }
                    });
            });
    }

    void MaterialEditorApplication::DestroyMainWindow()
    {
        m_window.reset();
    }

    QWidget* MaterialEditorApplication::GetAppMainWindow()
    {
        return m_window.get();
    }
} // namespace MaterialEditor
