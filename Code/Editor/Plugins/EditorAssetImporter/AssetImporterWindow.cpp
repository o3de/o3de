/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetImporterWindow.h>
#include <ui_AssetImporterWindow.h>
#include <AssetImporterPlugin.h>
#include <ImporterRootDisplay.h>

#include <QTimer>
#include <QFile>
#include <QFileDialog>
#include <QCloseEvent>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDockWidget>
#include <QLabel>

class IXMLDOMDocumentPtr; // Needed for settings.h
class CXTPDockingPaneLayout; // Needed for settings.h
#include <Settings.h>

#include <AzQtComponents/Components/StylesheetPreprocessor.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/functional.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/string/conversions.h>
#include <Util/PathUtil.h>
#include <ActionOutput.h>

#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/ProcessingOverlayWidget.h>
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/AsyncOperationProcessingHandler.h>
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/ExportJobProcessingHandler.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>
#include <SceneAPI/SceneUI/SceneWidgets/SceneGraphInspectWidget.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/Rules/ScriptProcessorRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IScriptProcessorRule.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>

const char* AssetImporterWindow::s_documentationWebAddress = "https://www.o3de.org/docs/user-guide/assets/scene-settings/";
const AZ::Uuid AssetImporterWindow::s_browseTag = AZ::Uuid::CreateString("{C240D2E1-BFD2-4FFA-BB5B-CC0FA389A5D3}");

AssetImporterWindow::AssetImporterWindow()
    : AssetImporterWindow(nullptr)
{
}

AssetImporterWindow::AssetImporterWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::AssetImporterWindow)
    , m_assetImporterDocument(new AssetImporterDocument())
    , m_serializeContext(nullptr)
    , m_rootDisplay(nullptr)
    , m_overlay(nullptr)
    , m_isClosed(false)
    , m_processingOverlayIndex(AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex)
{
    Init();
}

AssetImporterWindow::~AssetImporterWindow()
{
    AZ_Assert(m_processingOverlayIndex == AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex,
        "Processing overlay (and potentially background thread) still active at destruction.");
    AZ_Assert(!m_processingOverlay, "Processing overlay (and potentially background thread) still active at destruction.");
}

void AssetImporterWindow::OpenFile(const AZStd::string& filePath)
{
    if (m_processingOverlay)
    {
        QMessageBox::warning(this, "In progress", "Please wait for the previous task to complete before opening a new file.");
        return;
    }

    if (!m_overlay->CanClose())
    {
        QMessageBox::warning(this, "In progress", "Unable to close one or more windows at this time.");
        return;
    }

    // Make sure we are not browsing *over* a current editing operation
    if (!IsAllowedToChangeSourceFile())
    {
        // Issue will already have been reported to the user.
        return;
    }

    if (!m_overlay->PopAllLayers())
    {
        QMessageBox::warning(this, "In progress", "Unable to close one or more windows at this time.");
        return;
    }

    OpenFileInternal(filePath);
}

void AssetImporterWindow::closeEvent(QCloseEvent* ev)
{
    if (m_isClosed)
    {
        return;
    }

    if (m_processingOverlay)
    {
        AZ_Assert(m_processingOverlayIndex != AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex,
            "Processing overlay present, but not the index in the overlay for it.");
        if (m_processingOverlay->HasProcessingCompleted())
        {
            if (m_overlay->PopLayer(m_processingOverlayIndex))
            {
                m_processingOverlayIndex = AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex;
                m_processingOverlay.reset(nullptr);
            }
            else
            {
                QMessageBox::critical(this, "Processing In Progress", "Unable to close the result window at this time.",
                    QMessageBox::Ok, QMessageBox::Ok);
                ev->ignore();
                return;
            }
        }
        else
        {
            QMessageBox::critical(this, "Processing In Progress", "Please wait until processing has completed to try again.",
                QMessageBox::Ok, QMessageBox::Ok);
            ev->ignore();
            return;
        }
    }

    if (!m_overlay->CanClose())
    {
        QMessageBox::critical(this, "Unable to close", "Unable to close one or more windows at this time.",
            QMessageBox::Ok, QMessageBox::Ok);
        ev->ignore();
        return;
    }

    if (!IsAllowedToChangeSourceFile())
    {
        ev->ignore();
        return;
    }

    ev->accept();
    m_isClosed = true;
}

