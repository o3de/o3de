/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ColorPicker/ColorHexEdit.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>

#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QEvent>
#include <QMenu>
#include <QApplication>
#include <QClipboard>

#include <functional>

namespace AzQtComponents
{

ColorHexEdit::ParsedColor ColorHexEdit::convertTextToColorValues(const QString& text, bool editAlpha, qreal fallbackAlpha)
{
    const uint rgb = text.toUInt(nullptr, 16);

    int redShift = 16;
    int greenShift = 8;
    int blueShift = 0;

    if (text.size() > 6 && editAlpha)
    {
        int offset = text.size() > 7 ? 8 : 4;
        redShift += offset;
        greenShift += offset;
        blueShift += offset;
    }

    auto pullChannel = [&rgb](int shiftAmount, int mask = 0xff) {
        const int intValue = (rgb >> shiftAmount) & mask;
        return static_cast<qreal>(intValue) / 255.0;
    };

    ParsedColor ret{ pullChannel(redShift), pullChannel(greenShift), pullChannel(blueShift), fallbackAlpha };

    if (text.size() > 6 && editAlpha)
    {
        // if there was only 7 characters, then the mask will only be the last hex character
        uint mask = (text.size() > 7) ? 0xff : 0xf;
        ret.alpha = pullChannel(0, mask);
    }

    return ret;
}

ColorHexEdit::ColorHexEdit(QWidget* parent)
    : QWidget(parent)
    , m_red(0.0)
    , m_green(0.0)
    , m_blue(0.0)
    , m_alpha(1.0)
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_edit = new QLineEdit(QStringLiteral("000000"), this);
    m_edit->setObjectName(QStringLiteral("colorhexedit"));
    m_edit->setValidator(new QRegExpValidator(QRegExp(QStringLiteral("^[0-9A-Fa-f]{1,6}$")), this));
    m_edit->setFixedWidth(52);
    LineEdit::setErrorIconEnabled(m_edit, false);
    connect(m_edit, &QLineEdit::textChanged, this, &ColorHexEdit::textChanged);
    connect(m_edit, &QLineEdit::editingFinished, this, [this]() {
        m_valueChanging = false;
        emit valueChangeEnded();
    });
    m_edit->installEventFilter(this);

    m_edit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_edit, &QWidget::customContextMenuRequested, this, &ColorHexEdit::showContextMenu);

    m_hexLabel = new QLabel(tr("#"), this);
    layout->addWidget(m_hexLabel);
    layout->addWidget(m_edit);
    layout->addStretch();

    initEditValue();
}

ColorHexEdit::~ColorHexEdit()
{
}

qreal ColorHexEdit::red() const
{
    return m_red;
}

qreal ColorHexEdit::green() const
{
    return m_green;
}

qreal ColorHexEdit::blue() const
{
    return m_blue;
}

qreal ColorHexEdit::alpha() const
{
    return m_alpha;
}

void ColorHexEdit::setRed(qreal red)
{
    if (qFuzzyCompare(red, m_red))
    {
        return;
    }
    m_red = red;
    initEditValue();
}

void ColorHexEdit::setGreen(qreal green)
{
    if (qFuzzyCompare(green, m_green))
    {
        return;
    }
    m_green = green;
    initEditValue();
}

void ColorHexEdit::setBlue(qreal blue)
{
    if (qFuzzyCompare(blue, m_blue))
    {
        return;
    }
    m_blue = blue;
    initEditValue();
}

void ColorHexEdit::setAlpha(qreal alpha)
{
    if (qFuzzyCompare(alpha, m_alpha))
    {
        return;
    }
    m_alpha = alpha;

    if (m_editAlpha)
    {
        initEditValue();
    }
}

void ColorHexEdit::setLabelVisible(bool visible)
{
    m_hexLabel->setVisible(visible);
}

bool ColorHexEdit::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_edit)
    {
        if (event->type() == QEvent::FocusIn)
        {
            m_valueChanging = true;
            emit valueChangeBegan();
        }
        else if (event->type() == QEvent::FocusOut)
        {
            m_valueChanging = false;
            emit valueChangeEnded();
        }
    }

    return false;
}

void ColorHexEdit::textChanged(const QString& text)
{
    if (!m_valueChanging)
    {
        m_valueChanging = true;
        emit valueChangeBegan();
    }

    ParsedColor parsedColor = convertTextToColorValues(text, m_editAlpha, m_alpha);

    auto setIfChanged = [](qreal parsedValue, qreal& member) -> bool
    {
        if (!qFuzzyCompare(member, parsedValue))
        {
            member = parsedValue;
            return true;
        }

        return false;
    };

    bool signalRedChanged = setIfChanged(parsedColor.red, m_red);
    bool signalGreenChanged = setIfChanged(parsedColor.green, m_green);
    bool signalBlueChanged = setIfChanged(parsedColor.blue, m_blue);
    bool signalAlphaChanged = setIfChanged(parsedColor.alpha, m_alpha);

    // need to emit after we're done updating all components
    // or we'll get signalled before we're done

    if (signalRedChanged)
    {
        emit redChanged(m_red);
    }

    if (signalGreenChanged)
    {
        emit greenChanged(m_green);
    }

    if (signalBlueChanged)
    {
        emit blueChanged(m_blue);
    }

    if (signalAlphaChanged)
    {
        emit alphaChanged(m_alpha);
    }
}

unsigned int convertToSingleValue(qreal realRed, qreal realGreen, qreal realBlue, qreal realAlpha, bool includeAlpha)
{
    const int red = qRound(realRed * 255.0);
    const int green = qRound(realGreen * 255.0);
    const int blue = qRound(realBlue * 255.0);

    if (includeAlpha)
    {
        const int alpha = qRound(realAlpha * 255.0);
        unsigned int rgba = alpha | (blue << 8) | (green << 16) | (red << 24);
        return rgba;
    }
    else
    {
        unsigned int rgb = blue | (green << 8) | (red << 16);
        return rgb;
    }
}

void ColorHexEdit::initEditValue()
{
    unsigned int rgb = convertToSingleValue(m_red, m_green, m_blue, m_alpha, m_editAlpha);

    if (rgb != m_edit->text().toUInt(nullptr, 16))
    {
        const auto value = QStringLiteral("%1").arg(rgb, 6, 16, QLatin1Char('0'));
        const QSignalBlocker b(this);
        m_edit->setText(value.toUpper());
    }
}

void ColorHexEdit::showContextMenu(const QPoint& pos)
{
    if (QMenu* menu = m_edit->createStandardContextMenu())
    {
        QClipboard* clipboard = QApplication::clipboard();

        if (clipboard)
        {
            QAction* firstAction = menu->actions().size() > 0 ? menu->actions()[0] : nullptr;

            QAction* action = new QAction(tr("Copy Value (With Alpha)"), menu);
            connect(action, &QAction::triggered, this, [this, clipboard]() {
                unsigned int rgba = convertToSingleValue(m_red, m_green, m_blue, m_alpha, true);
                const QString value = QStringLiteral("%1").arg(rgba, 8, 16, QLatin1Char('0'));
                clipboard->setText(value);
            });
            menu->insertAction(firstAction, action);

            action = new QAction(tr("Copy Value"), menu);
            connect(action, &QAction::triggered, this, [this, clipboard]() {
                clipboard->setText(m_edit->text());
            });
            menu->insertAction(firstAction, action);

            menu->insertSeparator(firstAction);
        }

        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->popup(m_edit->mapToGlobal(pos));
    }
}

} // namespace AzQtComponents

#include "Components/Widgets/ColorPicker/moc_ColorHexEdit.cpp"
