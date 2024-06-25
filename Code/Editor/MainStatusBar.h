/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_MAINSTATUSBAR_H
#define CRYINCLUDE_EDITOR_MAINSTATUSBAR_H
#pragma once

#if !defined(Q_MOC_RUN)
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
