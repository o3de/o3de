/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/VectorInput.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzCore/Math/Transform.h>
#include <AzCore/std/limits.h>

#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionSpinBox>
#include <QHBoxLayout>
#include <QEvent>

namespace AzQtComponents
{

static constexpr const char* g_CoordinatePropertyName = "Coordinate";

static int g_CoordLabelMargins = 2; // Keep this up to date with css

//! Everything private to this file that the flattened GUI adds. A named namespace, not an anonymous one: a unity build joins this file
//! with others into one translation unit, where anonymous namespaces merge.
namespace VectorInputFlat
{
    //! VectorInput.qss:9-16 `AzQtComponents--VectorElement QLabel { margin-top: 2px; margin-bottom: 2px; border-top-left-radius: 2px;
    //! border-bottom-left-radius: 2px }`: the label's fill lies 2px inside its rect above and below, with its two left corners rounded.
    //! TOKENIZE: VectorLabelMargin, VectorLabelRadius
    constexpr int LabelMargin = 2;
    constexpr int LabelRadius = 2;

    //! The fill of each coordinate, VectorInput.qss:37-55. TOKENIZE: VectorXColor, VectorYColor, VectorZColor, VectorWColor
    QColor coordinateColor(const QString& coordinate)
    {
        if (coordinate == QLatin1String("X"))
        {
            return QColor(0xE2, 0x52, 0x43);
        }
        if (coordinate == QLatin1String("Y"))
        {
            return QColor(0x58, 0xBC, 0x61);
        }
        if (coordinate == QLatin1String("Z"))
        {
            return QColor(0x42, 0x85, 0xF4);
        }
        if (coordinate == QLatin1String("W"))
        {
            return QColor(0xE5, 0x78, 0x29);
        }
        return QColor();
    }

    //! QRenderRule::borderClip (qstylesheetstyle.cpp:1288-1325) of that rule, which has no border: a path along the box with the arcs of
    //! its two left corners. QRenderRule::drawBackground fills it antialiased (:1348-1370), so at a fractional device ratio the rows at
    //! the edges blend.
    QPainterPath labelPath(const QRectF& box)
    {
        const qreal radius = LabelRadius;
        QPainterPath path;
        path.moveTo(box.left() + radius, box.top());
        path.lineTo(box.right(), box.top());
        path.lineTo(box.right(), box.bottom());
        path.lineTo(box.left() + radius, box.bottom());
        path.arcTo(QRectF(box.left(), box.bottom() - 2 * radius, 2 * radius, 2 * radius), 270, -90);
        path.lineTo(box.left(), box.top() + radius);
        path.arcTo(QRectF(box.left(), box.top(), 2 * radius, 2 * radius), 180, -90);
        path.closeSubpath();
        return path;
    }
} // namespace VectorInputFlat

// To be populated by VectorInput::initStaticVars
static int g_labelSize = -1;

VectorElement::VectorElement(QWidget* parent)
    : QWidget(parent)
    , m_spinBox(new AzQtComponents::DoubleSpinBox(this))
    , m_label(new QLabel(this))
{
    m_label->setAttribute(Qt::WA_Hover, true);
    m_label->setObjectName(SpinBox::s_draggableLabelName);
    m_spinBox->setKeyboardTracking(false); // don't send valueChanged every time a character is typed (no need for undo/redo per digit of a double value)

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);

    m_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(m_label);
    layout->addWidget(m_spinBox, 1);

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

void VectorElement::layout(QWidget* , QAbstractSpinBox* , QLabel* , bool )
{
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

    element->resizeLabel();
    if (auto label = element->findChild<QLabel*>(QString(), Qt::FindDirectChildrenOnly))
    {
        SpinBox::registerLabelToWatcher(label, true);
    }

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

    if (auto label = element->findChild<QLabel*>(QString(), Qt::FindDirectChildrenOnly))
    {
        SpinBox::registerLabelToWatcher(label, false);
    }

    return true;
}

bool VectorElement::drawFlattenedLabel(const QStyleOption* option, QPainter* painter, const QWidget* widget)
{
    // The label's rule drew its fill (the color of the vector's Coordinate) inside the margin it set above and below, which also
    // gave the label its frame box (Style's SE_ShapedFrameContents): so the fill is 4px shorter than the label's rect. A label
    // of a vector without a coordinate has no fill.
    if (!option || !widget || !widget->inherits("QLabel") || !Selectors::childOfClass(widget, "AzQtComponents::VectorElement"))
    {
        return false;
    }

    const QColor fill = VectorInputFlat::coordinateColor(widget->parentWidget()->property(g_CoordinatePropertyName).toString());
    if (fill.isValid())
    {
        const QRectF box(option->rect.adjusted(0, VectorInputFlat::LabelMargin, 0, -VectorInputFlat::LabelMargin));
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->fillPath(VectorInputFlat::labelPath(box), fill);
        painter->restore();
    }
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
                connect(m_elements[elementIndex]->GetSpinBox(), &AzQtComponents::DoubleSpinBox::valueChangeBegan, this, &VectorInput::valueChangeBegan);
                connect(m_elements[elementIndex]->GetSpinBox(), &AzQtComponents::DoubleSpinBox::valueChangeEnded, this, &VectorInput::valueChangeEnded);

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
        // an inline sheet of every vector label that has a style (StyleForX..W): the GuiPerfFloor noop and passthrough rungs leave it out
        AzQtComponents::GuiPerfFloor::setInlineStyleSheet(m_elements[index]->getLabelWidget(), qss);
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

