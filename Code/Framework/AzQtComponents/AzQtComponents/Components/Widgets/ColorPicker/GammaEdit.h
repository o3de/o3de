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
