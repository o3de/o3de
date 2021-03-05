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

#ifndef CRYINCLUDE_EDITOR_MATEDITMAINDLG_H
#define CRYINCLUDE_EDITOR_MATEDITMAINDLG_H

#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QString>
#include <QAbstractNativeEventFilter>
#endif

class CMaterialDialog;

class CMatEditMainDlg
    : public QWidget
    , public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit CMatEditMainDlg(const QString& title = QString(), QWidget* parent = nullptr);
    ~CMatEditMainDlg();

    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;

protected:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void OnKickIdle();
    void OnMatEditSend(int param);
    CMaterialDialog* m_materialDialog = nullptr;
};

#endif // CRYINCLUDE_EDITOR_MATEDITMAINDLG_H
