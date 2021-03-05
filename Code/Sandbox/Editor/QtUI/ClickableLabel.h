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