void AssetImporterWindow::Init()
{
    // Serialization and reflection framework setup
    EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(m_serializeContext, "Serialization context not available");

    // Load the style sheets
    AzQtComponents::StylesheetPreprocessor styleSheetProcessor(nullptr);

    auto mainWindowQSSPath = AZ::IO::Path(AZ::Utils::GetEnginePath()) / "Assets";
    mainWindowQSSPath /= "Editor/Styles/AssetImporterWindow.qss";
    mainWindowQSSPath.MakePreferred();
    QFile mainWindowStyleSheetFile(mainWindowQSSPath.c_str());
    if (mainWindowStyleSheetFile.open(QFile::ReadOnly))
    {
        setStyleSheet(styleSheetProcessor.ProcessStyleSheet(mainWindowStyleSheetFile.readAll()));
    }

    ui->setupUi(this);

    if (!gSettings.enableSceneInspector)
    {
        ui->m_actionInspect->setVisible(false);
    }

    ResetMenuAccess(WindowState::InitialNothingLoaded);

    // Setup the overlay system, and set the root to be the root display. The root display has the browse,
    //  the Import button & the cancel button, which are handled here by the window.
    m_overlay.reset(aznew AZ::SceneAPI::UI::OverlayWidget(this));
    m_rootDisplay.reset(aznew ImporterRootDisplay(m_serializeContext));
    connect(m_rootDisplay.data(), &ImporterRootDisplay::UpdateClicked, this, &AssetImporterWindow::UpdateClicked);

    connect(m_overlay.data(), &AZ::SceneAPI::UI::OverlayWidget::LayerAdded, this, &AssetImporterWindow::OverlayLayerAdded);
    connect(m_overlay.data(), &AZ::SceneAPI::UI::OverlayWidget::LayerRemoved, this, &AssetImporterWindow::OverlayLayerRemoved);

    m_overlay->SetRoot(m_rootDisplay.data());
    ui->m_mainArea->layout()->addWidget(m_overlay.data());

    // Filling the initial browse prompt text to be programmatically set from available extensions
    AZStd::unordered_set<AZStd::string> extensions;
    EBUS_EVENT(AZ::SceneAPI::Events::AssetImportRequestBus, GetSupportedFileExtensions, extensions);
    AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, !extensions.empty(), "No file extensions defined for assets.");
    if (!extensions.empty())
    {
        for (AZStd::string& extension : extensions)
        {
            extension = extension.substr(1);
            AZStd::to_upper(extension.begin(), extension.end());
        }

        AZStd::string joinedExtensions;
        AzFramework::StringFunc::Join(joinedExtensions, extensions.begin(), extensions.end(), " or ");

        AZStd::string firstLineText =
            AZStd::string::format(
                "%s files are available for use after placing them in any folder within your game project. "
                "These files will automatically be processed and may be accessed via the Asset Browser. <a href=\"%s\">Learn more...</a>",
                joinedExtensions.c_str(), s_documentationWebAddress);

        ui->m_initialPromptFirstLine->setText(firstLineText.c_str());

        AZStd::string secondLineText =
            AZStd::string::format("To adjust the %s settings, right-click the file in the Asset Browser and select \"Edit Settings\" from the context menu.", joinedExtensions.c_str());
        ui->m_initialPromptSecondLine->setText(secondLineText.c_str());
    }
    else
    {
        AZStd::string firstLineText =
            AZStd::string::format(
                "Files are available for use after placing them in any folder within your game project. "
                "These files will automatically be processed and may be accessed via the Asset Browser. <a href=\"%s\">Learn more...</a>", s_documentationWebAddress);

        ui->m_initialPromptFirstLine->setText(firstLineText.c_str());

        AZStd::string secondLineText = "To adjust the settings, right-click the file in the Asset Browser and select \"Edit Settings\" from the context menu.";
        ui->m_initialPromptSecondLine->setText(secondLineText.c_str());

        // Hide the initial browse container so we can show the error (it will be shown again when the overlay pops)
        ui->m_initialBrowseContainer->hide();

        QMessageBox::critical(this, "No Extensions Detected",
            "No importable file types were detected. This likely means an internal error has taken place which has broken the "
            "registration of valid import types (e.g. FBX). This type of issue requires engineering support.");
    }
}

