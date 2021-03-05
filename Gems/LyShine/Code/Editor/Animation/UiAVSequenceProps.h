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

#pragma once


class CUiAnimViewSequence;

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QScopedPointer>
#endif

namespace Ui {
    class CUiAVSequenceProps;
}

class CUiAVSequenceProps
    : public QDialog
{
    Q_OBJECT
public:
    CUiAVSequenceProps(CUiAnimViewSequence* pSequence, float fps, QWidget* pParent = NULL);    // standard constructor
    ~CUiAVSequenceProps();

private:
    CUiAnimViewSequence* m_pSequence;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

    void MoveScaleKeys();
    float m_FPS;
    int m_outOfRange;
    int m_timeUnit;

    QScopedPointer<Ui::CUiAVSequenceProps>  ui;

public slots:
    void OnBnClickedTuFrames(bool);
    void OnBnClickedTuSeconds(bool);
};
