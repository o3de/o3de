/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentApplication.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Document/CreateDocumentDialog.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

#include <QDesktopServices>
#include <QDialog>
#include <QMenu>
#include <QUrl>

namespace AtomToolsFramework
{
    AtomToolsDocumentApplication::AtomToolsDocumentApplication(const char* targetName, int* argc, char*** argv)
        : Base(targetName, argc, argv)
    {
    }

    void AtomToolsDocumentApplication::StartCommon(AZ::Entity* systemEntity)
    {
        Base::StartCommon(systemEntity);

        m_documentSystem.reset(aznew AtomToolsDocumentSystem(m_toolId));

        m_assetBrowserInteractions->RegisterContextMenuActions(
            [](const AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                return entries.front()->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source;
            },
            [this]([[maybe_unused]] QWidget* caller, QMenu* menu, const AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                bool handledOpen = false;
                DocumentTypeInfoVector documentTypes;
                AtomToolsDocumentSystemRequestBus::EventResult(
                    documentTypes, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::GetRegisteredDocumentTypes);
                for (const auto& documentType : documentTypes)
                {
                    if (documentType.IsSupportedExtensionToOpen(entries.front()->GetFullPath()))
                    {
                        menu->addAction(QObject::tr("Open"), [entries, this]() {
                            AZ::SystemTickBus::QueueFunction([toolId = m_toolId, path = entries.front()->GetFullPath()]() {
                                AtomToolsDocumentSystemRequestBus::Event(toolId, &AtomToolsDocumentSystemRequestBus::Events::OpenDocument, path);
                            });
                        });
                        handledOpen = true;
                        break;
                    }
                }

                if (!handledOpen)
                {
                    menu->addAction(QObject::tr("Open"), [entries]() {
                        QDesktopServices::openUrl(QUrl::fromLocalFile(entries.front()->GetFullPath().c_str()));
                    });
                }

                for (const auto& documentType : documentTypes)
                {
                    if (documentType.IsSupportedExtensionToCreate(entries.front()->GetFullPath()))
                    {
                        const QString createActionName = documentType.IsSupportedExtensionToSave(entries.front()->GetFullPath())
                            ? QObject::tr("Create Child %1...").arg(documentType.m_documentTypeName.c_str())
                            : QObject::tr("Create %1...").arg(documentType.m_documentTypeName.c_str());

                        menu->addAction(createActionName, [entries, documentType, this]() {
                            const auto& savePath = GetSaveFilePathFromDialog(
                                {}, documentType.m_supportedExtensionsToSave, documentType.m_documentTypeName);
                            if (!savePath.empty())
                            {
                                AtomToolsDocumentSystemRequestBus::Event(
                                    m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFilePath,
                                    entries.front()->GetFullPath(), savePath);
                            }
                        });
                    }
                }
            });

        m_assetBrowserInteractions->RegisterContextMenuActions(
            [](const AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                return entries.front()->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Folder;
            },
            [this](QWidget* caller, QMenu* menu, const AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                DocumentTypeInfoVector documentTypes;
                AtomToolsDocumentSystemRequestBus::EventResult(
                    documentTypes, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::GetRegisteredDocumentTypes);
                for (const auto& documentType : documentTypes)
                {
                    menu->addAction(QObject::tr("Create %1...").arg(documentType.m_documentTypeName.c_str()), [caller, documentType, entries, this]() {
                        CreateDocumentDialog dialog(documentType, entries.front()->GetFullPath().c_str(), caller);
                        dialog.adjustSize();

                        if (dialog.exec() == QDialog::Accepted && !dialog.m_sourcePath.isEmpty() && !dialog.m_targetPath.isEmpty())
                        {
                            AtomToolsDocumentSystemRequestBus::Event(
                                m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFilePath,
                                dialog.m_sourcePath.toUtf8().constData(), dialog.m_targetPath.toUtf8().constData());
                        }
                    });
                }
            });
    }

    void AtomToolsDocumentApplication::Destroy()
    {
        m_documentSystem.reset();
        Base::Destroy();
    }

    void AtomToolsDocumentApplication::ProcessCommandLine(const AZ::CommandLine& commandLine)
    {
        // Process command line options for opening documents on startup
        size_t openDocumentCount = commandLine.GetNumMiscValues();
        for (size_t openDocumentIndex = 0; openDocumentIndex < openDocumentCount; ++openDocumentIndex)
        {
            const AZStd::string openDocumentPath = commandLine.GetMiscValue(openDocumentIndex);

            AZ_Printf(m_targetName.c_str(), "Opening document: %s", openDocumentPath.c_str());
            AtomToolsDocumentSystemRequestBus::Event(m_toolId, &AtomToolsDocumentSystemRequestBus::Events::OpenDocument, openDocumentPath);
        }

        Base::ProcessCommandLine(commandLine);
    }
} // namespace AtomToolsFramework