void AssetImporterWindow::OpenFileInternal(const AZStd::string& filePath)
{
    using namespace AZ::SceneAPI::SceneUI;

    auto asyncLoadHandler = AZStd::make_shared<AZ::SceneAPI::SceneUI::AsyncOperationProcessingHandler>(
        s_browseTag,
        [this, filePath]()
        {
            m_assetImporterDocument->LoadScene(filePath);

            // Update the UI on the main thread.
            QTimer::singleShot(0, [&]() { UpdateSceneDisplay({}); });
            
        },
        [this]()
        {
            HandleAssetLoadingCompleted();
        }, this);

    m_processingOverlay.reset(new ProcessingOverlayWidget(m_overlay.data(), ProcessingOverlayWidget::Layout::Loading, s_browseTag));
    m_processingOverlay->SetAndStartProcessingHandler(asyncLoadHandler);
    m_processingOverlay->SetAutoCloseOnSuccess(true);
    connect(m_processingOverlay.data(), &AZ::SceneAPI::SceneUI::ProcessingOverlayWidget::Closing, this, &AssetImporterWindow::ClearProcessingOverlay);
    m_processingOverlayIndex = m_processingOverlay->PushToOverlay();
}

bool AssetImporterWindow::IsAllowedToChangeSourceFile()
{
    if (!m_rootDisplay->HasUnsavedChanges())
    {
        return true;
    }

    const int result = QMessageBox::question(
        this,
        tr("Save Changes?"),
        tr("Changes have been made to the asset during this session. Would you like to save prior to closing?"),
        QMessageBox::Yes,
        QMessageBox::No,
        QMessageBox::Cancel);

    if (result == QMessageBox::Cancel)
    {
        return false;
    }
    else if (result == QMessageBox::No)
    {
        return true;
    }

    AZStd::shared_ptr<AZ::ActionOutput> output = AZStd::make_shared<AZ::ActionOutput>();
    m_assetImporterDocument->SaveScene(
        output,
        [this](bool wasSuccessful)
        {
            if(wasSuccessful)
            {
                m_isClosed = true;

                // Delete the parent, because this window is nested inside another, dockable window.
                // Just deleting this will leave the dockable window open.
                // Requesting the panel that this is docked in to close, will result in issues on some re-open states,
                // if only the panel is closed, then the next time it's opened, the scene settings won't be correctly loaded.
                this->parent()->deleteLater();
            }
            else
            {
                QMessageBox messageBox(
                    QMessageBox::Icon::Warning,
                    tr("Failed to save"),
                    tr("An error has been encountered saving this file. See the logs for details."));
                messageBox.exec();
            }
        });

    // Don't close yet, in case the save fails.
    // Scene saving is asynchronous, and will close the panel if it's successful.
    return false;
}

