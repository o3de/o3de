/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetEditorWidget.h"
#include "AssetEditorTab.h"

#include <API/EditorAssetSystemAPI.h>
#include <API/ToolsApplicationAPI.h>

#include <AssetEditor/AssetEditorBus.h>
#include <AssetEditor/AssetEditorHeader.h>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have
                                                          // dll-interface to be used by clients of class 'QLayoutItem'
#include <AzToolsFramework/AssetEditor/ui_AssetEditorToolbar.h>
AZ_POP_DISABLE_WARNING
#include <AzToolsFramework/AssetEditor/ui_AssetEditorStatusBar.h>

#include <AssetBrowser/AssetSelectionModel.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/sort.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzQtComponents/Components/Widgets/FileDialog.h>

#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <SourceControl/SourceControlAPI.h>

#include <UI/PropertyEditor/PropertyRowWidget.hxx>
#include <UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include <AzFramework/DocumentPropertyEditor/ReflectionAdapter.h>
#include <UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <UI/DocumentPropertyEditor/FilteredDPE.h>

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QVBoxLayout>

namespace AzToolsFramework
{
    namespace AssetEditor
    {
        //////////////////////////////////
        // AssetEditorWidgetUserSettings
        //////////////////////////////////

        void AssetEditorWidgetUserSettings::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetEditorWidgetUserSettings>()
                    ->Version(0)
                    ->Field("m_showPreviewMessage", &AssetEditorWidgetUserSettings::m_lastSavePath)
                    ->Field("m_snapDistance", &AssetEditorWidgetUserSettings::m_recentPaths);
            }
        }

        AssetEditorWidgetUserSettings::AssetEditorWidgetUserSettings()
        {
            char assetRoot[AZ_MAX_PATH_LEN] = { 0 };
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@projectroot@", assetRoot, AZ_MAX_PATH_LEN);

            m_lastSavePath = assetRoot;
        }

        void AssetEditorWidgetUserSettings::AddRecentPath(const AZStd::string& recentPath)
        {
            AZStd::string lowerCasePath = recentPath;
            AZStd::to_lower(lowerCasePath.begin(), lowerCasePath.end());

            auto item = AZStd::find(m_recentPaths.begin(), m_recentPaths.end(), lowerCasePath);
            if (item != m_recentPaths.end())
            {
                m_recentPaths.erase(item);
            }

            m_recentPaths.emplace(m_recentPaths.begin(), lowerCasePath);

            while (m_recentPaths.size() > 10)
            {
                m_recentPaths.pop_back();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetEditorWidget
        //////////////////////////////////////////////////////////////////////////

        const AZ::Crc32 k_assetEditorWidgetSettings = AZ_CRC_CE("AssetEditorSettings");

        AssetEditorWidget::AssetEditorWidget(QWidget* parent)
            : QWidget(parent)
            , m_statusBar(new Ui::AssetEditorStatusBar())
        {
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve serialize context.");

            setObjectName("AssetEditorWidget");

            QVBoxLayout* mainLayout = new QVBoxLayout();
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(0);

            m_tabs = new AzQtComponents::TabWidget(this);
            m_tabs->setObjectName("AssetEditorTabWidget");
            AzQtComponents::TabWidget::applySecondaryStyle(m_tabs, false);
            m_tabs->setContentsMargins(0, 0, 0, 0);
            m_tabs->setTabsClosable(true);
            m_tabs->setMovable(true);
            mainLayout->addWidget(m_tabs);

            connect(m_tabs, &QTabWidget::tabCloseRequested, this, &AssetEditorWidget::onTabCloseButtonPressed);
            connect(m_tabs, &AzQtComponents::TabWidget::currentChanged, this, &AssetEditorWidget::currentTabChanged);
            
            QWidget* statusBarWidget = new QWidget(this);
            statusBarWidget->setObjectName("AssetEditorStatusBar");
            m_statusBar->setupUi(statusBarWidget);
            m_statusBar->textEdit->setText("");
            m_statusBar->textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);

            mainLayout->addWidget(statusBarWidget);

            PopulateGenericAssetTypes();

            QMenuBar* mainMenu = new QMenuBar();
            QMenu* fileMenu = mainMenu->addMenu(tr("&File"));
            // Add Create New Asset menu and populate it with all asset types that have GenericAssetHandler
            m_newAssetMenu = fileMenu->addMenu(tr("&New"));

            for (const auto& assetType : m_genericAssetTypes)
            {
                QString assetTypeName;
                AZ::AssetTypeInfoBus::EventResult(assetTypeName, assetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);

                if (!assetTypeName.isEmpty())
                {
                    QAction* newAssetAction = m_newAssetMenu->addAction(assetTypeName);
                    connect(
                        newAssetAction,
                        &QAction::triggered,
                        this,
                        [assetType, this]()
                        {
                            CreateAsset(assetType, AZ::Uuid::CreateNull());
                        }
                    );
                }
            }

            QAction* openAssetAction = fileMenu->addAction("&Open...");
            connect(openAssetAction, &QAction::triggered, this, &AssetEditorWidget::OpenAssetWithDialog);

            m_recentFileMenu = fileMenu->addMenu("Open Recent");

            fileMenu->addSeparator();

            m_saveAssetAction = fileMenu->addAction("&Save");
            m_saveAssetAction->setShortcut(QKeySequence::Save);
            connect(m_saveAssetAction, &QAction::triggered, this, &AssetEditorWidget::SaveAsset);

            m_saveAsAssetAction = fileMenu->addAction("&Save As");
            m_saveAsAssetAction->setShortcut(QKeySequence::SaveAs);
            connect(m_saveAsAssetAction, &QAction::triggered, this, &AssetEditorWidget::SaveAssetAs);

            m_saveAllAssetsAction = fileMenu->addAction("Save All");
            connect(m_saveAllAssetsAction, &QAction::triggered, this, &AssetEditorWidget::SaveAll);

            // The Save actions are disabled by default.
            // They are activated when an asset is created/opened.
            m_saveAssetAction->setEnabled(false);
            m_saveAsAssetAction->setEnabled(false);
            m_saveAllAssetsAction->setEnabled(false);

            QMenu* viewMenu = mainMenu->addMenu(tr("&View"));

            QAction* expandAll = viewMenu->addAction("&Expand All");
            connect(expandAll, &QAction::triggered, this, &AssetEditorWidget::ExpandAll);

            QAction* collapseAll = viewMenu->addAction("&Collapse All");
            connect(collapseAll, &QAction::triggered, this, &AssetEditorWidget::CollapseAll);

            mainLayout->setMenuBar(mainMenu);

            setLayout(mainLayout);

            m_userSettings =
                AZ::UserSettings::CreateFind<AssetEditorWidgetUserSettings>(k_assetEditorWidgetSettings, AZ::UserSettings::CT_LOCAL);

            UpdateRecentFileListState();

            QObject::connect(m_recentFileMenu, &QMenu::aboutToShow, this, &AssetEditorWidget::PopulateRecentMenu);
        }

        AssetEditorWidget::~AssetEditorWidget()
        {
        }

        void AssetEditorWidget::SaveAll()
        {

            for (int tabIndex = 0; tabIndex < m_tabs->count(); tabIndex++)
            {
                AssetEditorTab* tab = qobject_cast<AssetEditorTab*>(m_tabs->widget(tabIndex));
                if (!tab->IsDirty())
                {
                    continue;
                }

                tab->SaveAsset();
            }
        }

        bool AssetEditorWidget::SaveAllAndClose()
        {
            AssetEditorTab::SaveCompleteCallback saveCompleteCallback = [this]() -> void
            {
                // Check that all tabs are clean.
                for (int tabIndex = 0; tabIndex < m_tabs->count(); tabIndex++)
                {
                    AssetEditorTab* tab = qobject_cast<AssetEditorTab*>(m_tabs->widget(tabIndex));
                    if (tab->IsDirty() && !tab->UserRefusedSave())
                    {
                        return;
                    }
                }

                CloseOnNextTick();
            };

            // Ensure the UserRefusedSave flag is reset so that if this gets called twice, dirty tabs aren't ignored.
            for (int tabIndex = m_tabs->count() - 1; tabIndex >= 0; tabIndex--)
            {
                AssetEditorTab* tab = qobject_cast<AssetEditorTab*>(m_tabs->widget(tabIndex));
                tab->ClearUserRefusedSaveFlag();
            }

            if (!m_tabs->count())
            {
                parentWidget()->parentWidget()->close();
                return true;
            }

            for (int tabIndex = m_tabs->count() - 1; tabIndex >= 0; tabIndex--)
            {
                AssetEditorTab* tab = qobject_cast<AssetEditorTab*>(m_tabs->widget(tabIndex));
                if (!tab)
                {
                    continue;
                }

                if (!tab->IsDirty())
                {
                    continue;
                }

                if (!tab->TrySaveAsset(saveCompleteCallback))
                {
                    // Action has been canceled.
                    return false;
                }
            }

            return true;
        }

        void AssetEditorWidget::CreateAsset(AZ::Data::AssetType assetType, const AZ::Uuid& observerToken)
        {
            QString newAssetName = QString(tr("New Asset %1").arg(m_nextNewAssetIndex++));
            AssetEditorTab* newTab = MakeNewTab(newAssetName);
            newTab->CreateAsset(assetType, newAssetName, observerToken);
        }

        void AssetEditorWidget::OpenAsset(const AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            AZ::Data::AssetInfo typeInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                typeInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, asset.GetId());

            // Check if there is an asset to open
            if (typeInfo.m_relativePath.empty())
            {
                return;
            }

            bool hasResult = false;
            AZStd::string fullPath;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                hasResult,
                &AzToolsFramework::AssetSystem::AssetSystemRequest::GetFullSourcePathFromRelativeProductPath,
                typeInfo.m_relativePath,
                fullPath);

            AZStd::string fileName = typeInfo.m_relativePath;

            if (AzFramework::StringFunc::Path::Normalize(fileName))
            {
                AzFramework::StringFunc::Path::StripPath(fileName);
            }

            AddRecentPath(fullPath.c_str());

            AssetEditorTab* tab = FindTabForAsset(asset.GetId());
            if (tab)
            {
                // This asset is already open, just switch to the correct tab.
                m_tabs->setCurrentWidget(tab);
            }
            else
            {
                AssetEditorTab* newTab = MakeNewTab(fileName.c_str());
                newTab->LoadAsset(asset.GetId(), asset.GetType(), fileName.c_str());
            }
        }

        void AssetEditorWidget::OpenAssetWithDialog()
        {
            AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection(m_genericAssetTypes);
            EditorRequests::Bus::Broadcast(&EditorRequests::BrowseForAssets, selection);
            if (selection.IsValid())
            {
                auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());

                AssetEditorTab* tab = FindTabForAsset(product->GetAssetId());
                if (tab)
                {
                    // This asset is already open, just switch to the correct tab.
                    m_tabs->setCurrentWidget(tab);
                }
                else
                {
                    AssetEditorTab* newTab = MakeNewTab(product->GetName().c_str());
                    newTab->LoadAsset(product->GetAssetId(), product->GetAssetType(), product->GetName().c_str());
                }

                AddRecentPath(product->GetFullPath());
            }
        }

        void AssetEditorWidget::OpenAssetFromPath(const AZStd::string& assetPath)
        {
            bool hasResult = false;
            AZ::Data::AssetInfo assetInfo;
            AZStd::string watchFolder;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                hasResult,
                &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath,
                assetPath.c_str(),
                assetInfo,
                watchFolder);

            if (hasResult)
            {
                AZStd::string fileName = assetPath;

                if (AzFramework::StringFunc::Path::Normalize(fileName))
                {
                    AzFramework::StringFunc::Path::StripPath(fileName);
                }

                AZ::Data::AssetInfo typeInfo;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    typeInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetInfo.m_assetId);

                AssetEditorTab* tab = FindTabForAsset(assetInfo.m_assetId);
                if (tab)
                {
                    // This asset is already open, just switch to the correct tab.
                    m_tabs->setCurrentWidget(tab);
                }
                else
                {
                    AssetEditorTab* newTab = MakeNewTab(fileName.c_str());
                    newTab->LoadAsset(assetInfo.m_assetId, typeInfo.m_assetType, fileName.c_str());
                }
                AddRecentPath(assetPath.c_str());
            }
        }

        bool AssetEditorWidget::SaveAssetToPath(AZStd::string_view assetPath)
        {
            AssetEditorTab* tab = qobject_cast<AssetEditorTab*>(m_tabs->currentWidget());
            tab->SaveAsset();

            return tab->SaveAssetToPath(assetPath.data());
        }

        void AssetEditorWidget::SaveAsset()
        {
            if (!m_tabs->count())
            {
                return;
            }

            AssetEditorTab* tab = qobject_cast<AssetEditorTab*>(m_tabs->currentWidget());
            tab->SaveAsset();

        }
        
        void AssetEditorWidget::SaveAssetAs()
        {
            if (!m_tabs->count())
            {
                return;
            }

            AssetEditorTab* tab = qobject_cast<AssetEditorTab*>(m_tabs->currentWidget());
            tab->SaveAsDialog();
        }

        void AssetEditorWidget::currentTabChanged(int /*newCurrentIndex*/)
        {
            UpdateSaveMenuActionsStatus();
        }

        void AssetEditorWidget::onTabCloseButtonPressed(int tabIndexToClose)
        {
            AssetEditorTab* tab = qobject_cast<AssetEditorTab*>(m_tabs->widget(tabIndexToClose));
            if (!tab)
            {
                return;
            }

            tab->ClearUserRefusedSaveFlag();

            AssetEditorTab::SaveCompleteCallback saveCompleteCallback = [this, tabIndexToClose, tab]() -> void
            {
                if (!tab->IsDirty() || tab->UserRefusedSave())
                {
                    m_tabs->removeTab(tabIndexToClose);
                }
            };


            if (tab->IsDirty())
            {
                tab->TrySaveAsset(saveCompleteCallback);
                return;
            }

            m_tabs->removeTab(tabIndexToClose);
            delete tab;
        }

        void AssetEditorWidget::CloseTab(AssetEditorTab* tab)
        {
            int index = m_tabs->indexOf(tab);
            if (index >= 0)
            {
                m_tabs->removeTab(index);
                delete tab;
            }
        }

        void AssetEditorWidget::CloseOnNextTick()
        {
            // Close the window on the next tick so that the parent widgets finish processing any current event correctly.
            QTimer::singleShot(0, this, [this]()
            {
                    // Close tabs
                for (int tabIndex = m_tabs->count() - 1; tabIndex >= 0; tabIndex--)
                {
                    AssetEditorTab* tab = qobject_cast<AssetEditorTab*>(m_tabs->widget(tabIndex));
                    m_tabs->removeTab(tabIndex);
                    tab->deleteLater();
                }

                parentWidget()->parentWidget()->close();
            });
        }

        void AssetEditorWidget::CloseTabAndContainerIfEmpty(AssetEditorTab* tab)
        {
            CloseTab(tab);
            if (!m_tabs->count())
            {
                CloseOnNextTick();
            }
        }

        bool AssetEditorWidget::IsValidAssetType(const AZ::Data::AssetType& assetType) const
        {
            auto typeIter = AZStd::find_if(
                m_genericAssetTypes.begin(),
                m_genericAssetTypes.end(),
                [assetType](const AZ::Data::AssetType& testType)
                {
                    return assetType == testType;
                });

            if (typeIter != m_genericAssetTypes.end())
            {
                return true;
            }
            return false;
        }

        AssetEditorTab* AssetEditorWidget::MakeNewTab(const QString& name)
        {
            AssetEditorTab* newTab = new AssetEditorTab(m_tabs);
            m_tabs->addTab(newTab, name);

            connect(newTab, &AssetEditorTab::OnAssetSaveFailedSignal, this, &AssetEditorWidget::OnAssetSaveFailed);
            connect(newTab, &AssetEditorTab::OnAssetOpenedSignal, this, &AssetEditorWidget::OnAssetOpened);

            return newTab;
        }

        AssetEditorTab* AssetEditorWidget::FindTabForAsset(const AZ::Data::AssetId& assetId) const
        {
            for (int tabIndex = 0; tabIndex < m_tabs->count(); tabIndex++)
            {
                AssetEditorTab* tab = qobject_cast<AssetEditorTab*>(m_tabs->widget(tabIndex));
                if (assetId == tab->GetAssetId())
                    return tab;
            }

            return nullptr;
        }

        void AssetEditorWidget::PopulateGenericAssetTypes()
        {
            auto enumerateCallback = [this](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid& /*typeId*/) -> bool
            {
                AZ::Data::AssetType assetType = classData->m_typeId;

                // make sure this is not base class
                if (assetType == AZ::AzTypeInfo<AZ::Data::AssetData>::Uuid() ||
                    AZ::FindAttribute(AZ::Edit::Attributes::EnableForAssetEditor, classData->m_attributes) == nullptr)
                {
                    return true;
                }

                // narrow down all reflected AssetTypeInfos to those that have a GenericAssetHandler
                AZ::Data::AssetManager& manager = AZ::Data::AssetManager::Instance();
                if (const AZ::Data::AssetHandler* assetHandler = manager.GetHandler(classData->m_typeId))
                {
                    if (!azrtti_istypeof<AzFramework::GenericAssetHandlerBase>(assetHandler))
                    {
                        // its not the generic asset handler.
                        return true;
                    }
                }

                m_genericAssetTypes.push_back(assetType);
                return true;
            };

            m_serializeContext->EnumerateDerived(enumerateCallback, AZ::Uuid::CreateNull(), AZ::AzTypeInfo<AZ::Data::AssetData>::Uuid());

            AZStd::sort(
                m_genericAssetTypes.begin(),
                m_genericAssetTypes.end(),
                [&](const AZ::Data::AssetType& lhsAssetType, const AZ::Data::AssetType& rhsAssetType)
                {
                    AZStd::string lhsAssetTypeName = "";
                    AZ::AssetTypeInfoBus::EventResult(lhsAssetTypeName, lhsAssetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);
                    AZStd::string rhsAssetTypeName = "";
                    AZ::AssetTypeInfoBus::EventResult(rhsAssetTypeName, rhsAssetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);
                    return lhsAssetTypeName < rhsAssetTypeName;
                });
        }

        void AssetEditorWidget::ExpandAll()
        {
            for (int tabIndex = 0; tabIndex < m_tabs->count(); tabIndex++)
            {
                AssetEditorTab* tab = qobject_cast<AssetEditorTab*>(m_tabs->widget(tabIndex));
                tab->ExpandAll();
            }
        }

        void AssetEditorWidget::CollapseAll()
        {
            for (int tabIndex = 0; tabIndex < m_tabs->count(); tabIndex++)
            {
                AssetEditorTab* tab = qobject_cast<AssetEditorTab*>(m_tabs->widget(tabIndex));
                tab->CollapseAll();
            }
        }

       void AssetEditorWidget::SetCurrentTab(AssetEditorTab* tab)
        {
            m_tabs->setCurrentWidget(tab);
        }

        void AssetEditorWidget::UpdateTabTitle(AssetEditorTab* tab)
        {
            // Ensure the name is up to date and add a "*" if the asset is dirty.
            QString assetName = tab->GetAssetName();
            if (tab->IsDirty())
            {
                assetName.append("*");
            }

            int index = m_tabs->indexOf(tab);
            m_tabs->setTabText(index, assetName);
        }

        void AssetEditorWidget::SetLastSavePath(const AZStd::string& savePath)
        {
            m_userSettings->m_lastSavePath = savePath;
        }

        const QString AssetEditorWidget::GetLastSavePath() const
        {
            return m_userSettings->m_lastSavePath.c_str();
        }

        void AssetEditorWidget::SetStatusText(const QString& assetStatus)
        {
            m_statusBar->textEdit->setText(assetStatus);
        }

        void AssetEditorWidget::AddRecentPath(const AZStd::string& recentPath)
        {
            m_userSettings->AddRecentPath(recentPath);
            UpdateRecentFileListState();
        }

        void AssetEditorWidget::PopulateRecentMenu()
        {
            m_recentFileMenu->clear();

            if (m_userSettings->m_recentPaths.empty())
            {
                m_recentFileMenu->setEnabled(false);
            }
            else
            {
                m_recentFileMenu->setEnabled(true);

                for (const AZStd::string& recentFile : m_userSettings->m_recentPaths)
                {
                    bool hasResult = false;
                    AZStd::string relativePath;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                        hasResult,
                        &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath,
                        recentFile,
                        relativePath);

                    if (hasResult)
                    {
                        QAction* action = m_recentFileMenu->addAction(relativePath.c_str());
                        connect(
                            action,
                            &QAction::triggered,
                            this,
                            [recentFile, this]()
                            {
                                this->OpenAssetFromPath(recentFile);
                            });
                    }
                }
            }
        }

        void AssetEditorWidget::UpdateSaveMenuActionsStatus()
        {
            if (!m_tabs->count())
            {
                m_saveAllAssetsAction->setEnabled(false);
                m_saveAssetAction->setEnabled(false);
                m_saveAsAssetAction->setEnabled(false);
                return;
            }

            // Enable the save all option if any tabs are dirty.
            bool haveDirtyTabs = false;

            for (int tabIndex = 0; tabIndex < m_tabs->count(); tabIndex++)
            {
                AssetEditorTab* tab = qobject_cast<AssetEditorTab*>(m_tabs->widget(tabIndex));
                if (tab->IsDirty())
                {
                    haveDirtyTabs = true;
                    break;
                }
            }

            m_saveAllAssetsAction->setEnabled(haveDirtyTabs);

            // Current tab
            AssetEditorTab* tab = qobject_cast<AssetEditorTab*>(m_tabs->currentWidget());
            if (tab)
            {
                // Enable the Save option depending on whether the current tab is dirty.
                m_saveAssetAction->setEnabled(tab->IsDirty());

                // Always enable Save As... if a tab is active.
                m_saveAsAssetAction->setEnabled(true);
            }
        }

        void AssetEditorWidget::OnAssetSaveFailed(const AZStd::string& error)
        {
            Q_EMIT OnAssetSaveFailedSignal(error);
        }

        void AssetEditorWidget::OnAssetOpened(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            Q_EMIT OnAssetOpenedSignal(asset);
        }

        void AssetEditorWidget::UpdateRecentFileListState()
        {
            if (m_recentFileMenu)
            {
                if (!m_userSettings || m_userSettings->m_recentPaths.empty())
                {
                    m_recentFileMenu->setEnabled(false);
                }
                else
                {
                    m_recentFileMenu->setEnabled(true);
                }
            }
        }
        
        void AssetEditorWidget::ShowAddAssetMenu(const QToolButton* menuButton)
        {
            const auto position = menuButton->mapToGlobal(menuButton->geometry().bottomLeft());
            m_newAssetMenu->exec(position);
        }

    } // namespace AssetEditor
} // namespace AzToolsFramework

#include "AssetEditor/moc_AssetEditorWidget.cpp"
