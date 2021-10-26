/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : A dialog for batch-rendering sequences


#include "EditorDefs.h"

#include "SequenceBatchRenderDialog.h"

#include <AzCore/Component/ComponentApplication.h>
#include <AzFramework/Windowing/WindowBus.h>

// Qt
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QStringListModel>
#include <QtConcurrent>

// CryCommon
#include <CryCommon/Maestro/Types/AnimNodeType.h>

// Editor
#include "MainWindow.h"
#include "CustomResolutionDlg.h"
#include "ViewPane.h"
#include "GameEngine.h"
#include "Include/ICommandManager.h"
#include "CryEdit.h"
#include "Viewport.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <TrackView/ui_SequenceBatchRenderDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

namespace
{
    const int g_useActiveViewportResolution = -1;   // reserved value to indicate the use of the active viewport resolution
    int resolutions[][2] = {
        {1280, 720},
        {1920, 1080},
        {1998, 1080},
        {2048, 858},
        {2560, 1440},
        {3840, 2160},
        { g_useActiveViewportResolution, g_useActiveViewportResolution }    // active viewport res must be the last element of the resolution array
    };

    // cached current active viewport resolution
    int activeViewportWidth;
    int activeViewportHeight;

    struct SFPSPair
    {
        int fps;
        const char* fpsDesc;
    } fpsOptions[] = {
        {24, "Film(24)"}, {25, "PAL(25)"}, {30, "NTSC(30)"},
        {48, "Show(48)"}, {50, "PAL Field(50)"}, {60, "NTSC Field(60)"}
    };

    // currently supported file extensions
    const char* imageFormatExtensions[] = {"dds", "ppm"};

    const char defaultPresetFilename[] = "defaultBatchRender.preset";

    const char customResFormat[] = "Custom(%1 x %2)...";

    const int kBatchRenderFileVersion = 2; // This version number should be incremented every time available options like the list of formats,
    // the list of buffers change.

    // get the actual render width to use (substitutes active viewport width if needed)
    int getResWidth(int renderItemWidth)
    {
        return (renderItemWidth == g_useActiveViewportResolution) ? activeViewportWidth : renderItemWidth;
    }
    // get the actual render height to use (substitutes active viewport height if needed)
    int getResHeight(int renderItemHeight)
    {
        return (renderItemHeight == g_useActiveViewportResolution) ? activeViewportHeight : renderItemHeight;
    }
}

static void UpdateAtomOutputFrameCaptureView(TrackView::AtomOutputFrameCapture& atomOutputFrameCapture, const int width, const int height)
{
    const AZ::EntityId activeCameraEntityId = TrackView::ActiveCameraEntityId();
    atomOutputFrameCapture.UpdateView(
        TrackView::TransformFromEntityId(activeCameraEntityId),
        TrackView::ProjectionFromCameraEntityId(activeCameraEntityId, static_cast<float>(width), static_cast<float>(height)));
}

CSequenceBatchRenderDialog::CSequenceBatchRenderDialog(float fps, QWidget* pParent /* = nullptr */)
    : QDialog(pParent)
    , m_fpsForTimeToFrameConversion(fps)
    , m_customResH(0)
    , m_customResW(0)
    , m_customFPS(0)
    , m_bFFMPEGCommandAvailable(false)
    , m_ui(new Ui::SequenceBatchRenderDialog)
    , m_renderListModel(new QStringListModel(this))
    , CV_TrackViewRenderOutputCapturing(0)
    , m_prefixValidator(new CPrefixValidator(this))
{
    m_ui->setupUi(this);
    setFixedSize(size());
    m_ui->m_renderList->setModel(m_renderListModel);

    OnInitDialog();

    connect(&m_renderTimer, &QTimer::timeout, this, &CSequenceBatchRenderDialog::OnKickIdleTimout);
    m_renderTimer.setInterval(0);
    m_renderTimer.setSingleShot(true);

    REGISTER_CVAR3("TrackViewRenderOutputCapturing", CV_TrackViewRenderOutputCapturing, 0, VF_NULL, "Set to 1 when Track View is actively capturing render output.");
}

CSequenceBatchRenderDialog::~CSequenceBatchRenderDialog()
= default;

void CSequenceBatchRenderDialog::reject()
{
    if (m_renderContext.IsInRendering())
    {
        OnCancelRender();
    }
    else
    {
        QDialog::reject();
    }
}

void CSequenceBatchRenderDialog::OnInitDialog()
{
    QAction* browseAction = m_ui->m_destinationEdit->addAction(style()->standardPixmap(QStyle::SP_DirOpenIcon), QLineEdit::TrailingPosition);
    connect(browseAction, &QAction::triggered, this, [=]()
    {
        const QString dir = QFileDialog::getExistingDirectory(this);
        if (!dir.isEmpty())
        {
            m_ui->m_destinationEdit->setText(dir);
        }
    });

    void(QComboBox::* activated)(int) = &QComboBox::activated;
    void(QSpinBox::* editingFinished)() = &QSpinBox::editingFinished;

    connect(m_ui->BATCH_RENDER_ADD_SEQ, &QPushButton::clicked, this, &CSequenceBatchRenderDialog::OnAddRenderItem);
    connect(m_ui->BATCH_RENDER_REMOVE_SEQ, &QPushButton::clicked, this, &CSequenceBatchRenderDialog::OnRemoveRenderItem);
    connect(m_ui->BATCH_RENDER_CLEAR_SEQ, &QPushButton::clicked, this, &CSequenceBatchRenderDialog::OnClearRenderItems);
    connect(m_ui->m_updateBtn, &QPushButton::clicked, this, &CSequenceBatchRenderDialog::OnUpdateRenderItem);
    connect(m_ui->BATCH_RENDER_LOAD_PRESET, &QPushButton::clicked, this, &CSequenceBatchRenderDialog::OnLoadPreset);
    connect(m_ui->BATCH_RENDER_SAVE_PRESET, &QPushButton::clicked, this, &CSequenceBatchRenderDialog::OnSavePreset);
    connect(m_ui->BATCH_RENDER_LOAD_BATCH, &QPushButton::clicked, this, &CSequenceBatchRenderDialog::OnLoadBatch);
    connect(m_ui->BATCH_RENDER_SAVE_BATCH, &QPushButton::clicked, this, &CSequenceBatchRenderDialog::OnSaveBatch);
    connect(m_ui->m_pGoBtn, &QPushButton::clicked, this, &CSequenceBatchRenderDialog::OnGo);
    connect(m_ui->CANCEL, &QPushButton::clicked, this, &CSequenceBatchRenderDialog::OnDone);
    connect(m_ui->m_sequenceCombo, activated, this, &CSequenceBatchRenderDialog::OnSequenceSelected);
    connect(m_ui->m_fpsCombo->lineEdit(), &QLineEdit::textEdited, this, &CSequenceBatchRenderDialog::OnFPSEditChange);
    connect(m_ui->m_fpsCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CSequenceBatchRenderDialog::OnFPSChange);
    connect(m_ui->m_renderList->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CSequenceBatchRenderDialog::OnRenderItemSelChange);
    connect(m_ui->m_resolutionCombo, activated, this, &CSequenceBatchRenderDialog::OnResolutionSelected);
    connect(m_ui->m_startFrame, editingFinished, this, &CSequenceBatchRenderDialog::OnStartFrameChange);
    connect(m_ui->m_endFrame, editingFinished, this, &CSequenceBatchRenderDialog::OnEndFrameChange);
    connect(m_ui->m_imageFormatCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CSequenceBatchRenderDialog::OnImageFormatChange);

    const int bigEnoughNumber = 1000000;
    m_ui->m_startFrame->setRange(0, bigEnoughNumber);

    m_ui->m_endFrame->setRange(0, bigEnoughNumber);

    // Fill the sequence combo box.
    bool activeSequenceWasSet = false;
    for (int k = 0; k < GetIEditor()->GetMovieSystem()->GetNumSequences(); ++k)
    {
        IAnimSequence* pSequence = GetIEditor()->GetMovieSystem()->GetSequence(k);
        m_ui->m_sequenceCombo->addItem(pSequence->GetName());
        if (pSequence->IsActivated())
        {
            m_ui->m_sequenceCombo->setCurrentIndex(k);
            activeSequenceWasSet = true;
        }
    }
    if (!activeSequenceWasSet)
    {
        m_ui->m_sequenceCombo->setCurrentIndex(0);
    }

    m_ui->m_fpsCombo->setEditable(true);

    // Fill the shot combos and the default frame range.
    OnSequenceSelected();

    // Fill the resolution combo box.
    for (int i = 0; i < arraysize(resolutions); ++i)
    {
        if (resolutions[i][0] == g_useActiveViewportResolution  && resolutions[i][1] == g_useActiveViewportResolution)
        {
            m_ui->m_resolutionCombo->addItem(tr("Active View Resolution"));
            stashActiveViewportResolution();    // render dialog is modal, so we can stash the viewport res on init
        }
        else
        {
            m_ui->m_resolutionCombo->addItem(tr("%1 x %2").arg(resolutions[i][0]).arg(resolutions[i][1]));
        }
    }
    m_ui->m_resolutionCombo->addItem(tr("Custom..."));
    m_ui->m_resolutionCombo->setCurrentIndex(0);

    // Fill the FPS combo box.
    for (int i = 0; i < AZStd::size(fpsOptions); ++i)
    {
        m_ui->m_fpsCombo->addItem(fpsOptions[i].fpsDesc);
    }
    m_ui->m_fpsCombo->setCurrentIndex(0);

    // Fill the image format combo box.
    for (int i = 0; i < AZStd::size(imageFormatExtensions); ++i)
    {
        m_ui->m_imageFormatCombo->addItem(imageFormatExtensions[i]);
    }
    m_ui->m_imageFormatCombo->setCurrentIndex(0);

    m_ui->BATCH_RENDER_FILE_PREFIX->setText("Frame");
    m_ui->BATCH_RENDER_FILE_PREFIX->setValidator(m_prefixValidator.data());

    m_ui->m_progressStatusMsg->setText("Not running");

    m_ui->BATCH_RENDER_REMOVE_SEQ->setEnabled(false);
    m_ui->m_pGoBtn->setEnabled(false);
    m_ui->m_pGoBtn->setIcon(QPixmap(":/Trackview/clapperboard_ready.png"));

    m_ui->m_progressBar->setRange(0, 100);

    m_ui->BATCH_RENDER_FRAME_IN_FPS->setText(tr("In %1 FPS").arg(static_cast<int>(m_fpsForTimeToFrameConversion)));

    m_bFFMPEGCommandAvailable = GetIEditor()->GetICommandManager()->IsRegistered("plugin", "ffmpeg_encode");
    m_ffmpegPluginStatusMsg = m_bFFMPEGCommandAvailable ?
        QString("") :
        tr("FFMPEG plug-in isn't found(creating a video isn't supported).");
    m_ui->BATCH_RENDER_PRESS_ESC_TO_CANCEL->setText(m_ffmpegPluginStatusMsg);

    // Disable the create video checkbox if the ffmpeg command is not available
    if (!m_bFFMPEGCommandAvailable)
    {
        m_ui->m_createVideoCheckBox->setChecked(false);
        m_ui->m_createVideoCheckBox->setEnabled(false);
    }

    // Load previously saved options, if any.
    QString defaultPresetPath = Path::GetUserSandboxFolder();
    defaultPresetPath += defaultPresetFilename;
    if (CFileUtil::FileExists(defaultPresetPath))
    {
        LoadOutputOptions(defaultPresetPath);
    }

    CheckForEnableUpdateButton();
}