void AssetImporterWindow::UpdateClicked()
{
    using namespace AZ::SceneAPI::SceneUI;

    // There are specific measures in place to block re-entry, applying asserts to be safe
    if (m_processingOverlay)
    {
        AZ_Assert(!m_processingOverlay, "Attempted to update asset while processing is in progress.");
        return;
    }
    else if (!m_scriptProcessorRuleFilename.empty())
    {
        AZ_TracePrintf(AZ::SceneAPI::Utilities::WarningWindow, "A script updates the manifest; will not save.");
        return;
    }

    m_processingOverlay.reset(new ProcessingOverlayWidget(m_overlay.data(), ProcessingOverlayWidget::Layout::Exporting, s_browseTag));
    connect(m_processingOverlay.data(), &ProcessingOverlayWidget::Closing, this, &AssetImporterWindow::ClearProcessingOverlay);
    m_processingOverlayIndex = m_processingOverlay->PushToOverlay();

    // We need to block closing of the overlay until source control operations are complete
    m_processingOverlay->BlockClosing();

    m_processingOverlay->OnSetStatusMessage("Saving settings...");
    bool isSourceControlActive = false;
    {
        using SCRequestBus = AzToolsFramework::SourceControlConnectionRequestBus;
        SCRequestBus::BroadcastResult(isSourceControlActive, &SCRequestBus::Events::IsActive);
    }

    AZStd::shared_ptr<AZ::ActionOutput> output = AZStd::make_shared<AZ::ActionOutput>();
    m_assetImporterDocument->SaveScene(output,
        [output, this, isSourceControlActive](bool wasSuccessful)
        {
            if (output->HasAnyWarnings())
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::WarningWindow, "%s", output->BuildWarningMessage().c_str());
            }
            if (output->HasAnyErrors())
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "%s", output->BuildErrorMessage().c_str());
            }

            if (wasSuccessful)
            {
                if (!isSourceControlActive)
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::SuccessWindow, "Saving complete");
                }
                else
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::SuccessWindow, "Saving & source control operations complete");
                }

                m_rootDisplay->HandleSaveWasSuccessful();

                // Don't attach the job processor until all files are saved.
                m_processingOverlay->SetAndStartProcessingHandler(AZStd::make_shared<ExportJobProcessingHandler>(s_browseTag, m_fullSourcePath));
            }
            else
            {
                // This kind of failure means that it's possible the jobs will never actually start,
                //  so we act like the processing is complete to make it so the user won't be stuck
                //  in the processing UI in that case.
                m_processingOverlay->OnProcessingComplete();
            }

            // Blocking is only used for the period saving is happening. The ExportJobProcessingHandler will inform
            // the overlay widget when the AP is done with processing, which will also block closing until done.
            m_processingOverlay->UnblockClosing();
        }
    );
}

void AssetImporterWindow::OnSceneResetRequested()
{
    using namespace AZ::SceneAPI::Events;
    using namespace AZ::SceneAPI::SceneUI;
    using namespace AZ::SceneAPI::Utilities;

    auto asyncLoadHandler = AZStd::make_shared<AZ::SceneAPI::SceneUI::AsyncOperationProcessingHandler>(
        s_browseTag,
        [this]()
        {
            m_assetImporterDocument->GetScene()->GetManifest().Clear();

            AZ::SceneAPI::Events::ProcessingResultCombiner result;
            EBUS_EVENT_RESULT(result, AssetImportRequestBus, UpdateManifest, *m_assetImporterDocument->GetScene(),
                AssetImportRequest::ManifestAction::ConstructDefault, AssetImportRequest::RequestingApplication::Editor);

            // Specifically using success, because ignore would be an invalid case.
            // Whenever we do construct default, it should always be done
            if (result.GetResult() == ProcessingResult::Success)
            {
                AZ_TracePrintf(SuccessWindow, "Successfully reset the manifest.");
            }
            else
            {
                m_assetImporterDocument->ClearScene();
                AZ_TracePrintf(ErrorWindow, "Manifest reset returned in '%s'",
                    result.GetResult() == ProcessingResult::Failure ? "Failure" : "Ignored");
            }
        },
        [this]()
        {
            m_rootDisplay->HandleSceneWasReset(m_assetImporterDocument->GetScene());
        }, this);

    // reset the script rule from the .assetinfo file if it exists
    if (!m_scriptProcessorRuleFilename.empty())
    {
        m_scriptProcessorRuleFilename.clear();
        if (QFile::exists(m_assetImporterDocument->GetScene()->GetManifestFilename().c_str()))
        {
            QFile file(m_assetImporterDocument->GetScene()->GetManifestFilename().c_str());
            file.remove();
        }
    }
    UpdateSceneDisplay({});

    m_processingOverlay.reset(new ProcessingOverlayWidget(m_overlay.data(), ProcessingOverlayWidget::Layout::Resetting, s_browseTag));
    m_processingOverlay->SetAndStartProcessingHandler(asyncLoadHandler);
    m_processingOverlay->SetAutoCloseOnSuccess(true);
    connect(m_processingOverlay.data(), &ProcessingOverlayWidget::Closing, this, &AssetImporterWindow::ClearProcessingOverlay);
    m_processingOverlayIndex = m_processingOverlay->PushToOverlay();
}

