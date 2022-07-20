/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QWidget>
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace EMStudio
{
    //! Widget is shown in the inspector window in case no object is selected.
    class NoSelectionWidget
        : public QWidget
    {
        Q_OBJECT //AUTOMOC

    public:
        explicit NoSelectionWidget(QWidget* parent = nullptr);
        ~NoSelectionWidget() override = default;

    private:
        QLabel* m_label = nullptr;
    };
} // namespace EMStudio