void CSequenceBatchRenderDialog::OnRenderItemSelChange()
{
    // Enable/disable the 'remove'/'update' button properly.
    bool bNoSelection = !m_ui->m_renderList->selectionModel()->hasSelection();
    m_ui->BATCH_RENDER_REMOVE_SEQ->setEnabled(bNoSelection ? false : true);

    CheckForEnableUpdateButton();

    if (bNoSelection)
    {
        return;
    }

    // Apply the settings of the selected one to the dialog.
    const SRenderItem& item = m_renderItems[m_ui->m_renderList->currentIndex().row()];
    // sequence
    for (int i = 0; i < m_ui->m_sequenceCombo->count(); ++i)
    {
        const QString sequenceName = m_ui->m_sequenceCombo->itemText(i);
        if (sequenceName == item.pSequence->GetName())
        {
            m_ui->m_sequenceCombo->setCurrentIndex(i);
            OnSequenceSelected();
            break;
        }
    }
    // director
    for (int i = 0; i < m_ui->m_shotCombo->count(); ++i)
    {
        const QString directorName = m_ui->m_shotCombo->itemText(i);
        if (directorName == item.pDirectorNode->GetName())
        {
            m_ui->m_shotCombo->setCurrentIndex(i);
            break;
        }
    }
    // frame range
    m_ui->m_startFrame->setValue(static_cast<int>(item.frameRange.start * m_fpsForTimeToFrameConversion));
    m_ui->m_endFrame->setValue(static_cast<int>(item.frameRange.end * m_fpsForTimeToFrameConversion));
    // folder
    m_ui->m_destinationEdit->setText(item.folder);
    // fps
    bool bFound = false;
    for (int i = 0; i < arraysize(fpsOptions); ++i)
    {
        if (item.fps == fpsOptions[i].fps)
        {
            m_ui->m_fpsCombo->setCurrentIndex(i);
            bFound = true;
            break;
        }
    }
    if (bFound == false)
    {
        m_customFPS = item.fps;
        m_ui->m_fpsCombo->setCurrentText(QString::number(item.fps));
    }
    // prefix
    m_ui->BATCH_RENDER_FILE_PREFIX->setText(item.prefix);

    m_ui->m_disableDebugInfoCheckBox->setChecked(item.disableDebugInfo);

    // create_video
    if (m_bFFMPEGCommandAvailable)
    {
        m_ui->m_createVideoCheckBox->setChecked(item.bCreateVideo);
    }

    // resolution
    bFound = false;
    for (int i = 0; i < arraysize(resolutions); ++i)
    {
        if (item.resW == resolutions[i][0] && item.resH == resolutions[i][1])
        {
            m_ui->m_resolutionCombo->setCurrentIndex(i);
            bFound = true;
            break;
        }
    }
    if (bFound == false)
    {
        int indexOfCustomRes = arraysize(resolutions);
        const QString resText = QString::fromLatin1(customResFormat).arg(item.resW).arg(item.resH);
        m_customResW = item.resW;
        m_customResH = item.resH;
        m_ui->m_resolutionCombo->removeItem(indexOfCustomRes);
        m_ui->m_resolutionCombo->addItem(resText);
        m_ui->m_resolutionCombo->setCurrentIndex(indexOfCustomRes);
    }
    // cvars
    QString cvarsText;
    for (size_t i = 0; i < item.cvars.size(); ++i)
    {
        cvarsText += item.cvars[static_cast<int>(i)];
        cvarsText += "\r\n";
    }
    m_ui->m_cvarsEdit->setPlainText(cvarsText);
}

void CSequenceBatchRenderDialog::CheckForEnableUpdateButton()
{
    bool enable = false;

    // Enable the Update button if any ui elements are changed
    // from the currently selected render item.
    if (m_ui->m_renderList->selectionModel()->hasSelection())
    {
        SRenderItem item;
        if (SetUpNewRenderItem(item))
        {
            int index = m_ui->m_renderList->currentIndex().row();
            assert(index >= 0 && index < m_renderItems.size());
            enable = !(m_renderItems[index] == item);
        }
    }

    m_ui->m_updateBtn->setEnabled(enable);
}

