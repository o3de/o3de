/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
