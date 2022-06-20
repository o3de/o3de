/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/StyledSpinBox.h>

#include <math.h>

#include <QApplication>
#include <QIntValidator>
#include <QLineEdit>
#include <QSignalBlocker>

namespace AzQtComponents
{
    // Slider parameters
    const int sliderDefaultHeight = 35;
    const int sliderDefaultWidth = 192;

    // Decimal precision parameters
    const int decimalPrecisonDefault = 7;
    const int decimalDisplayPrecisionDefault = 3;

    class FocusInEventFilterPrivate
        : public QObject
    {
    public:
        FocusInEventFilterPrivate(StyledDoubleSpinBox* spinBox)
            : QObject(spinBox)
            , m_spinBox(spinBox) {}

    protected:
        bool eventFilter(QObject* obj, QEvent* event) override
        {
            if (event->type() == QEvent::FocusIn)
            {
                m_spinBox->displaySlider();
            }
            else if (event->type() == QEvent::FocusOut)
            {
                // Don't allow the focus out event to propogate if the mouse was
                // clicked on the slider
                if (m_spinBox->isMouseOnSlider())
                {
                    return true;
                }
                // Otherwise, hide the slider once we have lost focus
                else
                {
                    m_spinBox->hideSlider();
                }
            }

            return QObject::eventFilter(obj, event);
        }
    private:
        StyledDoubleSpinBox* m_spinBox;
    };

    class ClickEventFilterPrivate
        : public QObject
    {
    public:
        explicit ClickEventFilterPrivate(StyledDoubleSpinBox* spinBox, QSlider* slider)
            : QObject(spinBox)
            , m_spinBox(spinBox)
        {
            if (slider)
            {
                connect(slider, &QSlider::sliderPressed, this, [this] { m_dragging = true; });
                connect(slider, &QSlider::sliderReleased, this, [this] { m_dragging = false; });
            }
        }
        ~ClickEventFilterPrivate() override {}
    signals:
        void clickOnApplication(const QPoint& pos);
    protected:
        bool eventFilter(QObject* obj, QEvent* event) override
        {
            if (event->type() == QEvent::MouseButtonRelease && !m_dragging)
            {
                m_spinBox->handleClickOnApp(QCursor::pos());
            }

            return QObject::eventFilter(obj, event);
        }
    private:
        StyledDoubleSpinBox* m_spinBox;
        bool m_dragging = false;
    };

    StyledDoubleSpinBox::StyledDoubleSpinBox(QWidget* parent)
        : QDoubleSpinBox(parent)
        , m_restrictToInt(false)
        , m_customSliderMinValue(0.0f)
        , m_customSliderMaxValue(0.0f)
        , m_hasCustomSliderRange(false)
        , m_slider(nullptr)
        , m_ignoreNextUpdateFromSlider(false)
        , m_ignoreNextUpdateFromSpinBox(false)
        , m_displayDecimals(decimalDisplayPrecisionDefault)
    {
        setProperty("class", "SliderSpinBox");
        setButtonSymbols(QAbstractSpinBox::NoButtons);

        // Set the default decimal precision we will store to a large number
        // since we will be truncating the value displayed
        setDecimals(decimalPrecisonDefault);

        // Our tooltip will be the full decimal value, so keep it updated
        // whenever our value changes
        QObject::connect(this, static_cast<void(StyledDoubleSpinBox::*)(double)>(&StyledDoubleSpinBox::valueChanged), this, &StyledDoubleSpinBox::UpdateToolTip);
        UpdateToolTip(value());
    }

    StyledDoubleSpinBox::~StyledDoubleSpinBox()
    {
        delete m_slider;
    }

    void StyledDoubleSpinBox::SetDisplayDecimals(int precision)
    {
        m_displayDecimals = precision;
    }

    QString StyledDoubleSpinBox::StringValue(double value, bool truncated) const
    {
        // Determine which decimal precision to use for displaying the value
        int numDecimals = decimals();
        if (truncated && m_displayDecimals < numDecimals)
        {
            numDecimals = m_displayDecimals;
        }

        QString stringValue = locale().toString(value, 'f', numDecimals);

        // Handle special cases when we have decimals in our value
        if (numDecimals > 0)
        {
            // Remove trailing zeros, since the locale conversion won't do
            // it for us
            QChar zeroDigit = locale().zeroDigit();
            QString trailingZeros = QString("%1+$").arg(zeroDigit);
            stringValue.remove(QRegExp(trailingZeros));

            // It's possible we could be left with a decimal point on the end
            // if we stripped the trailing zeros, so if that's the case, then
            // add a zero digit on the end so that it is obvious that this is
            // a float value
            QChar decimalPoint = locale().decimalPoint();
            if (stringValue.endsWith(decimalPoint))
            {
                stringValue.append(zeroDigit);
            }
        }

        // Copied from the QDoubleSpinBox sub-class to handle removing the
        // group separator if necessary
        if (!isGroupSeparatorShown() && qAbs(value) >= 1000.0)
        {
            stringValue.remove(locale().groupSeparator());
        }

        return stringValue;
    }

