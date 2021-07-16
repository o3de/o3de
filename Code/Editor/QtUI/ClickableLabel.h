/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#ifndef CRYINCLUDE_EDITORCOMMON_CLICKABLELABEL_H
#define CRYINCLUDE_EDITORCOMMON_CLICKABLELABEL_H

#if !defined(Q_MOC_RUN)
#include <QLabel>
#endif

class SANDBOX_API ClickableLabel
    : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel(const QString& text, QWidget* parent = nullptr);
    explicit ClickableLabel(QWidget* parent = nullptr);
    bool event(QEvent* e) override;

    void setText(const QString& text);
    void setShowDecoration(bool b);

protected:
    void showEvent(QShowEvent* event) override;
    void enterEvent(QEvent* ev) override;
    void leaveEvent(QEvent* ev) override;

private:
    void updateFormatting(bool mouseOver);
    QString m_text;
    bool m_showDecoration;
};

#endif