void CSequenceBatchRenderDialog::OnAddRenderItem()
{
    // If there is no director node, it cannot be added.
    if (m_ui->m_shotCombo->count() == 0)
    {
        QMessageBox::critical(this, tr("Cannot add"), tr("No director available!"));
        return;
    }

    // Set up a new render item.
    SRenderItem item;
    if (SetUpNewRenderItem(item) == false)
    {
        return;
    }

    // Check a duplication before adding.
    for (size_t i = 0; i < m_renderItems.size(); ++i)
    {
        if (m_renderItems[i] == item)
        {
            QMessageBox::critical(this, tr("Cannot add"), tr("The same item already exists"));
            return;
        }
    }

    AddItem(item);
}

void CSequenceBatchRenderDialog::OnRemoveRenderItem()
{
    int index = m_ui->m_renderList->currentIndex().row();
    assert(index != CB_ERR);
    m_ui->m_renderList->model()->removeRow(index);
    m_renderItems.erase(m_renderItems.begin() + index);

    if (m_renderItems.empty())
    {
        m_ui->BATCH_RENDER_REMOVE_SEQ->setEnabled(false);
        m_ui->m_pGoBtn->setEnabled(false);
    }
    else
    {
        m_ui->m_renderList->setCurrentIndex(m_ui->m_renderList->model()->index(0, 0));
        OnRenderItemSelChange();
    }

    CheckForEnableUpdateButton();
}

void CSequenceBatchRenderDialog::OnClearRenderItems()
{
    m_ui->m_renderList->model()->removeRows(0, m_ui->m_renderList->model()->rowCount());
    m_renderItems.clear();

    m_ui->BATCH_RENDER_REMOVE_SEQ->setEnabled(false);
    m_ui->m_pGoBtn->setEnabled(false);

    CheckForEnableUpdateButton();
}

void CSequenceBatchRenderDialog::OnUpdateRenderItem()
{
    int index = m_ui->m_renderList->currentIndex().row();
    assert(index != -1);

    // Set up a new render item.
    SRenderItem item;
    SetUpNewRenderItem(item);

    // Check a duplication before updating.
    for (size_t i = 0; i < m_renderItems.size(); ++i)
    {
        if (m_renderItems[i] == item)
        {
            QMessageBox::critical(this, tr("Cannot update"), tr("The same item already exists!"));
            return;
        }
    }

    // Update the item.
    m_renderItems[index] = item;

    // Update the list box, too.
    m_ui->m_renderList->model()->setData(m_ui->m_renderList->model()->index(index, 0), GetCaptureItemString(item));

    m_ui->m_updateBtn->setEnabled(false);
}

void CSequenceBatchRenderDialog::OnLoadPreset()
{
    QString loadPath;
    if (CFileUtil::SelectFile("Preset Files (*.preset)", Path::GetUserSandboxFolder(), loadPath))
    {
        if (LoadOutputOptions(loadPath) == false)
        {
            QMessageBox::critical(this, tr("Cannot load"), tr("The file version is different!"));
        }
    }
}

void CSequenceBatchRenderDialog::OnSavePreset()
{
    QString savePath;
    if (CFileUtil::SelectSaveFile("Preset Files (*.preset)", "preset", Path::GetUserSandboxFolder(), savePath))
    {
        SaveOutputOptions(savePath);
    }
}

void CSequenceBatchRenderDialog::stashActiveViewportResolution()
{
    // stash active resolution in global vars
    activeViewportWidth = resolutions[0][0];
    activeViewportHeight = resolutions[0][1];
    CViewport* activeViewport = GetIEditor()->GetActiveView();
    if (activeViewport)
    {
        activeViewport->GetDimensions(&activeViewportWidth, &activeViewportHeight);
    }
}

void CSequenceBatchRenderDialog::OnGo()
{
    if (m_renderContext.IsInRendering())
    {
        OnCancelRender();
    }
    else
    {
        // Start a new batch.
        m_ui->m_pGoBtn->setText("Cancel");
        m_ui->m_pGoBtn->setIcon(QPixmap(":/Trackview/clapperboard_cancel.png"));
        // Inform the movie system that it soon will be in a batch-rendering mode.
        GetIEditor()->GetMovieSystem()->EnableBatchRenderMode(true);

        // Initialize the context.
        InitializeContext();

        // Trigger the first item.
        OnMovieEvent(IMovieListener::eMovieEvent_Stopped, nullptr);
    }
}

void CSequenceBatchRenderDialog::OnMovieEvent(IMovieListener::EMovieEvent event, IAnimSequence* pSequence)
{
    if (event == IMovieListener::eMovieEvent_Stopped
        || event == IMovieListener::eMovieEvent_Aborted)
    {
        // Finalize the current one, if any.
        if (pSequence)
        {
            EnterCaptureState(CaptureState::End);
            m_renderContext.endingSequence = pSequence;
            m_renderContext.canceled = (event == IMovieListener::eMovieEvent_Aborted);
        }
        else
        {
            // This is odd, but this is the condition that starts the first item capturing
            // when the user presses the start button.
            if (m_renderItems.size() > 0)
            {
                // Setup and trigger the first time
                m_renderContext.spentTime = 0.0f;
                m_renderContext.currentItemIndex = 0;
                CaptureItemStart();
            }
        }
    }
}

void CSequenceBatchRenderDialog::OnDone()
{
    if (m_renderContext.IsInRendering())
    {
        OnCancelRender();
    }
    else
    {
        // Save options when closed.
        QString defaultPresetPath = Path::GetUserSandboxFolder();
        defaultPresetPath += defaultPresetFilename;
        SaveOutputOptions(defaultPresetPath);

        reject();
    }
}

void CSequenceBatchRenderDialog::OnSequenceSelected()
{
    // Get the selected sequence.
    const QString seqName = m_ui->m_sequenceCombo->currentText();
    IAnimSequence* pSequence = GetIEditor()->GetMovieSystem()->FindLegacySequenceByName(seqName.toUtf8().data());

    // Adjust the frame range.
    float sFrame = pSequence->GetTimeRange().start * m_fpsForTimeToFrameConversion;
    float eFrame = pSequence->GetTimeRange().end * m_fpsForTimeToFrameConversion;
    m_ui->m_startFrame->setRange(0, static_cast<int>(eFrame));
    m_ui->m_endFrame->setRange(0, static_cast<int>(eFrame));

    // Set the default start/end frames properly.
    m_ui->m_startFrame->setValue(static_cast<int>(sFrame));
    m_ui->m_endFrame->setValue(static_cast<int>(eFrame));

    m_ui->m_shotCombo->clear();
    // Fill the shot combo box with the names of director nodes.
    for (int i = 0; i < pSequence->GetNodeCount(); ++i)
    {
        if (pSequence->GetNode(i)->GetType() == AnimNodeType::Director)
        {
            m_ui->m_shotCombo->addItem(pSequence->GetNode(i)->GetName());
        }
    }
    m_ui->m_shotCombo->setCurrentIndex(0);

    CheckForEnableUpdateButton();
}

void CSequenceBatchRenderDialog::OnFPSEditChange()
{
    const QString fpsText = m_ui->m_fpsCombo->currentText();
    bool ok;
    const int fps = fpsText.toInt(&ok);
    bool bInvalidInput = !ok || fps <= 0;

    if (bInvalidInput)
    {
        m_ui->m_fpsCombo->setCurrentIndex(0);
    }
    else
    {
        m_customFPS = fps;
    }

    CheckForEnableUpdateButton();
}

void CSequenceBatchRenderDialog::OnFPSChange(int itemIndex)
{
    m_customFPS = fpsOptions[itemIndex].fps;
    CheckForEnableUpdateButton();
}

void CSequenceBatchRenderDialog::OnImageFormatChange()
{
    CheckForEnableUpdateButton();
}

