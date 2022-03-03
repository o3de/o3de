/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AtomToolsFramework/CreateDocumentDialog/CreateDocumentDialog.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <Document/MaterialDocumentRequestBus.h>
#include <Viewport/MaterialViewportWidget.h>
#include <Window/MaterialEditorWindow.h>
#include <Window/SettingsDialog/SettingsDialog.h>
#include <Window/ViewportSettingsInspector/ViewportSettingsInspector.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QUrl>
#include <QWindow>
AZ_POP_DISABLE_WARNING

namespace MaterialEditor
{
    MaterialEditorWindow::MaterialEditorWindow(const AZ::Crc32& toolId, QWidget* parent)
        : Base(toolId, parent)
    {
        QApplication::setWindowIcon(QIcon(":/Icons/application.svg"));

        m_toolBar = new MaterialEditorToolBar(m_toolId, this);
        m_toolBar->setObjectName("ToolBar");
        addToolBar(m_toolBar);

        m_materialViewport = new MaterialViewportWidget(m_toolId, centralWidget());
        m_materialViewport->setObjectName("Viewport");
        m_materialViewport->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        centralWidget()->layout()->addWidget(m_materialViewport);

        m_assetBrowser->SetFilterState("", AZ::RPI::StreamingImageAsset::Group, true);
        m_assetBrowser->SetFilterState("", AZ::RPI::MaterialAsset::Group, true);
        m_assetBrowser->SetOpenHandler([this](const AZStd::string& absolutePath) {
            if (AzFramework::StringFunc::Path::IsExtension(absolutePath.c_str(), AZ::RPI::MaterialSourceData::Extension))
            {
                AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
                    m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::OpenDocument, absolutePath);
                return;
            }

            if (AzFramework::StringFunc::Path::IsExtension(absolutePath.c_str(), AZ::RPI::MaterialTypeSourceData::Extension))
            {
                return;
            }

            QDesktopServices::openUrl(QUrl::fromLocalFile(absolutePath.c_str()));
        });

        m_materialInspector = new AtomToolsFramework::AtomToolsDocumentInspector(m_toolId, this);
        m_materialInspector->SetDocumentSettingsPrefix("/O3DE/Atom/MaterialEditor/MaterialInspector");
        m_materialInspector->SetIndicatorFunction(
            [](const AzToolsFramework::InstanceDataNode* node)
            {
                const auto property = AtomToolsFramework::FindAncestorInstanceDataNodeByType<AtomToolsFramework::DynamicProperty>(node);
                if (property && !AtomToolsFramework::ArePropertyValuesEqual(property->GetValue(), property->GetConfig().m_parentValue))
                {
                    return ":/Icons/changed_property.svg";
                }
                return ":/Icons/blank.png";
            });

        AddDockWidget("Inspector", m_materialInspector, Qt::RightDockWidgetArea, Qt::Vertical);
        AddDockWidget("Viewport Settings", new ViewportSettingsInspector(m_toolId), Qt::LeftDockWidgetArea, Qt::Vertical);
        SetDockWidgetVisible("Viewport Settings", false);

        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    void MaterialEditorWindow::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        Base::OnDocumentOpened(documentId);
        m_materialInspector->SetDocumentId(documentId);
    }

    void MaterialEditorWindow::ResizeViewportRenderTarget(uint32_t width, uint32_t height)
    {
        QSize requestedViewportSize = QSize(width, height) / devicePixelRatioF();
        QSize currentViewportSize = m_materialViewport->size();
        QSize offset = requestedViewportSize - currentViewportSize;
        QSize requestedWindowSize = size() + offset;
        resize(requestedWindowSize);

        AZ_Assert(
            m_materialViewport->size() == requestedViewportSize,
            "Resizing the window did not give the expected viewport size. Requested %d x %d but got %d x %d.",
            requestedViewportSize.width(), requestedViewportSize.height(), m_materialViewport->size().width(),
            m_materialViewport->size().height());

        [[maybe_unused]] QSize newDeviceSize = m_materialViewport->size();
        AZ_Warning(
            "Material Editor", static_cast<uint32_t>(newDeviceSize.width()) == width && static_cast<uint32_t>(newDeviceSize.height()) == height,
            "Resizing the window did not give the expected frame size. Requested %d x %d but got %d x %d.", width, height,
            newDeviceSize.width(), newDeviceSize.height());
    }

    void MaterialEditorWindow::LockViewportRenderTargetSize(uint32_t width, uint32_t height)
    {
        m_materialViewport->LockRenderTargetSize(width, height);
    }

    void MaterialEditorWindow::UnlockViewportRenderTargetSize()
    {
        m_materialViewport->UnlockRenderTargetSize();
    }

    bool MaterialEditorWindow::GetCreateDocumentParams(AZStd::string& openPath, AZStd::string& savePath)
    {
        AtomToolsFramework::CreateDocumentDialog createDialog(
            tr("Create Material"),
            tr("Select Material Type"),
            tr("Select Material Path"),
            QString(AZ::Utils::GetProjectPath().c_str()) + AZ_CORRECT_FILESYSTEM_SEPARATOR + "Assets",
            { AZ::RPI::MaterialSourceData::Extension },
            AtomToolsFramework::GetSettingsObject<AZ::Data::AssetId>(
                "/O3DE/Atom/MaterialEditor/DefaultMaterialTypeAsset",
                AZ::RPI::AssetUtils::GetAssetIdForProductPath("materials/types/standardpbr.azmaterialtype")),
            [](const AZ::Data::AssetInfo& assetInfo) {
                return AZ::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), ".azmaterialtype"); },
            this);
        createDialog.adjustSize();

        if (createDialog.exec() == QDialog::Accepted &&
            !createDialog.m_targetPath.isEmpty() &&
            !createDialog.m_sourcePath.isEmpty())
        {
            savePath = createDialog.m_targetPath.toUtf8().constData();
            openPath = createDialog.m_sourcePath.toUtf8().constData();
            return true;
        }
        return false;
    }

    bool MaterialEditorWindow::GetOpenDocumentParams(AZStd::string& openPath)
    {
        const AZStd::vector<AZ::Data::AssetType> assetTypes = { azrtti_typeid<AZ::RPI::MaterialAsset>() };
        openPath = AtomToolsFramework::GetOpenFileInfo(assetTypes).absoluteFilePath().toUtf8().constData();
        return !openPath.empty();
    }

    void MaterialEditorWindow::OpenSettings()
    {
        SettingsDialog dialog(this);
        dialog.exec();
    }

    void MaterialEditorWindow::OpenHelp()
    {
        QMessageBox::information(
            this, windowTitle(),
            R"(<html><head/><body>
            <p><h3><u>Material Editor Controls</u></h3></p>
            <p><b>LMB</b> - pan camera</p>
            <p><b>RMB</b> or <b>Alt+LMB</b> - orbit camera around target</p>
            <p><b>MMB</b> or <b>Alt+MMB</b> - move camera on its xy plane</p>
            <p><b>Alt+RMB</b> or <b>LMB+RMB</b> - dolly camera on its z axis</p>
            <p><b>Ctrl+LMB</b> - rotate model</p>
            <p><b>Shift+LMB</b> - rotate environment</p>
            </body></html>)");
    }
} // namespace MaterialEditor

#include <Window/moc_MaterialEditorWindow.cpp>
