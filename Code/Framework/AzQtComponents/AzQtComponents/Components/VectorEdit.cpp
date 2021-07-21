/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzQtComponents/Components/VectorEdit.h>

#include <QHBoxLayout>
#include <QDoubleValidator>
#include <QSignalBlocker>
#include <QIcon>

namespace AzQtComponents
{
    // An individual line edit with label
    VectorEditElement::VectorEditElement(const QString& label, QWidget* parent)
        : QWidget(parent)
        , m_lineEdit(new QLineEdit())
        , m_label(new QLabel(label))
        , m_color(Qt::white)
        , m_flavor(VectorEditElement::Plain)
    {
        m_lineEdit->setFixedWidth(33);
        auto layout = new QHBoxLayout(this);
        layout->addWidget(m_label);
        layout->addWidget(m_lineEdit);
        layout->setMargin(0);
        layout->setSpacing(3);

        m_lineEdit->setValidator(new QDoubleValidator(this));
        connect(m_lineEdit, &QLineEdit::textChanged,
            this, &VectorEditElement::valueChanged);

        m_label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    }

    float VectorEditElement::value() const
    {
        return m_lineEdit->text().toFloat();
    }

    QString VectorEditElement::label() const
    {
        return m_label->text();
    }

    void VectorEditElement::setLabel(const QString& text)
    {
        m_label->setText(text);
    }

    QColor VectorEditElement::color() const
    {
        return m_color;
    }

    void VectorEditElement::setColor(const QColor& color)
    {
        if (color != m_color)
        {
            m_color = color;
            updateColor();
        }
    }

    void VectorEditElement::setFlavor(VectorEditElement::Flavor flavor)
    {
        if (flavor != m_flavor)
        {
            m_flavor = flavor;
            updateColor();
        }
    }

    void VectorEditElement::updateColor()
    {
        if (m_flavor == Plain)
        {
            if (m_color.isValid())
            {
                m_lineEdit->setStyleSheet(QStringLiteral("QLineEdit:enabled { border: 1px solid \"%1\"; }").arg(m_color.name()));
                m_label->setStyleSheet(QStringLiteral("QLabel { color: \"%1\"; }").arg(m_color.name()));
            }
            else
            {
                m_lineEdit->setStyleSheet({});
            }
        }
        else
        {
            m_lineEdit->setStyleSheet({});
        }

        m_label->update();
        m_lineEdit->update();
    }

    void VectorEditElement::setValue(float value)
    {
        m_lineEdit->setText(QString::number(value));
    }

    VectorEdit::VectorEdit(QWidget* parent)
        : QWidget(parent)
        , m_flavor(VectorEditElement::Plain)
        , m_iconLabel(new QLabel())
    {
        m_iconLabel->setFixedWidth(16);

        // Two layouts so we can have smaller spacing between icon label and line edits
        auto outterLayout = new QHBoxLayout(this);
        outterLayout->setMargin(0);
        outterLayout->setSpacing(5);
        auto container = new QWidget();
        auto layout = new QHBoxLayout(container);

        layout->setMargin(0);
        layout->setSpacing(15);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

        const QStringList labels = { tr("X"), tr("Y"), tr("Z") };
        m_editElements.reserve(labels.size());
        for (const QString& label : labels)
        {
            auto element = new VectorEditElement(label, this);
            layout->addWidget(element);
            m_editElements.append(element);
            connect(element, &VectorEditElement::valueChanged,
                this, &VectorEdit::vectorChanged);
        }

        outterLayout->addSpacing(4);
        outterLayout->addWidget(m_iconLabel);
        outterLayout->addWidget(container);
    }

    QVector3D VectorEdit::vector() const
    {
        return {
                   x(), y(), z()
        };
    }

    float VectorEdit::x() const
    {
        return m_editElements.at(0)->value();
    }

    float VectorEdit::y() const
    {
        return m_editElements.at(1)->value();
    }

    float VectorEdit::z() const
    {
        return m_editElements.at(2)->value();
    }

    QString VectorEdit::xLabel() const
    {
        return m_editElements.at(0)->label();
    }

    QString VectorEdit::yLabel() const
    {
        return m_editElements.at(1)->label();
    }

    QString VectorEdit::zLabel() const
    {
        return m_editElements.at(2)->label();
    }

    void VectorEdit::setLabels(const QString& xLabel, const QString& yLabel, const QString& zLabel)
    {
        setLabels({ xLabel, yLabel, zLabel });
    }

    void VectorEdit::setLabels(const QStringList& labels)
    {
        for (int i = 0; i < qMin(labels.size(), 3); ++i)
        {
            m_editElements.at(i)->setLabel(labels.at(i));
        }
    }

    QColor VectorEdit::xColor() const
    {
        return m_editElements.at(0)->color();
    }

    QColor VectorEdit::yColor() const
    {
        return m_editElements.at(1)->color();
    }

    QColor VectorEdit::zColor() const
    {
        return m_editElements.at(2)->color();
    }

    void VectorEdit::setColors(const QColor& xColor, const QColor& yColor, const QColor& zColor)
    {
        setXColor(xColor);
        setYColor(yColor);
        setZColor(zColor);
    }

    void VectorEdit::setXColor(const QColor& color)
    {
        m_editElements.at(0)->setColor(color);
    }

    void VectorEdit::setYColor(const QColor& color)
    {
        m_editElements.at(1)->setColor(color);
    }

    void VectorEdit::setZColor(const QColor& color)
    {
        m_editElements.at(2)->setColor(color);
    }

    VectorEditElement::Flavor VectorEdit::flavor() const
    {
        return m_flavor;
    }

    void VectorEdit::setFlavor(VectorEditElement::Flavor flavor)
    {
        if (flavor == m_flavor)
        {
            return;
        }

        m_flavor = flavor;
        foreach(auto edit, m_editElements)
        {
            edit->setFlavor(flavor);
        }

        if (flavor == VectorEditElement::Invalid)
        {
            setPixmap(QPixmap(":/stylesheet/img/lineedit-invalid.png"));
        }
        else
        {
            setPixmap(QPixmap());
        }

        emit flavorChanged();
    }

    void VectorEdit::setPixmap(const QPixmap& icon)
    {
        m_iconLabel->setPixmap(icon);
    }

    void VectorEdit::setVector(QVector3D vec)
    {
        setVector(vec.x(), vec.y(), vec.z());
    }

    void VectorEdit::setVector(float xValue, float yValue, float zValue)
    {
        if (qFuzzyCompare(xValue, x()) && qFuzzyCompare(yValue, y()) && qFuzzyCompare(zValue, z()))
        {
            return;
        }

        QSignalBlocker sb0(m_editElements.at(0));
        QSignalBlocker sb1(m_editElements.at(1));
        QSignalBlocker sb2(m_editElements.at(2));
        setX(xValue);
        setY(yValue);
        setZ(zValue);

        emit vectorChanged();
    }

    void VectorEdit::setX(float v)
    {
        m_editElements.at(0)->setValue(v);
        // Signal is emitted automatically
    }

    void VectorEdit::setY(float v)
    {
        m_editElements.at(1)->setValue(v);
        // Signal is emitted automatically
    }

    void VectorEdit::setZ(float v)
    {
        m_editElements.at(2)->setValue(v);
        // Signal is emitted automatically
    }

}

#include "Components/moc_VectorEdit.cpp"
