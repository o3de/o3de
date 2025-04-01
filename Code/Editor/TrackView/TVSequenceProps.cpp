/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : implementation file


#include "EditorDefs.h"

#include "TVSequenceProps.h"

// Qt
#include <QMessageBox>

// Editor
#include "TrackViewSequence.h"
#include "TrackViewSequenceManager.h"
#include "AnimationContext.h"



AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <TrackView/ui_TVSequenceProps.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

CTVSequenceProps::CTVSequenceProps(CTrackViewSequence* pSequence, float fps, QWidget* pParent)
    : QDialog(pParent)
    , m_FPS(fps)
    , m_outOfRange(0)
    , m_timeUnit(Seconds)
    , ui(new Ui::CTVSequenceProps)
{
    ui->setupUi(this);

    AZ_Assert(pSequence, "pSequence is null");
    m_pSequence = pSequence;
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CTVSequenceProps::OnOK);
    connect(ui->CUT_SCENE, &QCheckBox::toggled, this, &CTVSequenceProps::ToggleCutsceneOptions);
    connect(ui->TO_SECONDS, &QRadioButton::toggled, this, &CTVSequenceProps::OnBnClickedToSeconds);
    connect(ui->TO_FRAMES, &QRadioButton::toggled, this, &CTVSequenceProps::OnBnClickedToFrames);

    OnInitDialog();
}

CTVSequenceProps::~CTVSequenceProps()
{
}

