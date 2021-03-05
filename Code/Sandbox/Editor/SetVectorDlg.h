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

#ifndef CRYINCLUDE_EDITOR_SETVECTORDLG_H
#define CRYINCLUDE_EDITOR_SETVECTORDLG_H

#pragma once
// GotoPositionDlg.h : header file
//

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace Ui
{
    class SetVectorDlg;
}

/////////////////////////////////////////////////////////////////////////////
// CSetVectorDlg dialog

class SANDBOX_API CSetVectorDlg
    : public QDialog
{
    Q_OBJECT
    // Construction
public:
    CSetVectorDlg(QWidget* pParent = NULL);   // standard constructor
    ~CSetVectorDlg();

    static Vec3 GetVectorFromString(const QString& vecString);
    
    // Implementation
protected:
    void OnInitDialog();
    void accept() override;
    void SetVector(const Vec3& v);
    Vec3 GetVectorFromText();
    Vec3 GetVectorFromEditor();
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    Vec3 currentVec;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

private:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QScopedPointer<Ui::SetVectorDlg> m_ui;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif // CRYINCLUDE_EDITOR_SETVECTORDLG_H