void CSequenceBatchRenderDialog::OnResolutionSelected()
{
    int indexOfCustomRes = arraysize(resolutions);
    if (m_ui->m_resolutionCombo->currentIndex() == indexOfCustomRes)
    {
        int defaultW;
        int defaultH;
        const QString currentCustomResText = m_ui->m_resolutionCombo->currentText();
        GetResolutionFromCustomResText(currentCustomResText.toStdString().c_str(), defaultW, defaultH);

        CCustomResolutionDlg resDlg(defaultW, defaultH, this);
        if (resDlg.exec() == QDialog::Accepted)
        {
            const int maxRes = 8192;
            m_customResW = min(resDlg.GetWidth(), maxRes);
            m_customResH = min(resDlg.GetHeight(), maxRes);
            const QString resText = QString(customResFormat).arg(m_customResW).arg(m_customResH);
            m_ui->m_resolutionCombo->setItemText(indexOfCustomRes, resText);
            m_ui->m_resolutionCombo->setCurrentIndex(indexOfCustomRes);
        }
        else
        {
            m_ui->m_resolutionCombo->setCurrentIndex(0);
        }
    }

    CheckForEnableUpdateButton();
}

void CSequenceBatchRenderDialog::SaveOutputOptions(const QString& pathname) const
{
    XmlNodeRef batchRenderOptionsNode = XmlHelpers::CreateXmlNode("batchrenderoptions");
    batchRenderOptionsNode->setAttr("version", kBatchRenderFileVersion);

    // Resolution
    XmlNodeRef resolutionNode = batchRenderOptionsNode->newChild("resolution");
    resolutionNode->setAttr("cursel", m_ui->m_resolutionCombo->currentIndex());
    if (m_ui->m_resolutionCombo->currentIndex() == arraysize(resolutions))
    {
        const QString resText = m_ui->m_resolutionCombo->currentText();
        resolutionNode->setContent(resText.toUtf8().data());
    }

    // FPS
    XmlNodeRef fpsNode = batchRenderOptionsNode->newChild("fps");
    fpsNode->setAttr("cursel", m_ui->m_fpsCombo->currentIndex());
    const QString fpsText = m_ui->m_fpsCombo->currentText();
    if (m_ui->m_fpsCombo->currentIndex() == -1 || m_ui->m_fpsCombo->findText(fpsText) == -1)
    {
        fpsNode->setContent(fpsText.toUtf8().data());
    }

    // Capture options (format, buffer, prefix, create_video)
    XmlNodeRef imageNode = batchRenderOptionsNode->newChild("image");
    imageNode->setAttr("format", m_ui->m_imageFormatCombo->currentIndex() % arraysize(imageFormatExtensions));
    const QString prefix = m_ui->BATCH_RENDER_FILE_PREFIX->text();
    imageNode->setAttr("prefix", prefix.toUtf8().data());
    bool disableDebugInfo = m_ui->m_disableDebugInfoCheckBox->isChecked();
    imageNode->setAttr("disabledebuginfo", disableDebugInfo);
    bool bCreateVideoOn = m_ui->m_createVideoCheckBox->isChecked();
    imageNode->setAttr("createvideo", bCreateVideoOn);

    // Custom configs
    XmlNodeRef cvarsNode = batchRenderOptionsNode->newChild("cvars");
    const QStringList lines = m_ui->m_cvarsEdit->toPlainText().split(QStringLiteral("\n"));
    for (const QString& line : lines)
    {
        cvarsNode->newChild("cvar")->setContent(line.toUtf8().data());
    }

    // Destination
    XmlNodeRef destinationNode = batchRenderOptionsNode->newChild("destination");
    const QString destinationText = m_ui->m_destinationEdit->text();
    destinationNode->setContent(destinationText.toUtf8().data());

    batchRenderOptionsNode->saveToFile(pathname.toUtf8().data());
}

bool CSequenceBatchRenderDialog::GetResolutionFromCustomResText(const char* customResText, int& retCustomWidth, int& retCustomHeight) const
{
    // initialize to first resolution preset as default if the sscanf below doesn't scan values successfully
    retCustomWidth = resolutions[0][0];
    retCustomHeight = resolutions[0][1];

    bool    scanSuccess   = false;
    int     scannedWidth  = retCustomWidth;      // initialize with default fall-back values - they'll be overwritten in the case of a succesful sscanf below.
    int     scannedHeight = retCustomHeight;

    scanSuccess = (azsscanf(customResText, "Custom(%d x %d)...", &scannedWidth, &scannedHeight) == 2);
    if (scanSuccess)
    {
        retCustomWidth = scannedWidth;
        retCustomHeight = scannedHeight;
    }
    return scanSuccess;
}

bool CSequenceBatchRenderDialog::LoadOutputOptions(const QString& pathname)
{
    XmlNodeRef batchRenderOptionsNode = XmlHelpers::LoadXmlFromFile(pathname.toStdString().c_str());
    if (batchRenderOptionsNode == nullptr)
    {
        return true;
    }
    int version = 0;
    batchRenderOptionsNode->getAttr("version", version);
    if (version != kBatchRenderFileVersion)
    {
        return false;
    }

    // Resolution
    XmlNodeRef resolutionNode = batchRenderOptionsNode->findChild("resolution");
    if (resolutionNode)
    {
        int curSel = CB_ERR;
        resolutionNode->getAttr("cursel", curSel);
        if (curSel == arraysize(resolutions))
        {
            const QString customResText = resolutionNode->getContent();
            m_ui->m_resolutionCombo->setItemText(curSel, customResText);

            GetResolutionFromCustomResText(customResText.toStdString().c_str(), m_customResW, m_customResH);
        }
        m_ui->m_resolutionCombo->setCurrentIndex(curSel);
    }

    // FPS
    XmlNodeRef fpsNode = batchRenderOptionsNode->findChild("fps");
    if (fpsNode)
    {
        int curSel = -1;
        fpsNode->getAttr("cursel", curSel);
        if (curSel == -1)
        {
            m_ui->m_fpsCombo->setCurrentIndex(-1);
            m_ui->m_fpsCombo->setCurrentText(fpsNode->getContent());
            m_customFPS = QString::fromLatin1(fpsNode->getContent()).toInt();
        }
        else
        {
            m_ui->m_fpsCombo->setCurrentIndex(curSel);
        }
    }

    // Capture options (format, buffer, prefix, create_video)
    XmlNodeRef imageNode = batchRenderOptionsNode->findChild("image");
    if (imageNode)
    {
        int curSel = CB_ERR;
        imageNode->getAttr("format", curSel);
        m_ui->m_imageFormatCombo->setCurrentIndex(curSel);
        curSel = CB_ERR;
        m_ui->BATCH_RENDER_FILE_PREFIX->setText(imageNode->getAttr("prefix"));
        bool disableDebugInfo = false;
        imageNode->getAttr("disabledebuginfo", disableDebugInfo);
        m_ui->m_disableDebugInfoCheckBox->setChecked(disableDebugInfo);
        if (m_bFFMPEGCommandAvailable)
        {
            bool bCreateVideoOn = false;
            imageNode->getAttr("createvideo", bCreateVideoOn);
            m_ui->m_createVideoCheckBox->setChecked(bCreateVideoOn);
        }
    }

    // Custom configs
    XmlNodeRef cvarsNode = batchRenderOptionsNode->findChild("cvars");
    if (cvarsNode)
    {
        QString cvarsText;
        for (int i = 0; i < cvarsNode->getChildCount(); ++i)
        {
            cvarsText += cvarsNode->getChild(i)->getContent();
            if (i < cvarsNode->getChildCount() - 1)
            {
                cvarsText += QStringLiteral("\r\n");
            }
        }
        m_ui->m_cvarsEdit->setPlainText(cvarsText);
    }

    // Destination
    XmlNodeRef destinationNode = batchRenderOptionsNode->findChild("destination");
    if (destinationNode)
    {
        m_ui->m_destinationEdit->setText(destinationNode->getContent());
    }

    return true;
}

void CSequenceBatchRenderDialog::OnStartFrameChange()
{
    if (m_ui->m_startFrame->value() >= m_ui->m_endFrame->value())
    {
        m_ui->m_endFrame->setValue(m_ui->m_startFrame->value() + 1);
    }

    CheckForEnableUpdateButton();
}

void CSequenceBatchRenderDialog::OnEndFrameChange()
{
    if (m_ui->m_startFrame->value() >= m_ui->m_endFrame->value())
    {
        m_ui->m_startFrame->setValue(m_ui->m_endFrame->value() - 1);
    }

    CheckForEnableUpdateButton();
}