void AssetImporterWindow::OnAssignScript()
{
    using namespace AZ::SceneAPI;
    using namespace AZ::SceneAPI::Events;
    using namespace AZ::SceneAPI::SceneUI;
    using namespace AZ::SceneAPI::Utilities;

    // use QFileDialog to select a Python script to embed into a scene manifest file
    QString pyFilename = QFileDialog::getOpenFileName(this,
        tr("Select scene builder Python script"),
        Path::GetEditingGameDataFolder().c_str(),
        tr("Python (*.py)"));

    if (pyFilename.isNull())
    {
        return;
    }

    // reset the script rule from the .assetinfo file if it exists
    if (!m_scriptProcessorRuleFilename.empty())
    {
        m_scriptProcessorRuleFilename.clear();
        if (QFile::exists(m_assetImporterDocument->GetScene()->GetManifestFilename().c_str()))
        {
            QFile file(m_assetImporterDocument->GetScene()->GetManifestFilename().c_str());
            file.remove();
        }
    }

    // find the path relative to the project folder
    pyFilename = Path::GetRelativePath(pyFilename, true);

    // create a script rule
    auto scriptProcessorRule = AZStd::make_shared<SceneData::ScriptProcessorRule>();
    scriptProcessorRule->SetScriptFilename(pyFilename.toUtf8().toStdString().c_str());

    // add the script rule to the manifest & save off the scene manifest
    Containers::SceneManifest sceneManifest;
    sceneManifest.AddEntry(scriptProcessorRule);
    if (sceneManifest.SaveToFile(m_assetImporterDocument->GetScene()->GetManifestFilename()))
    {
        OpenFile(m_assetImporterDocument->GetScene()->GetSourceFilename());
    }
}

void AssetImporterWindow::ResetMenuAccess(WindowState state)
{
    if (state == WindowState::FileLoaded)
    {
        ui->m_actionResetSettings->setEnabled(true);
        ui->m_actionInspect->setEnabled(true);
    }
    else
    {
        ui->m_actionResetSettings->setEnabled(false);
        ui->m_actionInspect->setEnabled(false);
    }
}

void AssetImporterWindow::OnOpenDocumentation()
{
    QDesktopServices::openUrl(QString(s_documentationWebAddress));
}

void AssetImporterWindow::OnInspect()
{
    AZ::SceneAPI::UI::OverlayWidgetButtonList buttons;
    AZ::SceneAPI::UI::OverlayWidgetButton closeButton;
    closeButton.m_text = "Close";
    closeButton.m_triggersPop = true;
    buttons.push_back(&closeButton);

    QLabel* label = new QLabel("Please close the inspector to continue editing the settings.");
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);

    // make sure the inspector doesn't outlive the AssetImporterWindow, since we own the data it will be inspecting.
    auto* theInspectWidget = aznew AZ::SceneAPI::UI::SceneGraphInspectWidget(*m_assetImporterDocument->GetScene());
    QObject::connect(this, &QObject::destroyed, theInspectWidget, [theInspectWidget]() { theInspectWidget->window()->close(); } );

    m_overlay->PushLayer(label, theInspectWidget, "Scene Inspector", buttons);
}