    QString StyledDoubleSpinBox::textFromValue(double value) const
    {
        // If our widget is focused, then show the full decimal value, otherwise
        // show the truncated value
        return StringValue(value, !hasFocus());
    }

    void StyledDoubleSpinBox::UpdateToolTip(double value)
    {
        // Set our tooltip to the full decimal value
        setToolTip(StringValue(value));
    }

    bool StyledDoubleSpinBox::SliderEnabled() const
    {
        // If the step size is set to 0, then don't show our slider
        return singleStep() != 0.0;
    }

    void StyledDoubleSpinBox::showEvent(QShowEvent* ev)
    {
        if (!m_slider)
        {
            initSlider();
        }

        prepareSlider();
        QWidget::showEvent(ev);
    }

    void StyledDoubleSpinBox::resizeEvent(QResizeEvent* ev)
    {
        if (!m_slider)
        {
            initSlider();
        }

        prepareSlider();
        QDoubleSpinBox::resizeEvent(ev);
    }

    void StyledDoubleSpinBox::focusInEvent(QFocusEvent* event)
    {
        QDoubleSpinBox::focusInEvent(event);

        // We need to set the special value text to an empty string, which
        // effectively makes no change, but actually triggers the line edit
        // display value to be updated so that when we receive focus to
        // begin editing, we display the full decimal precision instead of
        // the truncated display value
        setSpecialValueText(QString());
    }

    void StyledDoubleSpinBox::displaySlider()
    {
        if (!SliderEnabled())
        {
            return;
        }

        if (!m_slider)
        {
            initSlider();
        }

        prepareSlider();
        m_slider->setFocus();
        setProperty("SliderSpinBoxFocused", true);
        m_justPassFocusSlider = true;
        m_slider->show();
        m_slider->raise();
    }

    void StyledDoubleSpinBox::hideSlider()
    {
        if (!SliderEnabled())
        {
            return;
        }

        // prevent slider to hide when the
        // spinbox is passing focus along
        if (m_justPassFocusSlider)
        {
            m_justPassFocusSlider = false;
        }
        else
        {
            m_slider->hide();
            setProperty("SliderSpinBoxFocused", false);
            update();
        }
    }

    void StyledDoubleSpinBox::handleClickOnApp(const QPoint& pos)
    {
        if (isClickOnSlider(pos) || isClickOnSpinBox(pos))
        {
            if (m_slider && m_slider->isVisible())
            {
                displaySlider();
            }
        }
        else
        {
            hideSlider();
            clearFocus();
        }
    }

    bool StyledDoubleSpinBox::isMouseOnSlider()
    {
        const QPoint& globalPos = QCursor::pos();
        return isClickOnSlider(globalPos);
    }

    bool StyledDoubleSpinBox::isClickOnSpinBox(const QPoint& globalPos)
    {
        const auto pos = mapFromGlobal(globalPos);
        const auto spaceRect = QRect(0, 0, width(), height());
        return spaceRect.contains(pos);
    }

    bool StyledDoubleSpinBox::isClickOnSlider(const QPoint& globalPos)
    {
        if (!m_slider || m_slider->isHidden())
        {
            return false;
        }

        const auto pos = m_slider->mapFromGlobal(globalPos);
        auto spaceRect = m_slider->rect();
        return spaceRect.contains(pos);
    }

    void StyledDoubleSpinBox::SetCustomSliderRange(double min, double max)
    {
        m_customSliderMinValue = min;
        m_customSliderMaxValue = max;
        m_hasCustomSliderRange = true;
    }

    double StyledDoubleSpinBox::GetSliderMinimum()
    {
        if (m_hasCustomSliderRange)
        {
            return m_customSliderMinValue;
        }

        return minimum();
    }

    double StyledDoubleSpinBox::GetSliderRange()
    {
        if (m_hasCustomSliderRange)
        {
            return m_customSliderMaxValue - m_customSliderMinValue;
        }

        return maximum() - minimum();
    }

