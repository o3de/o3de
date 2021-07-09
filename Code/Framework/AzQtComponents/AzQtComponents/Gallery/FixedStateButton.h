/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QPushButton>
#include <QStyle>
#endif

class FixedStateButton
    : public QPushButton
{
    Q_OBJECT
public:
    explicit FixedStateButton(QWidget* parent = nullptr);

    void setState(QStyle::State state)
    {
        m_state = state;
    }

protected:
    void paintEvent(QPaintEvent* event) override;

    QStyle::State m_state;
};
