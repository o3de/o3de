/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_TRACKVIEW_TVSEQUENCEPROPS_H
#define CRYINCLUDE_EDITOR_TRACKVIEW_TVSEQUENCEPROPS_H
#pragma once


class CTrackViewSequence;

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QScopedPointer>
#endif

namespace Ui {
    class CTVSequenceProps;
}

class CTVSequenceProps
    : public QDialog
{
    Q_OBJECT
public:
    CTVSequenceProps(CTrackViewSequence* pSequence, float fps, QWidget* pParent = NULL);    // standard constructor
    ~CTVSequenceProps();

private:
    enum SequenceTimeUnit
    {
        Seconds = 0,
        Frames
    };
    CTrackViewSequence* m_pSequence;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

    void MoveScaleKeys();
    float m_FPS;
    int m_outOfRange;
    SequenceTimeUnit m_timeUnit;

    QScopedPointer<Ui::CTVSequenceProps>    ui;

public slots:
    void OnBnClickedToFrames(bool);
    void OnBnClickedToSeconds(bool);

private slots:
    void ToggleCutsceneOptions(bool);
    void UpdateSequenceProps(const QString& name);
};

#endif // CRYINCLUDE_EDITOR_TRACKVIEW_TVSEQUENCEPROPS_H