void CSequenceBatchRenderDialog::InitializeContext()
{
    m_renderContext.currentItemIndex = 0;
    m_renderContext.spentTime = 0;
    m_renderContext.expectedTotalTime = 0;
    for (size_t i = 0; i < m_renderItems.size(); ++i)
    {
        Range rng = m_renderItems[i].frameRange;
        m_renderContext.expectedTotalTime += rng.end - rng.start;
    }
    m_renderContext.captureOptions.once = false;

    m_ui->BATCH_RENDER_PRESS_ESC_TO_CANCEL->setText(tr("Press ESC to cancel"));
}

void CSequenceBatchRenderDialog::CaptureItemStart()
{
    // Disable most of the UI in group chunks.
    // (Leave the start/cancel button and feedback elements).
    m_ui->BATCH_RENDER_LIST_GROUP_BOX->setEnabled(false);
    m_ui->BATCH_RENDER_INPUT_GROUP_BOX->setEnabled(false);
    m_ui->BATCH_RENDER_OUTPUT_GROUP_BOX->setEnabled(false);

    m_renderContext.canceled = false;

    CV_TrackViewRenderOutputCapturing = 1;

    SRenderItem renderItem = m_renderItems[m_renderContext.currentItemIndex];
    IAnimSequence* nextSequence = renderItem.pSequence;
    // Initialize the next one for the batch rendering.
    // Set the active shot.
    m_renderContext.pActiveDirectorBU = nextSequence->GetActiveDirector();
    nextSequence->SetActiveDirector(renderItem.pDirectorNode);

    // Back up flags and range of the sequence.
    m_renderContext.flagBU = nextSequence->GetFlags();
    m_renderContext.rangeBU = nextSequence->GetTimeRange();

    // Change flags and range of the sequence so that it automatically starts
    // once the game mode kicks in with the specified range.
    nextSequence->SetFlags(m_renderContext.flagBU | IAnimSequence::eSeqFlags_PlayOnReset);

    m_renderContext.captureOptions.timeStep = 1.0f / renderItem.fps;

    Range newRange = renderItem.frameRange;
    newRange.start -= m_renderContext.captureOptions.timeStep;
    renderItem.pSequence->SetTimeRange(newRange);

    // Set up the custom config cvars for this item.
    for (size_t i = 0; i < renderItem.cvars.size(); ++i)
    {
        GetIEditor()->GetSystem()->GetIConsole()->ExecuteString(renderItem.cvars[static_cast<int>(i)].toUtf8().data());
    }

    // Set specific capture options for this item.
    m_renderContext.captureOptions.prefix = renderItem.prefix.toUtf8().data();

    Range rng = nextSequence->GetTimeRange();
    m_renderContext.captureOptions.duration = rng.end - rng.start;
    QString folder = renderItem.folder;
    QString itemText = m_ui->m_renderList->model()->index(m_renderContext.currentItemIndex, 0).data().toString();
    itemText.replace('/', '-'); // A full sequence name can have slash characters which aren't suitable for a file name.
    folder += "/";
    folder += itemText;

    // If this is a relative path, prepend the @products@ folder to match where the Renderer is going
    // to dump the frame buffer image captures.
    if (AzFramework::StringFunc::Path::IsRelative(folder.toUtf8().data()))
    {
        AZStd::string absolutePath;
        AZStd::string assetsRoot = AZ::IO::FileIOBase::GetInstance()->GetAlias("@products@");
        AzFramework::StringFunc::Path::Join(assetsRoot.c_str(), folder.toUtf8().data(), absolutePath);
        folder = absolutePath.c_str();
    }

    QString finalFolder = folder;
    int i = 2;
    while (QFileInfo::exists(finalFolder))
    {
        finalFolder = folder;
        const QString suffix = QString::fromLatin1("_v%1").arg(i);
        finalFolder += suffix;
        ++i;
    }

    // create a new folder before writing any files
    QDir().mkdir(finalFolder);

    m_renderContext.captureOptions.folder = finalFolder.toUtf8().data();

    // Change the resolution.
    const int renderWidth = getResWidth(renderItem.resW);
    const int renderHeight = getResHeight(renderItem.resH);
    ICVar* pCVarCustomResWidth = gEnv->pConsole->GetCVar("r_CustomResWidth");
    ICVar* pCVarCustomResHeight = gEnv->pConsole->GetCVar("r_CustomResHeight");
    if (pCVarCustomResWidth && pCVarCustomResHeight)
    {
        // If available, use the custom resolution cvars.
        m_renderContext.cvarCustomResWidthBU = pCVarCustomResWidth->GetIVal();
        m_renderContext.cvarCustomResHeightBU = pCVarCustomResHeight->GetIVal();
        pCVarCustomResWidth->Set(renderWidth);
        pCVarCustomResHeight->Set(renderHeight);
    }
    else
    {
        // Otherwise, try to adjust the viewport resolution accordingly.
        if (CLayoutViewPane* viewPane = MainWindow::instance()->GetActiveView())
        {
            viewPane->ResizeViewport(renderWidth, renderHeight);
        }
    }

    // turn off debug info if requested
    ICVar* cvarDebugInfo = gEnv->pConsole->GetCVar("r_DisplayInfo");
    if (cvarDebugInfo)
    {
        // cache the current value to restore during OnCaptureItemEnd()
        m_renderContext.cvarDisplayInfoBU = cvarDebugInfo->GetIVal();
        if (renderItem.disableDebugInfo && cvarDebugInfo->GetIVal())
        {
            const int DISPLAY_INFO_OFF = 0;
            cvarDebugInfo->Set(DISPLAY_INFO_OFF);
        }
    }

    // create a new atom pipeline to capture the frames of the current sequence
    m_atomOutputFrameCapture.CreatePipeline(
        *TrackView::SceneFromGameEntityContext(), "TrackViewSequencePipeline", renderItem.resW, renderItem.resH);
    UpdateAtomOutputFrameCaptureView(m_atomOutputFrameCapture, renderItem.resW, renderItem.resH);

    GetIEditor()->GetMovieSystem()->EnableFixedStepForCapture(m_renderContext.captureOptions.timeStep);

    // The capturing doesn't actually start here. It just flags the warming-up and
    // once it's done, then the capturing really begins.
    // The warming-up is necessary to settle down some post-fxs after the resolution change.
    EnterCaptureState(CaptureState::WarmingUpAfterResChange);

    m_renderTimer.start();
}

void CSequenceBatchRenderDialog::OnUpdateWarmingUpAfterResChange()
{
    UpdateSpinnerProgressMessage("Warming up");

    // Spend 30 frames warming up after frame buffer resolution change
    if (m_renderContext.framesSpentInCurrentPhase++ >= 30)
    {
        // We will handle the idle tick manually now because calling Game Update directly.
        SetEnableEditorIdleProcessing(false);

        GetIEditor()->SetInGameMode(true);

        EnterCaptureState(CaptureState::EnteringGameMode);
    }
}

void CSequenceBatchRenderDialog::OnUpdateEnteringGameMode()
{
    UpdateSpinnerProgressMessage("Entering game mode");

    GetIEditor()->GetGameEngine()->Update();

    // Pause the movie player on the first frame
    if (m_renderContext.framesSpentInCurrentPhase++ == 0)
    {
        GetIEditor()->GetMovieSystem()->Pause();
    }
    // Spend 30 frames warming up after changing to game mode.
    else if (m_renderContext.framesSpentInCurrentPhase++ > 30)
    {
        EnterCaptureState(CaptureState::BeginPlayingSequence);
    }
}