void AssetImporterWindow::OverlayLayerAdded()
{
    setCursor(Qt::WaitCursor);
    ResetMenuAccess(WindowState::OverlayShowing);
}

void AssetImporterWindow::OverlayLayerRemoved()
{
    if (m_isClosed && !m_overlay->IsAtRoot())
    {
        return;
    }

    setCursor(Qt::ArrowCursor);

    // Reset menu access
    if (m_assetImporterDocument->GetScene())
    {
        ResetMenuAccess(WindowState::FileLoaded);
    }
    else
    {
        ResetMenuAccess(WindowState::InitialNothingLoaded);

        ui->m_initialBrowseContainer->show();
        m_rootDisplay->hide();
    }
}

void AssetImporterWindow::SetTitle(const char* filePath)
{
    QWidget* dock = parentWidget();
    while (dock)
    {
        QDockWidget* converted = qobject_cast<QDockWidget*>(dock);
        if (converted)
        {
            AZStd::string extension;
            if (AzFramework::StringFunc::Path::GetExtension(filePath, extension, false))
            {
                extension[0] = static_cast<char>(toupper(extension[0]));
                for (size_t i = 1; i < extension.size(); ++i)
                {
                    extension[i] = static_cast<char>(tolower(extension[i]));
                }
            }
            else
            {
                extension = "Scene";
            }
            AZStd::string fileName;
            AzFramework::StringFunc::Path::GetFileName(filePath, fileName);
            converted->setWindowTitle(QString("%1 Settings - %2").arg(extension.c_str(), fileName.c_str()));
            break;
        }
        else
        {
            dock = dock->parentWidget();
        }
    }
}

void AssetImporterWindow::UpdateSceneDisplay(const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene> scene) const
{
    QString sceneHeaderText;
    if(scene.get())
    {
        sceneHeaderText = QString::fromUtf8(scene->GetManifestFilename().c_str(), static_cast<int>(scene->GetManifestFilename().size()));
    }
    if (!m_scriptProcessorRuleFilename.empty())
    {
        sceneHeaderText.append("\n Assigned Python builder script (")
            .append(m_scriptProcessorRuleFilename.c_str())
            .append(")");
    }

    if (scene)
    {
        m_rootDisplay->SetSceneDisplay(sceneHeaderText, scene);
    }
    else
    {
        m_rootDisplay->SetSceneHeaderText(sceneHeaderText);
    }
}

void AssetImporterWindow::HandleAssetLoadingCompleted()
{
    if (!m_assetImporterDocument->GetScene())
    {
        AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Failed to load scene.");
        return;
    }

    m_fullSourcePath = m_assetImporterDocument->GetScene()->GetSourceFilename();
    SetTitle(m_fullSourcePath.c_str());

    using namespace AZ::SceneAPI;
    m_scriptProcessorRuleFilename.clear();

    // load up the source scene manifest file
    Containers::SceneManifest sceneManifest;
    if (sceneManifest.LoadFromFile(m_assetImporterDocument->GetScene()->GetManifestFilename()))
    {
        // check a Python script rule is in that source manifest
        auto view = Containers::MakeDerivedFilterView<DataTypes::IScriptProcessorRule>(sceneManifest.GetValueStorage());
        if (!view.empty())
        {
            // record the info about the rule in the class
            const auto scriptProcessorRule = &*view.begin();
            m_scriptProcessorRuleFilename = scriptProcessorRule->GetScriptFilename();
        }
    }

    UpdateSceneDisplay(m_assetImporterDocument->GetScene());

    // Once we've browsed to something successfully, we need to hide the initial browse button layer and
    //  show the main area where all the actual work takes place
    ui->m_initialBrowseContainer->hide();
    m_rootDisplay->show();
}

void AssetImporterWindow::ClearProcessingOverlay()
{
    m_processingOverlayIndex = AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex;
    m_processingOverlay.reset(nullptr);
}

#include <moc_AssetImporterWindow.cpp>
