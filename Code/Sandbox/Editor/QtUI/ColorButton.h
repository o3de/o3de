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
#ifndef CRYINCLUDE_EDITORCOMMON_COLORBUTTON_H
#define CRYINCLUDE_EDITORCOMMON_COLORBUTTON_H

#include <Include/EditorCoreAPI.h>

#if !defined(Q_MOC_RUN)
#include <QToolButton>
#endif

class QColor;
class QPaintEvent;

class EDITOR_CORE_API ColorButton
    : public QToolButton
{
    Q_OBJECT

    Q_PROPERTY(QColor Color READ Color WRITE SetColor)

public:
    ColorButton(QWidget* parent = nullptr);
    virtual ~ColorButton() {};

    QColor Color() const { return m_color; }
    void SetColor(const QColor& color)
    {
        m_color = color;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor m_color;

signals:
    void ColorChanged(const QColor& color);

private slots:
    void OnClick();
};

#endif // CRYINCLUDE_EDITORCOMMON_COLORBUTTON_H
