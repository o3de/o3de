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
#ifndef CRYINCLUDE_EDITORCOMMON_QPARENTWNDWIDGET_H
#define CRYINCLUDE_EDITORCOMMON_QPARENTWNDWIDGET_H

#if !defined(Q_MOC_RUN)
#include <QWidget>

#include "EditorCommonAPI.h"
#endif

// QParentWndWidget can be used to show Qt popup windows/dialogs on top on
// Win32/MFC windows.
//
// Example of usage:
//   QParentWndWidget parent(parentHwnd);
//
//   QDialog dialog(parent);
//   dialog.exec(...);
class EDITOR_COMMON_API QParentWndWidget
    : public QWidget
{
    Q_OBJECT
public:
    QParentWndWidget(HWND parent);

    void show();
    void hide();
    void center();

    HWND parentWindow() const { return m_parent; }

protected:
    void childEvent(QChildEvent* e) override;
    void focusInEvent(QFocusEvent* ev) override;
    bool focusNextPrevChild(bool next) override;

    bool eventFilter(QObject* o, QEvent* e) override;
#if QT_VERSION >= 0x50000
    bool nativeEvent(const QByteArray&, void* message, long*);
#else
    bool winEvent(MSG* msg, long*);
#endif

private:
    HWND m_parent;
    HWND m_parentToCenterOn;
    HWND m_modalityRoot;

    HWND m_previousFocus;

    bool m_parentWasDisabled;
};

#endif // CRYINCLUDE_EDITORCOMMON_QPARENTWNDWIDGET_H
