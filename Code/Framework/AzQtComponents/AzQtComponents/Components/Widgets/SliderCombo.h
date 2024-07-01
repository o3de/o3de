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
#include <AzQtComponents/Components/Widgets/Slider.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <QWidget>
#endif

namespace AzQtComponents
{
    //! Control that combines a SpinBox and a Slider for integer value input.
    //! This control can be set to have soft and hard minimum and maximum values.
    //! If they differ, the soft minimum and maximum values define the range that is
    //! selectable via the slider, while the hard minimum and maximum are the actual
    //! limits in the range of selectable values. This can be used to implement a
    //! recommended range of values easily selectable via the slider, and also support
    //! the actual value range for a property via manual input.
    class AZ_QT_COMPONENTS_API SliderCombo
        : public QWidget
    {
        Q_OBJECT

        //! Hard minimum value. Sets the minimum value on the SpinBox.
        Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
        //! Hard maximum value. Sets the maximum value on the SpinBox.
        Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
        //! Soft minimum value. Sets the minimum value on the Slider.
        Q_PROPERTY(int softMinimum READ softMinimum WRITE setSoftMinimum)
        //! Soft maximum value. Sets the maximum value on the Slider.
        Q_PROPERTY(int softMaximum READ softMaximum WRITE setSoftMaximum)
        //! Current value.
        Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    public:
        using value_type = int;

        explicit SliderCombo(QWidget *parent = nullptr);
        ~SliderCombo();

        //! Sets the current value.
        void setValue(int value);
        //! Gets the current value.
        Q_REQUIRED_RESULT int value() const;

        //! Sets the hard minimum value selectable via the spinbox.
        void setMinimum(int min);
        //! Gets the hard minimum value selectable via the spinbox.
        Q_REQUIRED_RESULT int minimum() const;

        //! Sets the hard maximum value selectable via the spinbox.
        void setMaximum(int max);
        //! Gets the hard maximum value selectable via the spinbox.
        Q_REQUIRED_RESULT int maximum() const;

        //! Sets the hard minimum and maximum values selectable via the spinbox.
        void setRange(int min, int max);

        //! Sets the soft minimum value selectable via the slider.
        void setSoftMinimum(int min);
        //! Gets the soft minimum value selectable via the slider.
        Q_REQUIRED_RESULT int softMinimum() const;

        //! Sets the soft maximum value selectable via the slider.
        void setSoftMaximum(int max);
        //! Gets the soft maximum value selectable via the slider.
        Q_REQUIRED_RESULT int softMaximum() const;

        //! Gets true if a soft minimum is set for this control.
        Q_REQUIRED_RESULT bool hasSoftMinimum() const { return m_useSoftMinimum; }
        //! Gets true if a soft maximum is set for this control.
        Q_REQUIRED_RESULT bool hasSoftMaximum() const { return m_useSoftMaximum; }

        //! Sets the soft minimum and maximum values selectable via the slider.
        void setSoftRange(int min, int max);

        //! Returns the underlying spinbox.
        SpinBox* spinbox() const;
        //! Returns the underlying slider.
        SliderInt* slider() const;

    Q_SIGNALS:
        //! Triggered when this control's value has been changed.
        void valueChanged();
        //! Triggered when this control's value has stopped being changed.
        //! Only triggers once at the end of a drag operation.
        void editingFinished();

    protected:
        void focusInEvent(QFocusEvent* event) override;

    private Q_SLOTS:
        void updateSpinBox();
        void updateSlider();

    private:
        void refreshUi();

        SliderInt* m_slider = nullptr;
        SpinBox* m_spinbox = nullptr;
        int m_sliderMin = 0;
        int m_sliderMax = 100;
        bool m_useSoftMinimum = false;
        bool m_useSoftMaximum = false;
        int m_minimum = 0;
        int m_maximum = 100;
        int m_softMinimum = 0;
        int m_softMaximum = 100;
        int m_value = 0;
    };

