/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once


class CUiAnimViewSequence;

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformIncl.h>
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

    virtual bool OnInitDialog();
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