// CTVSequenceProps message handlers
bool CTVSequenceProps::OnInitDialog()
{
    ui->NAME->setText(m_pSequence->GetName().c_str());
    int seqFlags = m_pSequence->GetFlags();

    ui->ALWAYS_PLAY->setChecked((seqFlags & IAnimSequence::eSeqFlags_PlayOnReset));
    ui->CUT_SCENE->setChecked((seqFlags & IAnimSequence::eSeqFlags_CutScene));
    ui->DISABLEPLAYER->setChecked((seqFlags & IAnimSequence::eSeqFlags_NoPlayer));
    ui->DISABLESOUNDS->setChecked((seqFlags & IAnimSequence::eSeqFlags_NoGameSounds));
    ui->NOSEEK->setChecked((seqFlags & IAnimSequence::eSeqFlags_NoSeek));
    ui->NOABORT->setChecked((seqFlags & IAnimSequence::eSeqFlags_NoAbort));
    ui->EARLYMOVIEUPDATE->setChecked((seqFlags & IAnimSequence::eSeqFlags_EarlyMovieUpdate));

    ToggleCutsceneOptions(ui->CUT_SCENE->isChecked());

    ui->MOVE_SCALE_KEYS->setChecked(BST_UNCHECKED);

    ui->START_TIME->setRange(0.0, (1e+5));
    ui->END_TIME->setRange(0.0, (1e+5));

    Range timeRange = m_pSequence->GetTimeRange();
    float invFPS = 1.0f / m_FPS;

    ui->START_TIME->setValue(timeRange.start);
    ui->START_TIME->setSingleStep(invFPS);
    ui->END_TIME->setValue(timeRange.end);
    ui->END_TIME->setSingleStep(invFPS);

    if (m_pSequence->GetFlags() & IAnimSequence::eSeqFlags_DisplayAsFramesOrSeconds)
    {
        m_timeUnit = Frames;
        ui->TO_FRAMES->setChecked(true);
    }
    else
    {
        m_timeUnit = Seconds;
        ui->TO_SECONDS->setChecked(true);
    }

    m_outOfRange = 0;
    if (m_pSequence->GetFlags() & IAnimSequence::eSeqFlags_OutOfRangeConstant)
    {
        m_outOfRange = 1;
        ui->ORT_CONSTANT->setChecked(true);
    }
    else if (m_pSequence->GetFlags() & IAnimSequence::eSeqFlags_OutOfRangeLoop)
    {
        m_outOfRange = 2;
        ui->ORT_LOOP->setChecked(true);
    }
    else
    {
        ui->ORT_ONCE->setChecked(true);
    }

    return true;  // return true unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CTVSequenceProps::MoveScaleKeys()
{
    // Move/Rescale the sequence to a new time range.
    Range timeRangeOld = m_pSequence->GetTimeRange();
    Range timeRangeNew;
    timeRangeNew.start = static_cast<float>(ui->START_TIME->value());
    timeRangeNew.end = static_cast<float>(ui->END_TIME->value());

    if (!(timeRangeNew == timeRangeOld))
    {
        m_pSequence->AdjustKeysToTimeRange(timeRangeNew);
    }
}

void CTVSequenceProps::UpdateSequenceProps(const QString& name)
{
    if (ui->MOVE_SCALE_KEYS->isChecked())
    {
        MoveScaleKeys();
    }

    Range timeRange;
    timeRange.start = static_cast<float>(ui->START_TIME->value());
    timeRange.end = static_cast<float>(ui->END_TIME->value());

    if (m_timeUnit == Frames)
    {
        float invFPS = 1.0f / m_FPS;
        timeRange.start = static_cast<float>(ui->START_TIME->value()) * invFPS;
        timeRange.end = static_cast<float>(ui->END_TIME->value()) * invFPS;
    }

    m_pSequence->SetTimeRange(timeRange);

    CAnimationContext* ac = GetIEditor()->GetAnimation();
    if (ac)
    {
        ac->UpdateTimeRange();
    }

    QString seqName = QString::fromUtf8(m_pSequence->GetName().c_str());
    if (name != seqName)
    {
        // Rename sequence.
        const CTrackViewSequenceManager* sequenceManager = GetIEditor()->GetSequenceManager();

        sequenceManager->RenameNode(m_pSequence, name.toUtf8().data());
    }

    int seqFlags = m_pSequence->GetFlags();
    seqFlags &= ~(IAnimSequence::eSeqFlags_OutOfRangeConstant | IAnimSequence::eSeqFlags_OutOfRangeLoop);

    if (ui->ALWAYS_PLAY->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_PlayOnReset;
    }
    else
    {
        seqFlags &= ~IAnimSequence::eSeqFlags_PlayOnReset;
    }

    if (ui->CUT_SCENE->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_CutScene;
    }
    else
    {
        seqFlags &= ~IAnimSequence::eSeqFlags_CutScene;
    }

    if (ui->DISABLEPLAYER->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_NoPlayer;
    }
    else
    {
        seqFlags &= (~IAnimSequence::eSeqFlags_NoPlayer);
    }

    if (ui->ORT_CONSTANT->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_OutOfRangeConstant;
    }
    else if (ui->ORT_LOOP->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_OutOfRangeLoop;
    }

    if (ui->DISABLESOUNDS->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_NoGameSounds;
    }
    else
    {
        seqFlags &= (~IAnimSequence::eSeqFlags_NoGameSounds);
    }

    if (ui->NOSEEK->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_NoSeek;
    }
    else
    {
        seqFlags &= (~IAnimSequence::eSeqFlags_NoSeek);
    }

    if (ui->NOABORT->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_NoAbort;
    }
    else
    {
        seqFlags &= (~IAnimSequence::eSeqFlags_NoAbort);
    }

    if (ui->EARLYMOVIEUPDATE->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_EarlyMovieUpdate;
    }
    else
    {
        seqFlags &= (~IAnimSequence::eSeqFlags_EarlyMovieUpdate);
    }

    if (ui->TO_FRAMES->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_DisplayAsFramesOrSeconds;
    }
    else
    {
        seqFlags &= (~IAnimSequence::eSeqFlags_DisplayAsFramesOrSeconds);
    }

    if (static_cast<IAnimSequence::EAnimSequenceFlags>(seqFlags) != m_pSequence->GetFlags())
    {
        AzToolsFramework::ScopedUndoBatch undoBatch("Change TrackView Sequence Flags");
        m_pSequence->SetFlags(static_cast<IAnimSequence::EAnimSequenceFlags>(seqFlags));
        undoBatch.MarkEntityDirty(m_pSequence->GetSequenceComponentEntityId());
    }
}

void CTVSequenceProps::OnOK()
{
    QString name = ui->NAME->text();
    if (name.isEmpty())
    {
        QMessageBox::warning(this, "Sequence Properties", "A sequence name cannot be empty!");
        return;
    }

    if (name.contains('/'))
    {
        QMessageBox::warning(this, "Sequence Properties", "A sequence name cannot contain a '/' character!");
        return;
    }

    if (m_pSequence != nullptr)
    {
        AzToolsFramework::ScopedUndoBatch undoBatch("Change TrackView Sequence Settings");
        UpdateSequenceProps(name);
        undoBatch.MarkEntityDirty(m_pSequence->GetSequenceComponentEntityId());
    }

    accept();
}

void CTVSequenceProps::ToggleCutsceneOptions(bool bActivated)
{
    if (bActivated == false)
    {
        ui->NOABORT->setChecked(false);
        ui->DISABLEPLAYER->setChecked(false);
        ui->DISABLESOUNDS->setChecked(false);
    }

    ui->NOABORT->setEnabled(bActivated);
    ui->DISABLEPLAYER->setEnabled(bActivated);
    ui->DISABLESOUNDS->setEnabled(bActivated);
}

void CTVSequenceProps::OnBnClickedToFrames(bool v)
{
    if (!v)
    {
        return;
    }

    ui->START_TIME->setSingleStep(1.0f);
    ui->END_TIME->setSingleStep(1.0f);

    ui->START_TIME->setValue(AZStd::round(ui->START_TIME->value() * static_cast<double>(m_FPS)));
    ui->END_TIME->setValue(AZStd::round(ui->END_TIME->value() * static_cast<double>(m_FPS)));

    m_timeUnit = Frames;
}


void CTVSequenceProps::OnBnClickedToSeconds(bool v)
{
    if (!v)
    {
        return;
    }

    float fInvFPS = 1.0f / m_FPS;

    ui->START_TIME->setSingleStep(fInvFPS);
    ui->END_TIME->setSingleStep(fInvFPS);

    ui->START_TIME->setValue(ui->START_TIME->value() * fInvFPS);
    ui->END_TIME->setValue(ui->END_TIME->value() * fInvFPS);

    m_timeUnit = Seconds;
}

#include <TrackView/moc_TVSequenceProps.cpp>