void CSequenceBatchRenderDialog::OnUpdateBeginPlayingSequence()
{
    UpdateSpinnerProgressMessage("Begin Playing Sequence");

    SRenderItem renderItem = m_renderItems[m_renderContext.currentItemIndex];

    GetIEditor()->GetMovieSystem()->AddMovieListener(renderItem.pSequence, this);

    GetIEditor()->GetMovieSystem()->Resume();

    // Set the time range for this render, back it up 1 frame so the capture will start
    // exactly on the first frame.
    Range newRange = renderItem.frameRange;
    newRange.start -= m_renderContext.captureOptions.timeStep;
    renderItem.pSequence->SetTimeRange(newRange);

    // Start the sequence playing
    GetIEditor()->GetMovieSystem()->SetPlayingTime(renderItem.pSequence, newRange.start);

    EnterCaptureState(CaptureState::Capturing);
}

void CSequenceBatchRenderDialog::OnUpdateCapturing()
{
    // Make sure we are still in game mode if we are capturing, so we can never
    // get soft locked if we somehow leave game mode without this module knowing about it.
    if (!GetIEditor()->IsInGameMode())
    {
        m_renderContext.endingSequence = m_renderItems[m_renderContext.currentItemIndex].pSequence;
        m_renderContext.canceled = true;
        EnterCaptureState(CaptureState::End);
        return;
    }

    // Progress bar
    IAnimSequence* pCurSeq = m_renderItems[m_renderContext.currentItemIndex].pSequence;
    Range rng = pCurSeq->GetTimeRange();
    float elapsedTime = GetIEditor()->GetMovieSystem()->GetPlayingTime(pCurSeq) - rng.start;
    int percentage
        = int(100.0f * (m_renderContext.spentTime + elapsedTime) / m_renderContext.expectedTotalTime);
    m_ui->m_progressBar->setValue(percentage);

    // Progress message
    const QString itemText = m_ui->m_renderList->model()->index(m_renderContext.currentItemIndex, 0).data().toString();
    const QString msg = tr("Rendering '%1'...(%2%)").arg(itemText).arg(static_cast<int>(100.0f * elapsedTime / (rng.end - rng.start)));
    UpdateSpinnerProgressMessage(msg.toLatin1().data());

    m_renderContext.framesSpentInCurrentPhase++;
}

void CSequenceBatchRenderDialog::OnUpdateEnd(IAnimSequence* sequence)
{
    GetIEditor()->GetMovieSystem()->DisableFixedStepForCapture();

    // Important: End batch render mode BEFORE leaving Game Mode.
    // Otherwise track view will set the active camera based on the directors in the current sequence while leaving game mode
    GetIEditor()->GetMovieSystem()->EnableBatchRenderMode(false);

    GetIEditor()->GetMovieSystem()->RemoveMovieListener(sequence, this);
    GetIEditor()->SetInGameMode(false);
    GetIEditor()->GetGameEngine()->Update();        // Update is needed because SetInGameMode() queues game mode, Update() executes it.

    ICVar* pCVarCustomResWidth = gEnv->pConsole->GetCVar("r_CustomResWidth");
    ICVar* pCVarCustomResHeight = gEnv->pConsole->GetCVar("r_CustomResHeight");
    if (pCVarCustomResWidth && pCVarCustomResHeight)
    {
        // Restore the custom resolution cvars.
        pCVarCustomResWidth->Set(m_renderContext.cvarCustomResWidthBU);
        pCVarCustomResHeight->Set(m_renderContext.cvarCustomResHeightBU);
    }

    // Restore display debug info
    ICVar* cvarDebugInfo = gEnv->pConsole->GetCVar("r_DisplayInfo");
    if (cvarDebugInfo)
    {
        cvarDebugInfo->Set(m_renderContext.cvarDisplayInfoBU);
    }

    // Restore flags, range and the active director of the sequence.
    sequence->SetFlags(m_renderContext.flagBU);
    sequence->SetTimeRange(m_renderContext.rangeBU);
    sequence->SetActiveDirector(m_renderContext.pActiveDirectorBU);

    const auto imageFormat = m_ui->m_imageFormatCombo->currentText();

    SRenderItem renderItem = m_renderItems[m_renderContext.currentItemIndex];
    if (m_bFFMPEGCommandAvailable && renderItem.bCreateVideo)
    {
        // Create a video using the ffmpeg plug-in from captured images.
        m_renderContext.processingFFMPEG = true;

        AZStd::string outputFolder = m_renderContext.captureOptions.folder;
        auto future = QtConcurrent::run(
            [renderItem, outputFolder, imageFormat]
        {
            AZStd::string outputFile;
            AzFramework::StringFunc::Path::Join(outputFolder.c_str(), renderItem.prefix.toUtf8().data(), outputFile);

            QString inputFile = outputFile.c_str();
            outputFile += ".mp4";

            // Use a placeholder for the input file, will expand it with replace.
            QString inputFileDefine = "__input_file__";

            QString command = QStringLiteral("plugin.ffmpeg_encode '%1' '%2' '%3' %4 %5 '-vf crop=%6:%7:0:0'")
                .arg(inputFileDefine).arg(outputFile.c_str()).arg("mpeg4")
                .arg(10240).arg(renderItem.fps).arg(getResWidth(renderItem.resW)).arg(getResHeight(renderItem.resH));

            // Create the input file string, leave the %06d unexpanded for the mpeg tool.
            inputFile += "%06d.";
            inputFile += imageFormat;

            // Replace the input file
            command = command.replace(inputFileDefine, inputFile);

            // Run the command
            GetIEditor()->ExecuteCommand(command);
        });

        // Use a watcher to set a flag when the mpeg processing is complete.
        connect(&m_renderContext.processingFFMPEGWatcher, &QFutureWatcher<void>::finished, this, [this]()
        {
            m_renderContext.processingFFMPEG = false;
        });
        m_renderContext.processingFFMPEGWatcher.setFuture(future);

        EnterCaptureState(CaptureState::FFMPEGProcessing);
    }
    else
    {
        EnterCaptureState(CaptureState::Finalize);
    }
}

void CSequenceBatchRenderDialog::OnUpdateFFMPEGProcessing()
{
    UpdateSpinnerProgressMessage("FFMPEG processing");

    if (!m_renderContext.processingFFMPEG)
    {
        EnterCaptureState(CaptureState::Finalize);
    }
}

void CSequenceBatchRenderDialog::OnUpdateFinalize()
{
    SetEnableEditorIdleProcessing(true);
    m_renderTimer.stop();

    // Turn disabled UI elements back on
    m_ui->BATCH_RENDER_LIST_GROUP_BOX->setEnabled(true);
    m_ui->BATCH_RENDER_INPUT_GROUP_BOX->setEnabled(true);
    m_ui->BATCH_RENDER_OUTPUT_GROUP_BOX->setEnabled(true);

    m_renderContext.frameNumber = 0;
    m_renderContext.capturingFrame = false;

    m_atomOutputFrameCapture.DestroyPipeline(*TrackView::SceneFromGameEntityContext());

    // Check to see if there is more items to process
    bool done = m_renderContext.currentItemIndex == m_renderItems.size() - 1;
    if (done)
    {
        // Update end the batch message
        if (m_renderContext.canceled)
        {
            m_ui->m_progressBar->setValue(0);
            m_ui->m_progressStatusMsg->setText(tr("Rendering canceled"));
        }
        else
        {
            m_ui->m_progressBar->setValue(100);
            m_ui->m_progressStatusMsg->setText(tr("Rendering finished"));
        }

        m_ui->m_pGoBtn->setText(tr("Start"));
        m_ui->m_pGoBtn->setIcon(QPixmap(":/Trackview/clapperboard_ready.png"));
        m_renderContext.currentItemIndex = -1;
        m_ui->BATCH_RENDER_PRESS_ESC_TO_CANCEL->setText(m_ffmpegPluginStatusMsg);

        CV_TrackViewRenderOutputCapturing = 0;

        EnterCaptureState(CaptureState::Idle);
    }
    else
    {
        // Update the context.
        m_renderContext.spentTime += m_renderContext.captureOptions.duration;
        ++m_renderContext.currentItemIndex;

        // Trigger the next item.
        CaptureItemStart();
    }
}

