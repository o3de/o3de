/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QDoubleSpinBox>
#include <QSlider>
#include <QPoint>
#endif

class QIntValidator;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API StyledSliderPrivate
        : public QSlider
    {
        Q_OBJECT

    public:
        explicit StyledSliderPrivate(QWidget* parent = nullptr);
    };

    class AZ_QT_COMPONENTS_API StyledDoubleSpinBox
        : public QDoubleSpinBox
    {
        Q_OBJECT
    public:
        explicit StyledDoubleSpinBox(QWidget* parent = nullptr);
        ~StyledDoubleSpinBox();
        void displaySlider();
        void hideSlider();
        void handleClickOnApp(const QPoint& pos);
        bool isMouseOnSlider();
        void SetCustomSliderRange(double min, double max);

        void SetDisplayDecimals(int precision);
        QString textFromValue(double value) const override;

    protected:
        void showEvent(QShowEvent* ev) override;
        void resizeEvent(QResizeEvent* ev) override;
        void focusInEvent(QFocusEvent* event) override;
        double GetSliderMinimum();
        double GetSliderRange();

        bool m_restrictToInt;
        double m_customSliderMinValue;
        double m_customSliderMaxValue;
        bool m_hasCustomSliderRange;

    private Q_SLOTS:
        void UpdateToolTip(double value);

    private:
        void initSlider();
        void prepareSlider();
        bool isClickOnSpinBox(const QPoint& globalPos);
        bool isClickOnSlider(const QPoint& globalPos);
        void updateSliderValue(double newVal);
        void updateValueFromSlider(int newVal);
        int ConvertToSliderValue(double spinBoxValue);
        double ConvertFromSliderValue(int sliderValue);
        QString StringValue(double value, bool truncated = false) const;
        bool SliderEnabled() const;

        bool m_justPassFocusSlider = false;
        StyledSliderPrivate* m_slider;
        bool m_ignoreNextUpdateFromSlider;
        bool m_ignoreNextUpdateFromSpinBox;
        int m_displayDecimals;
    };

    class AZ_QT_COMPONENTS_API StyledSpinBox
        : public StyledDoubleSpinBox
    {
        Q_OBJECT
    public:
        explicit StyledSpinBox(QWidget* parent = nullptr);

        // Integer helper functions
        int maximum() const;
        int minimum() const;
        void setMaximum(int max);
        void setMinimum(int min);
        void setRange(int min, int max);
        void setSingleStep(int val);
        int singleStep() const;
        int value() const;

    public Q_SLOTS:
        void setValue(int val);

    Q_SIGNALS:
        void valueChanged(int val);

    private:
        QIntValidator* m_validator;
    };
} // namespace AzQtComponents

