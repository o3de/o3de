/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QWidget>
#endif

class QLineEdit;
class QCheckBox;

namespace AzQtComponents
{
    class DoubleSpinBox;
    class AZ_QT_COMPONENTS_API GammaEdit
        : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
        Q_PROPERTY(qreal gamma READ gamma WRITE setGamma NOTIFY gammaChanged)

    public:
        explicit GammaEdit(QWidget* parent = nullptr);
        ~GammaEdit() override;

        bool isChecked() const;
        qreal gamma() const;

    Q_SIGNALS:
        void toggled(bool enabled);
        void gammaChanged(qreal gamma);

    public Q_SLOTS:
        void setChecked(bool enabled);
        void setGamma(qreal gamma);

    private Q_SLOTS:
        void valueChanged(double gamma);

    private:
        qreal m_gamma;
        DoubleSpinBox* m_edit;
        QCheckBox* m_toggleSwitch;
    };
} // namespace AzQtComponents
