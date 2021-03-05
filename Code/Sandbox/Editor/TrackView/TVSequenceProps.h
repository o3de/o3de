/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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
