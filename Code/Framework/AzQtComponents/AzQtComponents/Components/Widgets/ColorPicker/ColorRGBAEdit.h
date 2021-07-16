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

class QGridLayout;
class QLabel;

namespace AzQtComponents
{
    class DoubleSpinBox;

    class AZ_QT_COMPONENTS_API ColorRGBAEdit
        : public QWidget
    {
        Q_OBJECT

        Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
        Q_PROPERTY(qreal red READ red WRITE setRed NOTIFY redChanged)
        Q_PROPERTY(qreal green READ green WRITE setGreen NOTIFY greenChanged)
        Q_PROPERTY(qreal blue READ blue WRITE setBlue NOTIFY blueChanged)
        Q_PROPERTY(qreal alpha READ alpha WRITE setAlpha NOTIFY alphaChanged)
        Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly NOTIFY readOnlyChanged)

    public:
        enum class Mode
        {
            Rgba,
            Rgb
        };
        Q_ENUM(Mode)

        explicit ColorRGBAEdit(QWidget* parent = nullptr);
        ~ColorRGBAEdit() override;

        Mode mode() const;
        bool isReadOnly() const;

        qreal red() const;
        qreal green() const;
        qreal blue() const;
        qreal alpha() const;

        void setHorizontalSpacing(int spacing);

    Q_SIGNALS:
        void modeChanged(Mode mode);
        void readOnlyChanged(bool readOnly);
        void redChanged(qreal red);
        void greenChanged(qreal green);
        void blueChanged(qreal blue);
        void alphaChanged(qreal alpha);
        void valueChangeBegan();
        void valueChangeEnded();

    public Q_SLOTS:
        void setMode(Mode mode);
        void setReadOnly(bool readOnly);
        void setRed(qreal red);
        void setGreen(qreal green);
        void setBlue(qreal blue);
        void setAlpha(qreal alpha);

    private Q_SLOTS:
        void redValueChanged(qreal value);
        void greenValueChanged(qreal value);
        void blueValueChanged(qreal value);
        void alphaValueChanged(qreal value);

    private:
        DoubleSpinBox* createComponentSpinBox();

        Mode m_mode;
        bool m_readOnly;
        qreal m_red;
        qreal m_green;
        qreal m_blue;
        qreal m_alpha;

        QGridLayout* m_layout;
        DoubleSpinBox* m_redSpin;
        DoubleSpinBox* m_greenSpin;
        DoubleSpinBox* m_blueSpin;
        DoubleSpinBox* m_alphaSpin;
        QLabel* m_alphaLabel;
    };
} // namespace AzQtComponents
