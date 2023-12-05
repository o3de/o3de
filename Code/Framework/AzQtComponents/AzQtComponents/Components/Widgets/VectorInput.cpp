/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/VectorInput.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzCore/Math/Transform.h>
#include <AzCore/std/limits.h>

#include <QLabel>
#include <QStyleOptionSpinBox>
#include <QHBoxLayout>
#include <QEvent>

namespace AzQtComponents
{

static constexpr const char* g_CoordinatePropertyName = "Coordinate";

static int g_CoordLabelMargins = 2; // Keep this up to date with css
// To be populated by VectorInput::initStaticVars
static int g_labelSize = -1;

VectorElement::VectorElement(QWidget* parent)
    : QWidget(parent)
    , m_spinBox(new AzQtComponents::DoubleSpinBox(this))
    , m_label(new QLabel(this))
{
    m_spinBox->setKeyboardTracking(false); // don't send valueChanged every time a character is typed (no need for undo/redo per digit of a double value)

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(2);

    VectorElement::layout(this, m_spinBox, m_label, false);

    connect(m_spinBox, QOverload<double>::of(&AzQtComponents::DoubleSpinBox::valueChanged), this, &VectorElement::onValueChanged);
    connect(m_spinBox, &AzQtComponents::DoubleSpinBox::editingFinished, this, &VectorElement::onSpinBoxEditingFinished);
}

void VectorElement::SetLabel(const char* label)
{
    setLabel(label);
}

void VectorElement::setLabel(const QString& label)
{
    m_labelText = label;
    m_label->setText(label);
    resizeLabel();
}

const QString& VectorElement::label() const
{
    return m_labelText;
}

void VectorElement::setValue(double newValue)
{
    // Nothing to do if the value is not actually changed.
    // Cast to float for comparison because our vector data is stored in floats.
    // Otherwise, small changes may be detected when comparing values
    if (AZ::IsCloseMag(static_cast<float>(m_value), static_cast<float>(newValue), AZStd::numeric_limits<float>::epsilon()))
    {
        return;
    }

    // If the spin box currently has focus, the user is editing it, so we should not
    // change the value from non-user input while they're in the middle of editing
    if (m_spinBox->hasFocus())
    {
        auto& deferredValue = m_deferredExternalValue.emplace();
        deferredValue.value = newValue;
        deferredValue.prevValue = m_value;
        return;
    }

    m_value = newValue;
    const QSignalBlocker blocker(m_spinBox);
    m_spinBox->setValue(newValue);
    m_wasValueEditedByUser = false;
    emit valueChanged(newValue);
}

void VectorElement::onSpinBoxEditingFinished()
{
    if (m_deferredExternalValue)
    {
        DeferredSetValue deferredValue = *m_deferredExternalValue;
        m_deferredExternalValue.reset();

        if (m_value == deferredValue.prevValue)
        {
            AZ_Assert(!m_spinBox->hasFocus(), "Editing finished but the spinbox still has focus");
            setValue(deferredValue.value);
        }
    }

    emit editingFinished();
}

void VectorElement::setCoordinate(VectorElement::Coordinate coordinate)
{
    setProperty(g_CoordinatePropertyName, QVariant::fromValue(coordinate));
}

void VectorElement::changeEvent(QEvent* event) {

    if(event->type() == QEvent::EnabledChange) {
         style()->unpolish(m_label);
         style()->polish(m_label);
    }
    QWidget::changeEvent(event);
}

QSize VectorElement::sizeHint() const
{
    QSize size = m_spinBox->sizeHint();
    const int labelWidth = label().length() > 1 ? m_label->width() : g_labelSize;
    size.rwidth() += labelWidth;
    return size;
}

void VectorElement::onValueChanged(double newValue)
{
    if (!AZ::IsClose(m_value, newValue, std::numeric_limits<double>::epsilon()))
    {
        m_value = newValue;
        m_wasValueEditedByUser = true;
        emit valueChanged(newValue);
    }
}

void VectorElement::layout(QWidget* element, QAbstractSpinBox* spinBox, QLabel* label, bool ui20)
{
    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(element->layout());
    if (!layout)
    {
        return;
    }

    if (layout->isEmpty())
    {
        layout->addWidget(spinBox, 1);
    }

    if (ui20)
    {
        label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

        layout->removeWidget(label);
        label->setParent(spinBox);
        label->move(0, 0);
        label->show();
    }
    else
    {
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        label->setParent(element);
        layout->insertWidget(0, label);
    }
}

QRect VectorElement::editFieldRect(const QProxyStyle* style, const QStyleOptionComplex* option, const QWidget* widget, const SpinBox::Config& config)
{
    if (qstyleoption_cast<const QStyleOptionSpinBox *>(option))
    {
        auto rect = SpinBox::editFieldRect(style, option, widget, config);

        int labelWidth = g_labelSize;

        if (auto vectorElement = static_cast<const VectorElement*>(widget->parent()))
        {
            if (vectorElement->label().length() > 1)
            {
                labelWidth = vectorElement->getLabelWidget()->width() - (g_CoordLabelMargins * 2);
            }
        }

        rect.adjust(labelWidth, 0, 0, 0);
        return rect;
    }

    return QRect();
}

bool VectorElement::polish(QProxyStyle* style, QWidget* widget, const SpinBox::Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(config);

    auto element = qobject_cast<VectorElement*>(widget);
    if (!element)
    {
        return false;
    }

    VectorElement::layout(element, element->m_spinBox, element->m_label, true);
    element->resizeLabel();

    return true;
}

bool VectorElement::unpolish(QProxyStyle* style, QWidget* widget, const SpinBox::Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(config);

    auto element = qobject_cast<VectorElement*>(widget);
    if (!element)
    {
        return false;
    }

    VectorElement::layout(element, element->m_spinBox, element->m_label, false);

    return true;
}

void VectorElement::initStaticVars(int labelSize)
{
    g_labelSize = labelSize;
}

void VectorElement::resizeLabel()
{
    QSize size = m_label->sizeHint();
    const int minimumSize = g_labelSize + g_CoordLabelMargins * 2;
    size.setHeight(minimumSize);
    if (size.width() <= minimumSize)
    {
        size.setWidth(minimumSize);
    }
    else
    {
        size += {g_CoordLabelMargins * 2, 0};
    }
    m_label->resize(size);
}

//////////////////////////////////////////////////////////////////////////

VectorInput::VectorInput(QWidget* parent, int elementCount, int elementsPerRow, QString customLabels)
    : QWidget(parent)
{
    // Set up Qt layout
    QGridLayout* pLayout = new QGridLayout(this);
    pLayout->setMargin(0);
    pLayout->setSpacing(2);
    pLayout->setContentsMargins(1, 0, 1, 0);

    AZ_Warning("Property Editor", elementCount >= 2,
        "Vector controls with less than 2 elements are not supported");

    if (elementCount < 2)
    {
        elementCount = 2;
    }

    m_elementCount = elementCount;
    m_elements = new VectorElement*[m_elementCount];

    // Setting up custom labels
    QString labels = customLabels.isEmpty() ? QStringLiteral("XYZW") : customLabels;

    // Adding elements to the layout
    int numberOfElementsRemaining = m_elementCount;
    int numberOfRowsInLayout = elementsPerRow <= 0 ? 1 : static_cast<int>(std::ceil(static_cast<float>(m_elementCount) / static_cast<float>(elementsPerRow)));
    int actualElementsPerRow = elementsPerRow <= 0 ? m_elementCount : elementsPerRow;

    for (int rowIdx = 0; rowIdx < numberOfRowsInLayout; rowIdx++)
    {
        QHBoxLayout* layout = new QHBoxLayout();
        for (int columnIdx = 0; columnIdx < actualElementsPerRow; columnIdx++)
        {
            if (numberOfElementsRemaining > 0)
            {
                int elementIndex = rowIdx * elementsPerRow + columnIdx;

                m_elements[elementIndex] = new VectorElement(this);
                m_elements[elementIndex]->setObjectName(labels.mid(elementIndex, 1));
                m_elements[elementIndex]->setLabel(labels.mid(elementIndex, 1));
                m_elements[elementIndex]->setCoordinate(static_cast<VectorElement::Coordinate>(elementIndex % 4));

                layout->addWidget(m_elements[elementIndex]);

                auto valueChanged = QOverload<double>::of(&AzQtComponents::DoubleSpinBox::valueChanged);
                connect(m_elements[elementIndex]->GetSpinBox(), valueChanged, this, [elementIndex, this](double value)
                    {
                        OnValueChangedInElement(value, elementIndex);
                    });
                connect(m_elements[elementIndex], &VectorElement::editingFinished, this, &VectorInput::editingFinished);

                numberOfElementsRemaining--;
            }
        }

        // Add internal layout to the external grid layout
        pLayout->addLayout(layout, rowIdx, 0);
    }
}

VectorInput::~VectorInput()
{
    delete[] m_elements;
}

void VectorInput::setLabel(int index, const QString& label)
{
    AZ_Warning("PropertyGrid", index < m_elementCount,
        "This control handles only %i controls", m_elementCount);
    if (index < m_elementCount)
    {
        m_elements[index]->setLabel(label);
    }
}

void VectorInput::setLabelStyle(int index, const QString& qss)
{
    AZ_Warning("PropertyGrid", index < m_elementCount,
        "This control handles only %i controls", m_elementCount);
    if (index < m_elementCount)
    {
        m_elements[index]->getLabelWidget()->setStyleSheet(qss);
    }
}

void VectorInput::setValuebyIndex(double value, int elementIndex)
{
    AZ_Warning("PropertyGrid", elementIndex < m_elementCount,
        "This control handles only %i controls", m_elementCount);
    if (elementIndex < m_elementCount)
    {
        m_elements[elementIndex]->setValue(value);
    }
}

void VectorInput::setMinimum(double value)
{
    for (size_t i = 0; i < m_elementCount; ++i)
    {
        auto spinBox = m_elements[i]->GetSpinBox();

        // Avoid setting this value if its close to the current value, because otherwise Qt will pump events into the queue to redraw/etc.
        if (!AZ::IsClose(spinBox->minimum(), value, DBL_EPSILON))
        {
            spinBox->blockSignals(true);
            spinBox->setMinimum(value);
            spinBox->blockSignals(false);
        }
    }
}

void VectorInput::setMaximum(double value)
{
    for (size_t i = 0; i < m_elementCount; ++i)
    {
        auto spinBox = m_elements[i]->GetSpinBox();

        // Avoid setting this value if its close to the current value, because otherwise Qt will pump events into the queue to redraw/etc.
        if (!AZ::IsClose(spinBox->maximum(), value, DBL_EPSILON))
        {
            spinBox->blockSignals(true);
            spinBox->setMaximum(value);
            spinBox->blockSignals(false);
        }
    }
}

void VectorInput::setStep(double value)
{
    for (size_t i = 0; i < m_elementCount; ++i)
    {
        m_elements[i]->GetSpinBox()->blockSignals(true);
        m_elements[i]->GetSpinBox()->setSingleStep(value);
        m_elements[i]->GetSpinBox()->blockSignals(false);
    }
}

void VectorInput::setDecimals(int value)
{
    for (size_t i = 0; i < m_elementCount; ++i)
    {
        m_elements[i]->GetSpinBox()->blockSignals(true);
        m_elements[i]->GetSpinBox()->setDecimals(value);
        m_elements[i]->GetSpinBox()->blockSignals(false);
    }
}

void VectorInput::setDisplayDecimals(int value)
{
    for (size_t i = 0; i < m_elementCount; ++i)
    {
        m_elements[i]->GetSpinBox()->blockSignals(true);
        m_elements[i]->GetSpinBox()->setDisplayDecimals(value);
        m_elements[i]->GetSpinBox()->blockSignals(false);
    }
}

void VectorInput::OnValueChangedInElement(double newValue, int elementIndex)
{
    Q_EMIT valueChanged(newValue);
    Q_EMIT valueAtIndexChanged(elementIndex, newValue);
}

void VectorInput::setSuffix(const QString& label)
{
    for (size_t i = 0; i < m_elementCount; ++i)
    {
        m_elements[i]->GetSpinBox()->blockSignals(true);
        m_elements[i]->GetSpinBox()->setSuffix(label);
        m_elements[i]->GetSpinBox()->blockSignals(false);
    }
}

QWidget* VectorInput::GetFirstInTabOrder()
{
    return m_elements[0]->GetSpinBox();
}

QWidget* VectorInput::GetLastInTabOrder()
{
    return m_elements[m_elementCount - 1]->GetSpinBox();
}

void VectorInput::UpdateTabOrder()
{
    for (int i = 0; i < m_elementCount - 1; i++)
    {
        setTabOrder(m_elements[i]->GetSpinBox(), m_elements[i + 1]->GetSpinBox());
    }
}

}

#include <Components/Widgets/moc_VectorInput.cpp>