void CSequenceBatchRenderDialog::OnKickIdleTimout()
{
    OnKickIdle();
    if (m_renderContext.IsInRendering())
    {
        m_renderTimer.start();
    }
    else
    {
        // All done with our custom OnKickIdle, restore editor idle.
        SetEnableEditorIdleProcessing(true);
    }

    //When we disable the editor idle processing. system tick is no longer invoked.
    //so we call it here to ensure rendering + other systems are updated.
    if (!m_editorIdleProcessingEnabled)
    {
        AZ::ComponentApplication* componentApplication = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(componentApplication, &AZ::ComponentApplicationRequests::GetApplication);
        if (componentApplication)
        {
            componentApplication->TickSystem();
        }
    }
}

void CSequenceBatchRenderDialog::OnKickIdle()
{
    if (m_renderContext.captureState == CaptureState::WarmingUpAfterResChange)
    {
        OnUpdateWarmingUpAfterResChange();
    }
    else if (m_renderContext.captureState == CaptureState::EnteringGameMode)
    {
        OnUpdateEnteringGameMode();
    }
    else if (m_renderContext.captureState == CaptureState::BeginPlayingSequence)
    {
        OnUpdateBeginPlayingSequence();
    }
    else if (m_renderContext.captureState == CaptureState::Capturing)
    {
        OnUpdateCapturing();
    }
    else if (m_renderContext.captureState == CaptureState::End)
    {
        OnUpdateEnd(m_renderContext.endingSequence);
        m_renderContext.endingSequence = nullptr;
    }
    else if (m_renderContext.captureState == CaptureState::FFMPEGProcessing)
    {
        OnUpdateFFMPEGProcessing();
    }
    else if (m_renderContext.captureState == CaptureState::Finalize)
    {
        OnUpdateFinalize();
    }
    else
    {
        if (!m_renderContext.IsInRendering())
        {
            CheckForEnableUpdateButton();
        }
    }

    if (GetIEditor()->IsInGameMode())
    {
        // note: the internal state may change by calling GetGameEngine()->Update() so
        // we must not cache this value
        const auto capturing = [this]
        {
            return m_renderContext.captureState == CaptureState::Capturing
                // this lags behind by one frame since we are capturing the back buffer,
                // so don't bother enabling the capture on the first frame.
                && m_renderContext.framesSpentInCurrentPhase != 0;
        };

        // if we're currently trying to capture and aren't waiting for a frame
        // capture to complete, it's possible to start the next capture
        const auto canBeginFrameCapture = [capturing, this]
        {
            return capturing() && !m_renderContext.capturingFrame;
        };

        if (canBeginFrameCapture())
        {
            // update the time so the frame number can be calculated in StartCapture()
            IAnimSequence* sequence = m_renderItems[m_renderContext.currentItemIndex].pSequence;
            m_renderContext.captureOptions.time = GetIEditor()->GetMovieSystem()->GetPlayingTime(sequence);

            GetIEditor()->GetMovieSystem()->StartCapture(m_renderContext.captureOptions, ++m_renderContext.frameNumber);
            GetIEditor()->GetMovieSystem()->ControlCapture();
        }

        // if we're not capturing or we're not currently waiting for the current frame to finish
        // being captured, it's safe to move to the next step of the main update
        if (!capturing() || !m_renderContext.capturingFrame)
        {
            const auto& renderItem = m_renderItems[m_renderContext.currentItemIndex];
            // update the view given the current camera transform and projection
            UpdateAtomOutputFrameCaptureView(m_atomOutputFrameCapture, renderItem.resW, renderItem.resH);
            GetIEditor()->GetGameEngine()->Update(); // step update (original frame capture)
        }

        if (canBeginFrameCapture())
        {
            const AZStd::string fileName = AZStd::string::format("Frame_%06d", m_renderContext.frameNumber);

            AZStd::string filePath;
            AzFramework::StringFunc::Path::Join(
                m_renderContext.captureOptions.folder.c_str(), fileName.c_str(), filePath, /*caseInsensitive=*/true,
                /*normalize=*/false);

            // track view callback after each frame is captured
            const auto captureFinishedCallback = [this]() {
                m_renderContext.capturingFrame = false;
                GetIEditor()->GetMovieSystem()->EndCapture();
                GetIEditor()->GetMovieSystem()->ControlCapture();
            };

            const auto imageFormatExtension = m_ui->m_imageFormatCombo->currentText();
            // readback result callback (how the image should be captured)
            // currently only .dds and .ppm
            const auto readbackCallback = [filePath,
                                           imageFormatExtension](const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult) {
                const auto imageFormatExtensionUtf8 = imageFormatExtension.toUtf8();
                const auto imageFormatExtensionCstr = imageFormatExtensionUtf8.constData();

                const AZStd::string fileName = AZStd::string::format("%s.%s", filePath.c_str(), imageFormatExtensionCstr);
                if (AZ::StringFunc::Equal(imageFormatExtensionCstr, "dds"))
                {
                    if (const AZ::Render::FrameCaptureOutputResult result = AZ::Render::DdsFrameCaptureOutput(fileName, readbackResult);
                        result.m_errorMessage.has_value())
                    {
                        AZ_Printf("TrackView", "Dds frame capture failed: %s", result.m_errorMessage.value().c_str());
                    }
                }
                else if (AZ::StringFunc::Equal(imageFormatExtensionCstr, "ppm"))
                {
                    if (const AZ::Render::FrameCaptureOutputResult result = AZ::Render::PpmFrameCaptureOutput(fileName, readbackResult);
                        result.m_errorMessage.has_value())
                    {
                        AZ_Printf("TrackView", "Ppm frame capture failed: %s", result.m_errorMessage.value().c_str());
                    }
                }
                else
                {
                    AZ_Printf("TrackView", "Image format .%s not supported", imageFormatExtensionCstr);
                }
            };

            m_renderContext.capturingFrame = m_atomOutputFrameCapture.BeginCapture(readbackCallback, captureFinishedCallback);
        }
    }
    else
    {
        // Post events, this will cause an Update tick.
        qApp->sendPostedEvents();
    }
}

void CSequenceBatchRenderDialog::OnCancelRender()
{
    if (m_renderContext.captureState == CaptureState::Capturing)
    {
        // In the capturing state, abort the sequence, OnMovieEvent with an abort will fire and cause
        // the transition to CaptureState::End.
        GetIEditor()->GetMovieSystem()->AbortSequence(m_renderItems[m_renderContext.currentItemIndex].pSequence);
    }
    else if (m_renderContext.captureState == CaptureState::EnteringGameMode)
    {
        // In the EnteringGameMode state, the movie sequences hasn't started yet, so we can't count on an
        // OnMovieEvent event to end the capture early. So transition into the End state manually.
        m_renderContext.endingSequence = m_renderItems[m_renderContext.currentItemIndex].pSequence;
        m_renderContext.canceled = true;
        EnterCaptureState(CaptureState::End);
    }
}

