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

#ifndef CRYINCLUDE_EDITOR_ENVIRONMENTPANEL_H
#define CRYINCLUDE_EDITOR_ENVIRONMENTPANEL_H

#pragma once
// EnvironmentPanel.h : header file
//

#include "Util/Variable.h"

#include <QWidget>
#include <QScopedPointer>

/////////////////////////////////////////////////////////////////////////////
// CEnvironmentPanel dialog
namespace Ui {
    class CEnvironmentPanel;
}

class CEnvironmentPanel
    : public QWidget
{
    // Construction
public:
    CEnvironmentPanel(QWidget* pParent = nullptr);   // standard constructor
    ~CEnvironmentPanel();

    // Implementation
protected:
    CVarBlockPtr m_varBlock;

public:
    void OnBnClickedApply();

private:
    QScopedPointer<Ui::CEnvironmentPanel> ui;

    IVariable::OnSetCallback m_onSetCallback;
};

#endif // CRYINCLUDE_EDITOR_ENVIRONMENTPANEL_H
