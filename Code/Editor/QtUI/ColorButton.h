/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
