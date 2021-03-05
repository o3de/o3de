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
