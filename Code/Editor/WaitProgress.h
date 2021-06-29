/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002-2012.
// -------------------------------------------------------------------------
//  File name:   waitprogress.h
//  Created:     10/5/2002 by Timur.
//  Description: CWaitProgress class adds information about lengthy process
//      Usage:
//
//      CWaitProgress wait;
//      wait.SetText("Long");
//      wait.SetProgress(35); // 35 percent.
//
////////////////////////////////////////////////////////////////////////////
#ifndef CRYINCLUDE_EDITOR_WAITPROGRESS_H
#define CRYINCLUDE_EDITOR_WAITPROGRESS_H

class QProgressBar;

#include <QString>

class CWaitProgress
{
public:
    CWaitProgress(const QString& lpszText, bool bStart = true);
    ~CWaitProgress();

    void Start();
    void Stop();
    //! @return true to continue, false to abort lengthy operation.
    bool Step(int nPercentage = -1);
    void SetText(const QString& lpszText);

protected:
    void CreateProgressControl();

    QString m_strText;
    bool m_bStarted;
    bool m_bIgnore;
    int m_percent;
    QProgressBar* m_progressBar;
    static bool m_bInProgressNow;
};
#endif // CRYINCLUDE_EDITOR_WAITPROGRESS_H