    /**
     * This class provide a QWidget which contains a spinbox and a slider
     */
    class AZ_QT_COMPONENTS_API SliderDoubleCombo
        : public QWidget
    {
        Q_OBJECT

        //! Hard minimum value. Sets the minimum value on the SpinBox.
        Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
        //! Hard maximum value. Sets the maximum value on the SpinBox.
        Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
        //! The number of steps used when changing the value via the arrow keys.
        Q_PROPERTY(int numSteps READ numSteps WRITE setNumSteps)
        //! The number of decimals the value is returned with.
        Q_PROPERTY(int decimals READ decimals WRITE setDecimals)
        //! Current value.
        Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
        //! Soft minimum value. Sets the minimum value on the Slider.
        Q_PROPERTY(double softMinimum READ softMinimum WRITE setSoftMinimum)
        //! Soft maximum value. Sets the maximum value on the Slider.
        Q_PROPERTY(double softMaximum READ softMaximum WRITE setSoftMaximum)
        //! An optional non-linear scale setting for the slider using a power curve.
        //! Defaults to 0.5, which is a linear curve. Lowering or raising the midpoint value
        //! will shift the scale to have higher precision at the lower or higher end, respectively.
        Q_PROPERTY(double curveMidpoint READ curveMidpoint WRITE setCurveMidpoint)

    public:
        using value_type = double;

        explicit SliderDoubleCombo(QWidget *parent = nullptr);
        ~SliderDoubleCombo();

        void resetLimits();

        //! Sets the current value.
        void setValue(double value);
        //! Sets the current value.
        void setValueSlider(double value);
        //! Return the current value.
        Q_REQUIRED_RESULT double value() const;

        //! Sets the hard minimum value selectable via the spinbox.
        void setMinimum(double min);
        //! Returns the hard minimum value selectable via the spinbox.
        Q_REQUIRED_RESULT double minimum() const;

        //! Sets the hard maximum value selectable via the spinbox.
        void setMaximum(double max);
        //! Returns the hard maximum value selectable via the spinbox.
        Q_REQUIRED_RESULT double maximum() const;

        //! Sets the hard minimum and maximum values selectable via the spinbox.
        void setRange(double min, double max);

        //! Sets the soft minimum value selectable via the slider.
        void setSoftMinimum(double min);
        //! Returns the soft minimum value selectable via the slider.
        Q_REQUIRED_RESULT double softMinimum() const;

        //! Sets the soft maximum value selectable via the slider.
        void setSoftMaximum(double max);
        //! Returns the soft maximum value selectable via the slider.
        Q_REQUIRED_RESULT double softMaximum() const;

        //! Returns true if a soft minimum is set for this control.
        Q_REQUIRED_RESULT bool hasSoftMinimum() const { return m_useSoftMinimum; }
        //! Returns true if a soft maximum is set for this control.
        Q_REQUIRED_RESULT bool hasSoftMaximum() const { return m_useSoftMaximum; }

        //! Sets the soft minimum and maximum values selectable via the slider.
        void setSoftRange(double min, double max);

        //! Sets the number of steps, which are used when changing value via the arrow keys.
        void setNumSteps(int steps);
        //! Returns the number of steps.
        Q_REQUIRED_RESULT int numSteps() const;

        //! Sets the number of decimals the value is returned with.
        void setDecimals(int decimals);
        //! Returns the number of decimals the value is returned with.
        Q_REQUIRED_RESULT int decimals() const;

        //! Sets a non-linear scale power curve for the slider.
        //! Defaults to 0.5, which is a linear curve. Lowering or raising the midpoint value
        //! will shift the scale to have higher precision at the lower or higher end, respectively.
        void setCurveMidpoint(double midpoint);
        //! Returns the current curve midpoint setting.
        Q_REQUIRED_RESULT double curveMidpoint() const;

        //! Returns the underlying spinbox.
        DoubleSpinBox* spinbox() const;
        //! Returns the underlying slider.
        SliderDouble* slider() const;

    Q_SIGNALS:
        //! Triggered when this control's value has been changed.
        void valueChanged();
        //! Triggered when this control's value has stopped being changed.
        //! Only triggers once at the end of a drag operation.
        void editingFinished();

    protected:
        void focusInEvent(QFocusEvent* event) override;

    private Q_SLOTS:
        void updateSpinBox();
        void updateSlider();

    private:
        void refreshUi();

        SliderDouble* m_slider = nullptr;
        DoubleSpinBox* m_spinbox = nullptr;
        double m_sliderMin = 0.0;
        double m_sliderMax = 100.0;
        bool m_useSoftMinimum = false;
        bool m_useSoftMaximum = false;
        double m_minimum = 0.0;
        double m_maximum = 100.0;
        double m_softMinimum = 0.0;
        double m_softMaximum = 100.0;
        double m_value = 0.0;
    };
} // namespace AzQtComponents
