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

#if !defined(Q_MOC_RUN)
#include <QPushButton>
#endif

class PresetButton
    : public QPushButton
{
    Q_OBJECT

public:

    using OnClicked = std::function<void(bool checked)>;

    PresetButton(const QString& defaultIconPath,
        const QString& hoverIconPath,
        const QString& selectedIconPath,
        const QSize& fixedButtonAndIconSize,
        const QString& text,
        OnClicked clicked,
        QWidget* parent = nullptr);

protected:
    void enterEvent(QEvent* ev) override;
    void leaveEvent(QEvent* ev) override;

private:

    void UpdateIcon(bool isChecked);

    bool m_isHovering;
    QIcon m_defaultIcon;
    QIcon m_hoverIcon;
    QIcon m_selectedIconPath;
};