    void StyledDoubleSpinBox::prepareSlider()
    {
        if (!SliderEnabled())
        {
            return;
        }

        // If we are treating this as integer only (like QSpinBox), then we can
        // set our min/max and value directly to the slider since it uses integers
        if (m_restrictToInt)
        {
            // If we have a custom slider range, then set that, otherwise
            // use the same range for the slider as our spinbox
            if (m_hasCustomSliderRange)
            {
                m_slider->setMinimum(static_cast<int>(m_customSliderMinValue));
                m_slider->setMaximum(static_cast<int>(m_customSliderMaxValue));
            }
            else
            {
                m_slider->setMinimum(static_cast<int>(minimum()));
                m_slider->setMaximum(static_cast<int>(maximum()));
            }
        }
        // Otherwise, we need to set a custom scale for our slider using 0 as the
        // minimum and a power of 10 based on our decimal precision as the maximum
        else
        {
            int scaledMax = static_cast<int>(pow(10, (int)log10(GetSliderRange()) + decimals()));
            m_slider->setMinimum(0);
            m_slider->setMaximum(scaledMax);
        }

        // Set our slider value
        updateSliderValue(value());

        // detect the required background depending on how close to border is the spinbox
        auto globalWindow = QApplication::activeWindow();
        if (!globalWindow)
        {
            return;
        }

        auto spinBoxTopLeftGlobal = mapToGlobal(QPoint(0, 0));
        int globalWidth = globalWindow->x() + globalWindow->width();

        if (globalWidth - spinBoxTopLeftGlobal.x() < sliderDefaultWidth)
        {
            int offset = sliderDefaultWidth - width();
            m_slider->setStyleSheet("background: transparent; border-image: url(:/stylesheet/img/styledspinbox-bg-right.png);");
            m_slider->setGeometry(spinBoxTopLeftGlobal.x() - offset, spinBoxTopLeftGlobal.y() + height(), sliderDefaultWidth, sliderDefaultHeight);
        }
        else
        {
            m_slider->setStyleSheet("background: transparent; border-image: url(:/stylesheet/img/styledspinbox-bg-left.png);");
            m_slider->setGeometry(spinBoxTopLeftGlobal.x(), spinBoxTopLeftGlobal.y() + height(), sliderDefaultWidth, sliderDefaultHeight);
        }
    }

    void StyledDoubleSpinBox::initSlider()
    {
        if (!SliderEnabled())
        {
            return;
        }

        m_slider = new StyledSliderPrivate(this);
        m_slider->setWindowFlags(Qt::WindowFlags(Qt::Window) | Qt::WindowFlags(Qt::FramelessWindowHint));

        QObject::connect(this, static_cast<void(StyledDoubleSpinBox::*)(double)>(&StyledDoubleSpinBox::valueChanged),
            this, &StyledDoubleSpinBox::updateSliderValue);

        QObject::connect(m_slider, &QSlider::valueChanged,
            this, &StyledDoubleSpinBox::updateValueFromSlider);

        // These event filters will be automatically removed when our spin box is deleted
        // since they are parented to it
        qApp->installEventFilter(new ClickEventFilterPrivate(this, m_slider));
        installEventFilter(new FocusInEventFilterPrivate(this));
    }

    void StyledDoubleSpinBox::updateSliderValue(double newVal)
    {
        if (!m_slider)
        {
            return;
        }

        // Ignore this update if it was triggered by the user changing the slider,
        // which updated our spin box value
        if (m_ignoreNextUpdateFromSpinBox)
        {
            m_ignoreNextUpdateFromSpinBox = false;
            return;
        }

        // No need to continue if the slider value didn't change
        int currentSliderValue = m_slider->value();
        int sliderValue = ConvertToSliderValue(newVal);
        if (sliderValue == currentSliderValue)
        {
            return;
        }

        // Since we are about to set the slider value, flag the next update
        // to be ignored the slider so that it doesn't cause an extra loop,
        // but only if the value wouldn't be out of bounds in the case where
        // our slider range is different than our text input range, because
        // if the value would be out of range for the slider, the value won't
        // actually change if the slider value is already at the min or max
        // value
        bool outOfBounds = false;
        if (m_hasCustomSliderRange)
        {
            if (m_restrictToInt)
            {
                if (currentSliderValue == m_customSliderMaxValue && sliderValue > m_customSliderMaxValue)
                {
                    outOfBounds = true;
                }
                else if (currentSliderValue == m_customSliderMinValue && sliderValue < m_customSliderMinValue)
                {
                    outOfBounds = true;
                }
            }
            else
            {
                if (currentSliderValue == m_slider->maximum() && sliderValue > m_slider->maximum())
                {
                    outOfBounds = true;
                }
                else if (currentSliderValue == 0 && sliderValue < 0)
                {
                    outOfBounds = true;
                }
            }
        }
        if (!outOfBounds)
        {
            m_ignoreNextUpdateFromSlider = true;
        }

        // Update the slider value
        m_slider->setValue(sliderValue);
    }