void CSequenceBatchRenderDialog::OnLoadBatch()
{
    QString loadPath;
    if (CFileUtil::SelectFile("Render Batch Files (*.batch)",
            Path::GetUserSandboxFolder(), loadPath))
    {
        XmlNodeRef batchRenderListNode = XmlHelpers::LoadXmlFromFile(loadPath.toStdString().c_str());
        if (batchRenderListNode == nullptr)
        {
            return;
        }
        int version = 0;
        batchRenderListNode->getAttr("version", version);
        if (version != kBatchRenderFileVersion)
        {
            QMessageBox::critical(this, tr("Cannot load"), tr("The file version is different!"));
            return;
        }

        OnClearRenderItems();

        for (int i = 0; i < batchRenderListNode->getChildCount(); ++i)
        {
            // Get an item.
            SRenderItem item;
            XmlNodeRef itemNode = batchRenderListNode->getChild(i);

            // sequence
            const QString seqName = itemNode->getAttr("sequence");
            item.pSequence = GetIEditor()->GetMovieSystem()->FindLegacySequenceByName(seqName.toUtf8().data());
            if (item.pSequence == nullptr)
            {
                QMessageBox::warning(this, tr("Sequence not found"), tr("A sequence of '%1' not found! This'll be skipped.").arg(seqName));
                continue;
            }

            // director node
            const QString directorName = itemNode->getAttr("director");
            for (int k = 0; k < item.pSequence->GetNodeCount(); ++k)
            {
                IAnimNode* pNode = item.pSequence->GetNode(k);
                if (pNode->GetType() == AnimNodeType::Director && directorName == pNode->GetName())
                {
                    item.pDirectorNode = pNode;
                    break;
                }
            }
            if (item.pDirectorNode == nullptr)
            {
                QMessageBox::warning(this, tr("Director node not found"), tr("A director node of '%1' not found in the sequence of '%2'! This'll be skipped.").arg(directorName).arg(seqName));
                continue;
            }

            // frame range
            itemNode->getAttr("startframe", item.frameRange.start);
            itemNode->getAttr("endframe", item.frameRange.end);

            // resolution
            itemNode->getAttr("width", item.resW);
            itemNode->getAttr("height", item.resH);

            // fps
            itemNode->getAttr("fps", item.fps);

            // prefix
            item.prefix = itemNode->getAttr("prefix");

            // create_video
            itemNode->getAttr("createvideo", item.bCreateVideo);

            // folder
            item.folder = itemNode->getAttr("folder");

            // cvars
            for (int k = 0; k < itemNode->getChildCount(); ++k)
            {
                const QString cvar = itemNode->getChild(k)->getContent();
                item.cvars.push_back(cvar);
            }

            AddItem(item);
        }
    }
}

void CSequenceBatchRenderDialog::OnSaveBatch()
{
    QString savePath;
    if (CFileUtil::SelectSaveFile("Render Batch Files (*.batch)", "batch",
            Path::GetUserSandboxFolder(), savePath))
    {
        XmlNodeRef batchRenderListNode = XmlHelpers::CreateXmlNode("batchrenderlist");
        batchRenderListNode->setAttr("version", kBatchRenderFileVersion);

        for (size_t i = 0; i < m_renderItems.size(); ++i)
        {
            const SRenderItem& item = m_renderItems[i];
            XmlNodeRef itemNode = batchRenderListNode->newChild("item");

            // sequence
            itemNode->setAttr("sequence", item.pSequence->GetName());

            // director node
            itemNode->setAttr("director", item.pDirectorNode->GetName());

            // frame range
            itemNode->setAttr("startframe", item.frameRange.start);
            itemNode->setAttr("endframe", item.frameRange.end);

            // resolution
            itemNode->setAttr("width", item.resW);
            itemNode->setAttr("height", item.resH);

            // fps
            itemNode->setAttr("fps", item.fps);

            // prefix
            itemNode->setAttr("prefix", item.prefix.toUtf8().data());

            // create_video
            itemNode->setAttr("createvideo", item.bCreateVideo);

            // folder
            itemNode->setAttr("folder", item.folder.toUtf8().data());

            // cvars
            for (size_t k = 0; k < item.cvars.size(); ++k)
            {
                itemNode->newChild("cvar")->setContent(item.cvars[static_cast<int>(k)].toUtf8().data());
            }
        }

        XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), batchRenderListNode, savePath.toStdString().c_str());
    }
}

bool CSequenceBatchRenderDialog::SetUpNewRenderItem(SRenderItem& item)
{
    const QString seqName = m_ui->m_sequenceCombo->currentText();
    const QString shotName = m_ui->m_shotCombo->currentText();
    // folder
    item.folder = m_ui->m_destinationEdit->text();
    if (item.folder.isEmpty())
    {
        QMessageBox::critical(this, tr("Cannot add"), tr("The output folder should be specified!"));
        return false;
    }
    // sequence
    item.pSequence = GetIEditor()->GetMovieSystem()->FindLegacySequenceByName(seqName.toUtf8().data());
    assert(item.pSequence);
    // director
    for (int i = 0; i < item.pSequence->GetNodeCount(); ++i)
    {
        IAnimNode* pNode = item.pSequence->GetNode(i);
        if (pNode->GetType() == AnimNodeType::Director && shotName == pNode->GetName())
        {
            item.pDirectorNode = pNode;
            break;
        }
    }
    if (item.pDirectorNode == nullptr)
    {
        return false;
    }
    // frame range
    item.frameRange = Range(m_ui->m_startFrame->value() / m_fpsForTimeToFrameConversion,
            m_ui->m_endFrame->value() / m_fpsForTimeToFrameConversion);
    // fps
    if (m_ui->m_fpsCombo->currentIndex() == -1 || m_ui->m_fpsCombo->currentText() != fpsOptions[m_ui->m_fpsCombo->currentIndex()].fpsDesc)
    {
        item.fps = m_customFPS;
    }
    else
    {
        item.fps = fpsOptions[m_ui->m_fpsCombo->currentIndex()].fps;
    }
    // prefix
    item.prefix = m_ui->BATCH_RENDER_FILE_PREFIX->text();
    // disable debug info
    item.disableDebugInfo = m_ui->m_disableDebugInfoCheckBox->isChecked();
    // create_video
    item.bCreateVideo = m_ui->m_createVideoCheckBox->isChecked();
    // resolution
    int curResSel = m_ui->m_resolutionCombo->currentIndex();
    if (curResSel < arraysize(resolutions))
    {
        item.resW = resolutions[curResSel][0];
        item.resH = resolutions[curResSel][1];
    }
    else
    {
        item.resW = m_customResW;
        item.resH = m_customResH;
    }
    // cvars
    const int kCVarNameMaxSize = 256;
    char buf[kCVarNameMaxSize];
    buf[kCVarNameMaxSize - 1] = 0;
    const QStringList lines = m_ui->m_cvarsEdit->toPlainText().split('\n');
    for (const QString& line : lines)
    {
        if (!line.isEmpty())
        {
            item.cvars.push_back(line);
        }
    }

    return true;
}

void CSequenceBatchRenderDialog::AddItem(const SRenderItem& item)
{
    // Add the item.
    m_renderItems.push_back(item);

    // Add it to the list box, too.
    m_renderListModel->setStringList(m_renderListModel->stringList() << GetCaptureItemString(item));

    m_ui->m_pGoBtn->setEnabled(true);
}

QString CSequenceBatchRenderDialog::GetCaptureItemString(const SRenderItem& item) const
{
    return QString::fromLatin1("%1_%2_%3-%4(%5x%6,%7)%8").arg(item.pSequence->GetName())
               .arg(item.pDirectorNode->GetName())
               .arg(int(item.frameRange.start * m_fpsForTimeToFrameConversion))
               .arg(int(item.frameRange.end * m_fpsForTimeToFrameConversion))
               .arg(getResWidth(item.resW)).arg(getResHeight(item.resH)).arg(item.fps)
               .arg(item.bCreateVideo ? "[v]" : "");
}

void CSequenceBatchRenderDialog::UpdateSpinnerProgressMessage(const char* description)
{
    static int count = 0;
    const char* rotatingCursor[] = { "|", "/", "-", "\\" };
    const QString msg = tr("%1 %2").arg(description).arg(rotatingCursor[(count++) % arraysize(rotatingCursor)]);
    m_ui->m_progressStatusMsg->setText(msg);
    GetIEditor()->Notify(eNotify_OnIdleUpdate);
}

void CSequenceBatchRenderDialog::EnterCaptureState(CaptureState captureState)
{
    m_renderContext.captureState = captureState;
    m_renderContext.framesSpentInCurrentPhase = 0;
}

void CSequenceBatchRenderDialog::SetEnableEditorIdleProcessing(bool enabled)
{
    if (enabled && !m_editorIdleProcessingEnabled)
    {
        EditorIdleProcessingBus::Broadcast(&EditorIdleProcessing::EnableIdleProcessing);
        m_editorIdleProcessingEnabled = true;
    }

    if (!enabled && m_editorIdleProcessingEnabled)
    {
        EditorIdleProcessingBus::Broadcast(&EditorIdleProcessing::DisableIdleProcessing);
        m_editorIdleProcessingEnabled = false;
    }
}

