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

#ifndef CRYINCLUDE_EDITOR_MAINSTATUSBAR_H
#define CRYINCLUDE_EDITOR_MAINSTATUSBAR_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "ISourceControl.h"
#include <QStatusBar>
#include <QWidget>
#include <QIcon>
#endif

class MainStatusBar;
class QLabel;
class QString;
class QPixmap;
class QTimerEvent;
class QMenu;
class QAction;


class StatusBarItem
    : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool clickable MEMBER m_isClickable)

public:
    StatusBarItem(const QString& name, MainStatusBar* parent, bool hasLeadingSpacer = false);
    StatusBarItem(const QString& name, bool isClickable, MainStatusBar* parent, bool hasLeadingSpacer = false);

    void SetText(const QString& text);
    void SetIcon(const QPixmap& icon);
    void SetIcon(const QIcon& icon);
    void SetToolTip(const QString& tip);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* me) override;
    void paintEvent(QPaintEvent* pe) override;

    virtual QString CurrentText() const;
    MainStatusBar* StatusBar() const;

private:
    QIcon m_icon;
    QString m_text;
    bool m_isClickable;
    bool m_hasLeadingSpacer;
};

class MainStatusBar
    : public QStatusBar
    , public IMainStatusBar
{
    Q_OBJECT
public:
    MainStatusBar(QWidget* parent = nullptr);

    //implement IMainStatusBar interface
    void SetStatusText(const QString& text) override;
    QWidget* SetItem(QString indicatorName, QString text, QString tip, int iconId) override;
    QWidget* GetItem(QString indicatorName) override;

    QWidget* SetItem(QString indicatorName, QString text, QString tip, const QPixmap& icon) override;

    void Init();

signals:
    void requestStatusUpdate();
};


#endif // CRYINCLUDE_EDITOR_MAINSTATUSBAR_H
