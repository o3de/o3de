/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_STRINGDLG_H
#define CRYINCLUDE_EDITOR_STRINGDLG_H

#pragma once

#include <QInputDialog>
#include <QMessageBox>
#include <SandboxAPI.h>

/////////////////////////////////////////////////////////////////////////////
// StringDlg Qt dialog

// return false if error input
typedef bool (StringDlgPredicate)(QString input);

class StringDlg : public QInputDialog
{
public:
    StringDlg(const QString &title, QWidget* pParent = nullptr, bool bFileNameLimitation = false);

    void SetCheckCallback(const std::function<StringDlgPredicate>& Check) {
        m_Check = Check;
    }
    void SetString(const QString &str) {
        setTextValue(str);
    }
    QString GetString() {
        return textValue();
    }

protected:
    virtual void accept();
    bool m_bFileNameLimitation;
    std::function<StringDlgPredicate> m_Check;
};


/////////////////////////////////////////////////////////////////////////////
// StringGroupDlg Qt dialog
class SANDBOX_API StringGroupDlg : public QDialog
{
    // Construction
public:
    StringGroupDlg(const QString &title = QString(), QWidget *parent = 0);

    void SetString(const QString &str);
    QString GetString() const;

    void SetGroup(const QString &str);
    QString GetGroup() const;

protected:
    QLineEdit *m_string;
    QLineEdit *m_group;
};


#endif // CRYINCLUDE_EDITOR_STRINGDLG_H