    void StyledDoubleSpinBox::updateValueFromSlider(int newVal)
    {
        if (!m_slider)
        {
            return;
        }

        // Ignore this updated if it was triggered by the user changing the spin box,
        // which updated our slider value
        if (m_ignoreNextUpdateFromSlider)
        {
            m_ignoreNextUpdateFromSlider = false;
            return;
        }

        // No need to continue of the spinbox value didn't change
        double currentSpinBoxValue = value();
        double spinBoxValue = ConvertFromSliderValue(newVal);
        if (spinBoxValue == currentSpinBoxValue)
        {
            return;
        }

        // Since we are about to set the spin box value, flag the next update
        // to be ignored the spin box so that it doesn't cause an extra loop
        m_ignoreNextUpdateFromSpinBox = true;

        // Update the spin box value
        setValue(spinBoxValue);
    }

    int StyledDoubleSpinBox::ConvertToSliderValue(double spinBoxValue)
    {
        // If we are in integer only mode, we can cast the value directly
        int newVal;
        if (m_restrictToInt)
        {
            newVal = (int)spinBoxValue;
        }
        // Otherwise we need to convert our spin box double value to an
        // appropriate integer value for our custom slider scale
        else
        {
            newVal = static_cast<int>((spinBoxValue - GetSliderMinimum()) / GetSliderRange()) * m_slider->maximum();
        }

        return newVal;
    }

    double StyledDoubleSpinBox::ConvertFromSliderValue(int sliderValue)
    {
        // If we are in integer only mode, we can cast the value directly
        double newVal;
        if (m_restrictToInt)
        {
            newVal = (double)sliderValue;
        }
        // Otherwise we need to convert our slider int value from its custom
        // scale to an appropriate double value for our spin box
        else
        {
            double sliderScale = (double)sliderValue / (double)m_slider->maximum();
            newVal = (sliderScale * GetSliderRange()) + GetSliderMinimum();
        }

        return newVal;
    }

    StyledSliderPrivate::StyledSliderPrivate(QWidget* parent /*= nullptr*/)
        : QSlider(parent)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setOrientation(Qt::Horizontal);
        hide();
    }

    StyledSpinBox::StyledSpinBox(QWidget* parent)
        : StyledDoubleSpinBox(parent)
        , m_validator(new QIntValidator(minimum(), maximum(), this))
    {
        // This StyledSpinBox mirrors the same functionality of the QSpinBox, so
        // we need to set this flag so our StyledDoubleSpinBox knows to behave
        // assuming only integer values
        m_restrictToInt = true;

        // To enforce integer only input, we set our decimal precision to 0 and
        // change the validator of the QLineEdit to only accept integers
        setDecimals(0);
        QLineEdit* lineEdit = findChild<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly);
        if (lineEdit)
        {
            lineEdit->setValidator(m_validator);
        }

        // Added a valueChanged signal with an int parameter to mirror the behavior
        // of the QSpinBox
        QObject::connect(this, static_cast<void(StyledDoubleSpinBox::*)(double)>(&StyledDoubleSpinBox::valueChanged), this, [this](double val) {
            emit valueChanged((int)val);
        });
    }

    int StyledSpinBox::maximum() const
    {
        return (int)StyledDoubleSpinBox::maximum();
    }

    int StyledSpinBox::minimum() const
    {
        return (int)StyledDoubleSpinBox::minimum();
    }

    void StyledSpinBox::setMaximum(int max)
    {
        StyledDoubleSpinBox::setMaximum((double)max);

        // Update our validator maximum
        m_validator->setTop(max);
    }

    void StyledSpinBox::setMinimum(int min)
    {
        StyledDoubleSpinBox::setMinimum((double)min);

        // Update our validator minimum
        m_validator->setBottom(min);
    }

    void StyledSpinBox::setRange(int min, int max)
    {
        StyledDoubleSpinBox::setRange((double)min, (double)max);

        // Update our validator range
        m_validator->setRange(min, max);
    }

    void StyledSpinBox::setSingleStep(int val)
    {
        StyledDoubleSpinBox::setSingleStep((double)val);
    }

    int StyledSpinBox::singleStep() const
    {
        return (int)StyledDoubleSpinBox::singleStep();
    }

    int StyledSpinBox::value() const
    {
        return (int)StyledDoubleSpinBox::value();
    }

    void StyledSpinBox::setValue(int val)
    {
        StyledDoubleSpinBox::setValue((double)val);
    }

} // namespace AzQtComponents

#include "Components/moc_StyledSpinBox.cpp"
