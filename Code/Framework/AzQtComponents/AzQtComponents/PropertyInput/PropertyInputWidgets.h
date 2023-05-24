/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/Components/Widgets/SpinBox.h>

#include <QLabel>
#include <QWidget>

namespace AzQtComponents
{
    //! Base Widget to allow double/float property value input edits.
    //! Used as a base class to implement widgets to be used in menus;
    //! They implement the correct sizing and styling to conform with regular menu items.
    class AZ_QT_COMPONENTS_API PropertyInputDoubleWidget
        : public QWidget
    {
    public:
        PropertyInputDoubleWidget();
        ~PropertyInputDoubleWidget();

    protected:
        virtual void OnSpinBoxValueChanged(double newValue) = 0;

        bool event(QEvent* event) override;

        QLabel* m_label = nullptr;
        AzQtComponents::DoubleSpinBox* m_spinBox = nullptr;
    };

} // namespace AzQtComponents
