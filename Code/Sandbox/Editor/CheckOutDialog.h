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

//////////////////////////////////////////////////////////////////////////
// CCheckOutDialog dialog
#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace Ui
{
    class CheckOutDialog;
}

class CCheckOutDialog
    : public QDialog
{
    Q_OBJECT

public:
    // Checkout dialog result.
    enum EResult
    {
        CHECKOUT = QDialog::Accepted,
        OVERWRITE,
        CANCEL = QDialog::Rejected
    };

    CCheckOutDialog(const QString& file, QWidget* pParent = NULL);   // standard constructor
    virtual ~CCheckOutDialog();

    // Dialog Data
    void OnInitDialog();

    // Enable functionality For All. In the end call with false to return it in init state.
    // Returns previous enable state
    static bool EnableForAll(bool isEnable);
    static bool IsForAll() { return InstanceIsForAll(); }

    static int LastResult() { return m_lastResult; }

protected:
    void OnBnClickedCancel();
    void OnBnClickedCheckout();
    void OnBnClickedOverwrite();

private:
    static bool& InstanceEnableForAll();
    static bool& InstanceIsForAll();

    void HandleResult(int result);

    QString m_file;
    QScopedPointer<Ui::CheckOutDialog> m_ui;

    static int m_lastResult;
};

//////////////////////////////////////////////////////////////////////////
class CAutoCheckOutDialogEnableForAll
{
public:
    CAutoCheckOutDialogEnableForAll()
    {
        m_bPrevState = CCheckOutDialog::EnableForAll(true);
    }

    ~CAutoCheckOutDialogEnableForAll()
    {
        CCheckOutDialog::EnableForAll(m_bPrevState);
    }

private:
    bool m_bPrevState;
};
