/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : A dialog for batch-rendering sequences

#pragma once

#include "AtomOutputFrameCapture.h"

#include <AzFramework/StringFunc/StringFunc.h>

#include <QDialog>
#include <QTimer>
#include <QFutureWatcher>
#include <QValidator>

class QStringListModel;

namespace Ui
{
    class SequenceBatchRenderDialog;
}

class CSequenceBatchRenderDialog
    : public QDialog
    , public IMovieListener
{
public:
    CSequenceBatchRenderDialog(float fps, QWidget* pParent = nullptr);
    virtual ~CSequenceBatchRenderDialog();

    void reject() override; // overriding so Qt doesn't cancel

protected:
    void OnInitDialog();

    void OnAddRenderItem();
    void OnRemoveRenderItem();
    void OnClearRenderItems();
    void OnUpdateRenderItem();
    void OnLoadPreset();
    void OnSavePreset();
    void OnGo();
    void OnDone();
    void OnSequenceSelected();
    void OnRenderItemSelChange();
    void OnFPSEditChange();
    void OnFPSChange(int itemIndex);
    void OnImageFormatChange();
    void OnResolutionSelected();
    void OnStartFrameChange();
    void OnEndFrameChange();
    void OnLoadBatch();
    void OnSaveBatch();
    void OnKickIdle();
    void OnCancelRender();

    void SaveOutputOptions(const QString& pathname) const;
    bool LoadOutputOptions(const QString& pathname);

    QString m_ffmpegPluginStatusMsg;
    bool m_bFFMPEGCommandAvailable;

    float m_fpsForTimeToFrameConversion;    // FPS setting in TrackView
    struct SRenderItem
    {
        IAnimSequence* pSequence;
        IAnimNode* pDirectorNode;
        Range frameRange;
        int resW, resH;
        int fps;
        QString folder;
        QString prefix;
        QStringList cvars;
        bool disableDebugInfo;
        bool bCreateVideo;
        SRenderItem()
            : pSequence(nullptr)
            , pDirectorNode(nullptr)
            , disableDebugInfo(false)
            , bCreateVideo(false) {}
        bool operator==(const SRenderItem& item)
        {
            if (pSequence == item.pSequence
                && pDirectorNode == item.pDirectorNode
                && frameRange == item.frameRange
                && resW == item.resW && resH == item.resH
                && fps == item.fps
                && folder == item.folder
                && prefix == item.prefix
                && cvars == item.cvars
                && disableDebugInfo == item.disableDebugInfo
                && bCreateVideo == item.bCreateVideo)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    };
    std::vector<SRenderItem> m_renderItems;

    // Capture States
    enum class CaptureState
    {
        Idle,
        WarmingUpAfterResChange,
        EnteringGameMode,
        BeginPlayingSequence,
        Capturing,
        End,
        FFMPEGProcessing,
        Finalize
    };

    struct SRenderContext
    {
        int currentItemIndex;
        float expectedTotalTime;
        float spentTime;
        int flagBU;
        Range rangeBU;
        int cvarCustomResWidthBU, cvarCustomResHeightBU;
        int cvarDisplayInfoBU;
        int  framesSpentInCurrentPhase;
        IAnimNode* pActiveDirectorBU;
        ICaptureKey captureOptions;
        bool processingFFMPEG;
        // Signals when an mpeg is finished being processed.
        QFutureWatcher<void> processingFFMPEGWatcher;
        // True if the user canceled a render.
        bool canceled;
        // The sequence that triggered the CaptureState::Ending.
        IAnimSequence* endingSequence;
        // Current capture state.
        CaptureState captureState;
        // Is an individual frame currently being captured.
        bool capturingFrame;
        // Current frame being captured
        int frameNumber;

        bool IsInRendering() const
        { return currentItemIndex >= 0; }

        SRenderContext()
            : currentItemIndex(-1)
            , expectedTotalTime(0)
            , spentTime(0)
            , flagBU(0)
            , pActiveDirectorBU(nullptr)
            , cvarCustomResWidthBU(0)
            , cvarCustomResHeightBU(0)
            , cvarDisplayInfoBU(0)
            , framesSpentInCurrentPhase(0)
            , processingFFMPEG(false)
            , canceled(false)
            , endingSequence(nullptr)
            , captureState(CaptureState::Idle)
            , capturingFrame(false)
            , frameNumber(0) {}
    };
    SRenderContext m_renderContext;

    // Custom validator to make sure the prefix is a valid part of a filename.
    class CPrefixValidator : public QValidator
    {
    public:
        CPrefixValidator(QObject* parent) : QValidator(parent) {}

        QValidator::State validate(QString& input, [[maybe_unused]] int& pos) const override
        {
            bool valid = input.isEmpty() || AzFramework::StringFunc::Path::IsValid(input.toUtf8().data());
            return valid ? QValidator::State::Acceptable : QValidator::State::Invalid;
        }
    };

    // Custom values from resolution/FPS combo boxes
    int m_customResW, m_customResH;
    int m_customFPS;

    void InitializeContext();
    void OnMovieEvent(IMovieListener::EMovieEvent event, IAnimSequence* pSequence) override;

    void CaptureItemStart();

    // Capture State Updates
    void OnUpdateWarmingUpAfterResChange();
    void OnUpdateEnteringGameMode();
    void OnUpdateBeginPlayingSequence();
    void OnUpdateCapturing();
    void OnUpdateEnd(IAnimSequence* pSequence);
    void OnUpdateFFMPEGProcessing();
    void OnUpdateFinalize();

    bool SetUpNewRenderItem(SRenderItem& item);
    void AddItem(const SRenderItem& item);
    QString GetCaptureItemString(const SRenderItem& item) const;

protected slots:
    void OnKickIdleTimout();

    bool GetResolutionFromCustomResText(const char* customResText, int& retCustomWidth, int& retCustomHeight) const;

private:
    void CheckForEnableUpdateButton();
    void stashActiveViewportResolution();
    void UpdateSpinnerProgressMessage(const char* description);
    void EnterCaptureState(CaptureState captureState);
    void SetEnableEditorIdleProcessing(bool enabled);

    QScopedPointer<Ui::SequenceBatchRenderDialog> m_ui;
    QStringListModel* m_renderListModel;
    QTimer m_renderTimer;
    bool m_editorIdleProcessingEnabled;
    int32 CV_TrackViewRenderOutputCapturing;
    QScopedPointer<CPrefixValidator> m_prefixValidator;

    TrackView::AtomOutputFrameCapture m_atomOutputFrameCapture;
};
