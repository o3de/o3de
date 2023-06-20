/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <cmath>

#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Utilities/Conversions.h>

#include <QApplication>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QGuiApplication>
#include <QLineEdit>
#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <QScreen>
#include <QSettings>
#include <QStyle>
#include <QStyleOptionSpinBox>
#include <QMenu>
#include <QTimer>
#include <QWindow>
#include <QTextLayout>

#include <QtWidgets/private/qstylesheetstyle_p.h>
#include <qcoreevent.h>

namespace AzQtComponents
{

static const char* g_hoverControlPropertyName = "HoverControl";
static const char* g_hoveredPropertyName = "hovered";
static const char* g_spinBoxUpPressedPropertyName = "SpinBoxUpButtonPressed";
static const char* g_spinBoxDownPressedPropertyName = "SpinBoxDownButtonPressed";
static const char* g_spinBoxValueIncreasingName = "SpinBoxValueIncreasing";
static const char* g_spinBoxValueDecreasingName = "SpinBoxValueDecreasing";
static const char* g_spinBoxScrollIncreasingName = "SpinBoxScrollIncreasing";
static const char* g_spinBoxScrollDecreasingName = "SpinBoxScrollDecreasing";
static const char* g_spinBoxScrollTimerName = "SpinBoxScrollTimer";
static const char* g_spinBoxIntializedValueName = "SpinBoxInitializedValue";
static const char* g_spinBoxMinReachedName = "SpinBoxMinReached";
static const char* g_spinBoxMaxReachedName = "SpinBoxMaxReached";
static const char* g_spinBoxFocusedName = "SpinBoxFocused";
static const char* g_spinBoxDraggingName = "SpinBoxDragging";
static const char* g_spinBoxInitPropertyFlagName = "PropertyInitFlag";

// Decimal precision parameters
static const int g_decimalPrecisonDefault = 7;
static const int g_decimalDisplayPrecisionDefault = 3;

class SpinBoxWatcher : public QObject
{
public:
    explicit SpinBoxWatcher(QObject* parent = nullptr);

    SpinBox::Config m_config;

    bool eventFilter(QObject* watched, QEvent* event) override;

    bool isEditing(const QAbstractSpinBox* spinBox) const { return m_spinBoxChanging == spinBox; }

private:
    enum State
    {
        Inactive,
        Dragging,
        ProcessingArrowButtons
    };
    int m_xPos = 0;
    QPointer<QAbstractSpinBox> m_spinBoxChanging = nullptr;
    QPointer<QScreen> m_activeScreen = nullptr;
    State m_state = Inactive;
    QAbstractSpinBox* m_mouseFocusedSpinBox = nullptr;
    QAbstractSpinBox* m_mouseFocusedSpinBoxSingleClicked = nullptr;

    bool filterSpinBoxEvents(QAbstractSpinBox* spinBox, QEvent* event);
    bool filterLineEditEvents(QLineEdit* lineEdit, QEvent* event);
    void setInitializeSpinboxValue(QAbstractSpinBox* spinBox, bool clearText = false);

    QTimer* SpinBoxScrollTimer(QAbstractSpinBox* spinBox);

    void initStyleOption(QAbstractSpinBox* spinBox, QStyleOptionSpinBox* styleOption);
    QAbstractSpinBox::StepEnabled stepEnabled(QAbstractSpinBox* spinBox);

    void emitValueChangeBegan(QAbstractSpinBox* spinBox);
    void emitValueChangeEnded(QAbstractSpinBox* spinBox);

    void resetCursor(QAbstractSpinBox* spinBox);
};

SpinBoxWatcher::SpinBoxWatcher(QObject* parent)
    : QObject(parent)
    , m_config(SpinBox::defaultConfig())
{

}

bool SpinBoxWatcher::eventFilter(QObject* watched, QEvent* event)
{
    bool filterEvent = false;
    if (auto spinBox = qobject_cast<QAbstractSpinBox*>(watched))
    {
        filterEvent = filterSpinBoxEvents(spinBox, event);
    }
    else if (auto lineEdit = qobject_cast<QLineEdit*>(watched))
    {
        filterEvent = filterLineEditEvents(lineEdit, event);
    }

    return filterEvent ? filterEvent : QObject::eventFilter(watched, event);
}

bool SpinBoxWatcher::filterSpinBoxEvents(QAbstractSpinBox* spinBox, QEvent* event)
{
    if (!spinBox || !spinBox->isEnabled())
    {
        return false;
    }

    bool filterEvent = false;
    switch (event->type())
    {
        case QEvent::MouseMove:
        {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if ((mouseEvent->buttons() & (Qt::LeftButton | Qt::MiddleButton)) && m_state == Dragging)
            {
                const int delta = mouseEvent->x() - m_xPos;
                if (qAbs(delta) <= qAbs(m_config.pixelsPerStep))
                {
                    break;
                }

                m_xPos = mouseEvent->x();
                int step = delta > 0 ? 1 : -1;
                if (mouseEvent->modifiers() & Qt::ShiftModifier)
                {
                    step *= 10;
                }

                if (step > 0)
                {
                    spinBox->setProperty(g_spinBoxValueIncreasingName, true);
                    spinBox->setProperty(g_spinBoxValueDecreasingName, false);
                    spinBox->setProperty(g_spinBoxScrollIncreasingName, true);
                    spinBox->setProperty(g_spinBoxScrollDecreasingName, false);
                }
                else
                {
                    spinBox->setProperty(g_spinBoxValueIncreasingName, false);
                    spinBox->setProperty(g_spinBoxValueDecreasingName, true);
                    spinBox->setProperty(g_spinBoxScrollIncreasingName, false);
                    spinBox->setProperty(g_spinBoxScrollDecreasingName, true);
                }

                spinBox->stepBy(step);

                if (m_activeScreen)
                {
                    const QPoint hotspot = spinBox->cursor().hotSpot();
                    const QRect screenRect = m_activeScreen->geometry().adjusted(hotspot.x(), hotspot.y(), -hotspot.x(), -hotspot.y());
                    QPoint screenPos = mouseEvent->screenPos().toPoint();
                    const int xPos = screenPos.x();
                    int newXPos = xPos;
                    // cursor bounces on the left and right side of the screen
                    // looks like buggy behaviour so mouse cursor is wrapped
                    // around to the other side of the screen.
                    if (xPos >= screenRect.right())
                    {
                        // wraps mouse cursor around to the left side of the screen
                        newXPos = screenRect.left() + 1;
                    }
                    else if (xPos <= screenRect.left())
                    {
                        // wraps mouse cursor around to the right side of the screen
                        newXPos = screenRect.right() - 1;
                    }

                    if (newXPos != xPos)
                    {
                        // Update our local x position so we can continue to step when the mouse moves
                        const int screenDelta = xPos - newXPos;
                        m_xPos -= screenDelta;

                        // Move the mouse cursor away from the edge of the screen.
                        screenPos.setX(newXPos);
                        QCursor::setPos(m_activeScreen, screenPos);
                    }
                }
            }
            setInitializeSpinboxValue(spinBox);
            break;
        }
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::HoverMove:
        {
            bool buttonUpPressed = spinBox->property(g_spinBoxUpPressedPropertyName).toBool();
            bool buttonDownPressed = spinBox->property(g_spinBoxDownPressedPropertyName).toBool();

            // We don't want the scroll icon to show up while clicking and dragging
            // on an arrow button
            if (!buttonUpPressed && !buttonDownPressed)
            {
                resetCursor(spinBox);
            }
            break;
        }

        case QEvent::Enter:
        case QEvent::Leave:
        {
            if (auto lineEdit = spinBox->findChild<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly))
            {
                // Recalculate the line edit geometry so that the value runs off the right edge
                // if the SpinBox buttons are not visible.
                QStyleOptionSpinBox styleOption;
                initStyleOption(spinBox, &styleOption);
                const auto newGeometry = spinBox->style()->subControlRect(QStyle::CC_SpinBox,
                                                                          &styleOption,
                                                                          QStyle::SC_SpinBoxEditField,
                                                                          spinBox);
                if (lineEdit->geometry() != newGeometry)
                {
                    lineEdit->setGeometry(newGeometry);
                }
            }

            // Have to account for Qt not setting this properly for spinboxes
            // We manually set a hovered property that can be matched against in the stylesheet
            spinBox->setProperty(g_hoveredPropertyName, bool(event->type() == QEvent::Enter));

            // Qt, for performance reasons, doesn't re-evaluate css rules when a dynamic property changes
            // so we have to force it to.
            spinBox->style()->unpolish(spinBox);
            spinBox->style()->polish(spinBox);

            break;
        }

        case QEvent::Paint:
        {
            // Update the mouse cursor. QPaintEvent is used because it occurs when the mouse
            // moves between sub-controls, which means we don't have to enable mouseTracking. We
            // can't do this in SpinBox::drawSpinBox because there we only have a const QWidget
            // pointer.

            if (m_state == Inactive)
            {
                const QPoint pos = spinBox->mapFromGlobal(QCursor::pos());
                if (spinBox->rect().contains(pos))
                {
                    resetCursor(spinBox);
                }
            }
            break;
        }

        case QEvent::FocusIn:
        {
            spinBox->setProperty(g_spinBoxFocusedName, true);
            if (m_config.autoSelectAllOnClickFocus)
            {
                QFocusEvent* fe = static_cast<QFocusEvent*>(event);
                if (fe->reason() == Qt::MouseFocusReason)
                {
                    m_mouseFocusedSpinBox = spinBox;
                }
            }
            break;
        }

        case QEvent::FocusOut:
        {
            // Checks whether valueChangeBegun has been emitted and emits valueChangeEnded if
            // required. This handles the case where a new value has been typed without pressing
            // return or enter, or after a wheel event.
            emitValueChangeEnded(spinBox);
            spinBox->setProperty(g_spinBoxFocusedName, false);
            break;
        }

        case QEvent::KeyPress:
        {
            auto keyEvent = static_cast<QKeyEvent*>(event);

            // Handle up/down arrows
            bool up = false;
            switch (keyEvent->key())
            {
                case Qt::Key_Up:
                    up = true;
                    // Fall through

                case Qt::Key_Down:
                    if (!keyEvent->isAutoRepeat())
                    {
                        emitValueChangeBegan(spinBox);
                    }

                    if (up)
                    {
                        spinBox->setProperty(g_spinBoxValueIncreasingName, true);
                        spinBox->setProperty(g_spinBoxValueDecreasingName, false);
                    }
                    else
                    {
                        spinBox->setProperty(g_spinBoxValueIncreasingName, false);
                        spinBox->setProperty(g_spinBoxValueDecreasingName, true);
                    }
                    break;

                case Qt::Key_Escape:
                    if (!spinBox->keyboardTracking())
                    {
                        // If we're not keyboard tracking, then changes to the text field
                        // aren't 'committed' until the user hits Enter, Return, tabs out of
                        // the field, or the spinbox is hidden.
                        // We want to stop the commit from happening if the user hits escape.
                        if (spinBox->hasFocus())
                        {
                            // Big logic jump here; if the current value has been committed already,
                            // then everything listening on the signals knows about it already.
                            // If the current value has NOT been committed, nothing knows about it
                            // but we don't want to trigger any further updates, because we're resetting
                            // it.
                            // So we disable signals here
                            if (auto azSpinBox = qobject_cast<SpinBox*>(spinBox))
                            {
                                QSignalBlocker blocker(azSpinBox);
                                azSpinBox->setValue(azSpinBox->m_lastValue);
                            }
                            else if (auto azDoubleSpinBox = qobject_cast<DoubleSpinBox*>(spinBox))
                            {
                                QSignalBlocker blocker(azDoubleSpinBox);
                                azDoubleSpinBox->setValue(azDoubleSpinBox->m_lastValue);
                            }

                            spinBox->selectAll();
                        }
                    }
                    break;
            }

            // Handle digits
            const bool isDigit = (keyEvent->key() >= Qt::Key_0) && (keyEvent->key() <= Qt::Key_9);
            const bool isForFloatingPointDecimal = (qobject_cast<QDoubleSpinBox*>(spinBox) && (keyEvent->key() == Qt::Key_Period));
            if (isDigit || keyEvent->key() == Qt::Key_Backspace || isForFloatingPointDecimal)
            {
                emitValueChangeBegan(spinBox);
                // emitValueChangeEnded is called when Qt::Key_Return or Qt::Key_Enter is released,
                // or in QEvent::FocusOut.
            }
            setInitializeSpinboxValue(spinBox, true);

            break;
        }

        case QEvent::KeyRelease:
        {
            auto keyEvent = static_cast<QKeyEvent*>(event);
            if (!m_spinBoxChanging.isNull() && !keyEvent->isAutoRepeat())
            {
                switch (keyEvent->key())
                {
                    case Qt::Key_Up:
                    case Qt::Key_Down:
                    case Qt::Key_Return:
                    case Qt::Key_Enter:
                        emitValueChangeEnded(spinBox);
                        spinBox->setProperty(g_spinBoxValueDecreasingName, false);
                        spinBox->setProperty(g_spinBoxValueIncreasingName, false);
                        spinBox->update();
                        break;
                }
            }
            break;
        }
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonPress:
        {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                QStyleOptionSpinBox styleOption;
                initStyleOption(spinBox, &styleOption);
                const auto control = spinBox->style()->hitTestComplexControl(QStyle::CC_SpinBox, &styleOption, mouseEvent->pos(), spinBox);
                const auto enabledSteps = stepEnabled(spinBox);

                const bool buttonUpPressed = (control == QStyle::SC_SpinBoxUp && (enabledSteps & QSpinBox::StepUpEnabled));
                const bool buttonDownPressed = (control == QStyle::SC_SpinBoxDown && (enabledSteps & QSpinBox::StepDownEnabled));

                if (buttonUpPressed || buttonDownPressed)
                {
                    spinBox->setProperty(g_spinBoxUpPressedPropertyName, buttonUpPressed);
                    spinBox->setProperty(g_spinBoxDownPressedPropertyName, buttonDownPressed);
                    m_state = ProcessingArrowButtons;
                }
                else
                {
                    m_state = Dragging;
                }
            }
            else if (mouseEvent->button() == Qt::MiddleButton)
            {
                m_state = Dragging;
            }

            if(m_state == Dragging) 
            {
                m_activeScreen = QGuiApplication::screenAt(mouseEvent->globalPos());
                m_xPos = mouseEvent->x();
                spinBox->grabMouse();
            }
            spinBox->setProperty(g_spinBoxDraggingName, m_state == Dragging);
            emitValueChangeBegan(spinBox);
            break;
        }
        case QEvent::MouseButtonRelease:
        {
            emitValueChangeEnded(spinBox);
            m_state = Inactive;
            spinBox->setProperty(g_spinBoxDraggingName, false);
            spinBox->setProperty(g_spinBoxUpPressedPropertyName, false);
            spinBox->setProperty(g_spinBoxDownPressedPropertyName, false);
            spinBox->setProperty(g_spinBoxValueDecreasingName, false);
            spinBox->setProperty(g_spinBoxValueIncreasingName, false);
            resetCursor(spinBox);
            spinBox->releaseMouse();
            spinBox->update();
            break;
        }

        case QEvent::Wheel:
        {
            if (!spinBox->hasFocus())
            {
                // To prevent the event being turned into a focus event, be sure to install an
                // AzQtComponents::GlobalEventFilter on your QApplication instance.
                event->accept();
                return true;
            }

            auto wheelEvent = static_cast<QWheelEvent*>(event);

            // Emulates Qt::ScrollEnd
            if (wheelEvent->source() != Qt::MouseEventSynthesizedBySystem)
            {
                QTimer* timer = SpinBoxScrollTimer(spinBox);
                if (!timer)
                {
                    timer = new QTimer(spinBox);
                    timer->setObjectName(g_spinBoxScrollTimerName);
                    timer->setSingleShot(true);
                    timer->setInterval(100);
                    timer->callOnTimeout([this, timer, spinBox]() {
                        emitValueChangeEnded(spinBox);
                        timer->setParent(nullptr);
                        timer->deleteLater();
                    });
                }

                timer->start();
            }

            setInitializeSpinboxValue(spinBox, true);

            // Qt::ScrollEnd is only supported on macOS and only applies to trackpads, not scroll
            // wheels.
            if (wheelEvent->phase() == Qt::ScrollEnd)
            {
                emitValueChangeEnded(spinBox);
                break;
            }

            const auto angleDelta = wheelEvent->angleDelta();
            if (angleDelta.isNull() || !m_spinBoxChanging.isNull())
            {
                break;
            }
            else if (angleDelta.y() != 0)
            {
                const auto enabledSteps = stepEnabled(spinBox);
                if ((angleDelta.y() < 0 && (enabledSteps & QSpinBox::StepDownEnabled)) ||
                    (angleDelta.y() > 0 && (enabledSteps & QSpinBox::StepUpEnabled)))
                {
                    emitValueChangeBegan(spinBox);
                    // emitValueChangeEnded is called in QEvent::FocusOut
                    if (angleDelta.y() < 0)
                    {
                        spinBox->setProperty(g_spinBoxValueDecreasingName, true);
                        spinBox->setProperty(g_spinBoxValueIncreasingName, false);
                    }
                    else
                    {
                        spinBox->setProperty(g_spinBoxValueDecreasingName, false);
                        spinBox->setProperty(g_spinBoxValueIncreasingName, true);
                    }
                }
            }
            break;
        }

        case QEvent::DynamicPropertyChange:
        {
            auto styleSheet = StyleManager::styleSheetStyle(spinBox);
            styleSheet->repolish(spinBox);
            break;
        }

        case QEvent::ShortcutOverride:
        {
            // This should be handled in the base class, but since that's part of Qt, do it here.
            // The Up and Down keys have a function while this widget is in focus, so prevent those shortcuts from firing
            auto keyEvent = static_cast<QKeyEvent*>(event);
            switch (keyEvent->key())
            {
            case Qt::Key_Up:
            case Qt::Key_Down:
                event->accept();
                return true;
                break;

            default:
                break;
            }
            break;
        }
    }

    return filterEvent;
}

bool SpinBoxWatcher::filterLineEditEvents(QLineEdit* lineEdit, QEvent* event)
{
    if (!lineEdit || !lineEdit->isEnabled())
    {
        return false;
    }

    auto spinBox = qobject_cast<QAbstractSpinBox*>(lineEdit->parent());
    if (!spinBox || !spinBox->isEnabled())
    {
        return false;
    }

    bool filterEvent = false;
    switch (event->type())
    {
        case QEvent::MouseButtonPress:
        {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() & Qt::MiddleButton)
            {
                filterSpinBoxEvents(spinBox, event);
                // Update cursor without waiting for an Hover event
                resetCursor(spinBox);
            }
            else if (m_config.autoSelectAllOnClickFocus)
            {
                if (mouseEvent->button() == Qt::LeftButton && m_mouseFocusedSpinBox == spinBox)
                {
                    lineEdit->selectAll();
                    m_mouseFocusedSpinBox = nullptr;
                    m_mouseFocusedSpinBoxSingleClicked = spinBox;
                    return true;
                }
                m_mouseFocusedSpinBox = nullptr;
            }
            break;
        }
        case QEvent::MouseMove:
        {
            if (m_state == Dragging)
            {
                filterSpinBoxEvents(spinBox, event);
            }
            break;
        }
        case QEvent::MouseButtonRelease:
        {
            if (m_state == Dragging)
            {
                filterSpinBoxEvents(spinBox, event);
            }
            break;
        }
        case QEvent::Paint:
        {
            if (lineEdit->property(g_spinBoxIntializedValueName).isValid() && !lineEdit->property(g_spinBoxIntializedValueName).toBool()) {
                return true;
            }
            break;
        }
        case QEvent::MouseButtonDblClick:
        {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (m_config.autoSelectAllOnClickFocus)
            {
                if (mouseEvent->button() == Qt::LeftButton && m_mouseFocusedSpinBoxSingleClicked == spinBox)
                {
                    // fake a single click event to place the mouse button
                    QMouseEvent fake(QEvent::MouseButtonPress, mouseEvent->localPos(), mouseEvent->button(), mouseEvent->buttons(), mouseEvent->modifiers());
                    QCoreApplication::sendEvent(lineEdit, &fake);
                    m_mouseFocusedSpinBoxSingleClicked = nullptr;
                    return true;
                }
                m_mouseFocusedSpinBoxSingleClicked = nullptr;
            }
            // Select whole number only on double click
            if ((mouseEvent->button() & Qt::LeftButton) && qobject_cast<QDoubleSpinBox*>(spinBox))
            {
                filterEvent = true;
                mouseEvent->accept();
                lineEdit->selectAll();
            }
            break;
        }
    }

    return filterEvent;
}

void SpinBoxWatcher::setInitializeSpinboxValue(QAbstractSpinBox* spinBox, bool clearText)
{
    if (auto lineEdit = spinBox->findChild<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly)) {
        if (lineEdit->property(g_spinBoxIntializedValueName).isValid() && !lineEdit->property(g_spinBoxIntializedValueName).toBool()) {
            lineEdit->setProperty(g_spinBoxIntializedValueName, true);
            if (clearText) {
                lineEdit->setText(QString());
            }
        }
    }
}

QTimer* SpinBoxWatcher::SpinBoxScrollTimer(QAbstractSpinBox* spinBox)
{
    QTimer* timer = nullptr;

    if (spinBox)
    {
        timer = spinBox->findChild<QTimer*>(g_spinBoxScrollTimerName, Qt::FindDirectChildrenOnly);
    }

    return timer;
}

void SpinBoxWatcher::initStyleOption(QAbstractSpinBox* spinBox, QStyleOptionSpinBox* option)
{
    Q_ASSERT(spinBox);
    Q_ASSERT(option);

    if (qobject_cast<QSpinBox*>(spinBox) || qobject_cast<QDoubleSpinBox*>(spinBox))
    {
        // QAbstractSpinBox::initStyleOption is protected so we have to replicate that logic here instead of using it directly.
        enum QSpinBoxPrivateButton {
            None = 0x000,
            Keyboard = 0x001,
            Mouse = 0x002,
            Wheel = 0x004,
            ButtonMask = 0x008,
            Up = 0x010,
            Down = 0x020,
            DirectionMask = 0x040
        };

        option->initFrom(spinBox);
        option->activeSubControls = QStyle::SC_None;
        option->buttonSymbols = spinBox->buttonSymbols();
        option->subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField;

        bool buttonUpPressed = spinBox->property(g_spinBoxUpPressedPropertyName).toBool();
        bool buttonDownPressed = spinBox->property(g_spinBoxDownPressedPropertyName).toBool();

        if (option->buttonSymbols != QAbstractSpinBox::NoButtons)
        {
            option->subControls |= QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;

            if (buttonUpPressed)
            {
                option->activeSubControls = QStyle::SC_SpinBoxUp;
            }
            else if (buttonDownPressed)
            {
                option->activeSubControls = QStyle::SC_SpinBoxDown;
            }
        }

        if (buttonUpPressed || buttonDownPressed)
    {
            option->state |= QStyle::State_Sunken;
    }
        else
    {
            option->activeSubControls = spinBox->property(g_hoverControlPropertyName).value<QStyle::SubControl>();
        }

        option->stepEnabled = spinBox->style()->styleHint(QStyle::SH_SpinControls_DisableOnBounds)
            ? stepEnabled(spinBox)
            : (QAbstractSpinBox::StepDownEnabled | QAbstractSpinBox::StepUpEnabled);

        option->frame = spinBox->hasFrame();
    }
    else
    {
        // Initialize it as best we can
        option->initFrom(spinBox);
        option->frame = spinBox->hasFrame();
        option->subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField;
        if (!(spinBox->buttonSymbols() & QAbstractSpinBox::NoButtons))
        {
            option->subControls |= QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;
        }
    }
}

QAbstractSpinBox::StepEnabled SpinBoxWatcher::stepEnabled(QAbstractSpinBox* spinBox)
{
    Q_ASSERT(spinBox);

    // QAbstractSpinBox::stepEnabled is protected; we can access it in the DoubleSpinBox and SpinBox objects, but not for anything else.

    QAbstractSpinBox::StepEnabled stepEnabled = QAbstractSpinBox::StepNone;
    if (auto azSpinBox = qobject_cast<SpinBox*>(spinBox))
    {
        stepEnabled = azSpinBox->stepEnabled();
    }
    else if (auto azDoubleSpinBox = qobject_cast<DoubleSpinBox*>(spinBox))
    {
        stepEnabled = azDoubleSpinBox->stepEnabled();
    }
    else
    {
        if (spinBox->isReadOnly())
        {
            return QAbstractSpinBox::StepNone;
        }

        // Assume we can step
        stepEnabled |= QAbstractSpinBox::StepUpEnabled;
        stepEnabled |= QAbstractSpinBox::StepDownEnabled;
    }

    spinBox->setProperty(g_spinBoxMinReachedName, !(stepEnabled & QAbstractSpinBox::StepDownEnabled));
    spinBox->setProperty(g_spinBoxMaxReachedName, !(stepEnabled & QAbstractSpinBox::StepUpEnabled));

    return stepEnabled;
}

void SpinBoxWatcher::emitValueChangeBegan(QAbstractSpinBox* spinBox)
{
    if (!m_spinBoxChanging.isNull())
    {
        Q_ASSERT(m_spinBoxChanging == spinBox);
        return;
    }

    if (auto azSpinBox = qobject_cast<SpinBox*>(spinBox))
    {
        m_spinBoxChanging = spinBox;
        emit azSpinBox->valueChangeBegan();
    }
    else if (auto azDoubleSpinBox = qobject_cast<DoubleSpinBox*>(spinBox))
    {
        m_spinBoxChanging = spinBox;
        emit azDoubleSpinBox->valueChangeBegan();
    }
}

void SpinBoxWatcher::emitValueChangeEnded(QAbstractSpinBox* spinBox)
{
    if (m_spinBoxChanging.isNull())
    {
        return;
    }

    if (spinBox)
    {
        spinBox->setProperty(g_spinBoxValueIncreasingName, false);
        spinBox->setProperty(g_spinBoxValueDecreasingName, false);
        spinBox->setProperty(g_spinBoxScrollIncreasingName, false);
        spinBox->setProperty(g_spinBoxScrollDecreasingName, false);
    }

    if (auto azSpinBox = qobject_cast<SpinBox*>(spinBox))
    {
        emit azSpinBox->valueChangeEnded();
        m_spinBoxChanging.clear();
    }
    else if (auto azDoubleSpinBox = qobject_cast<DoubleSpinBox*>(spinBox))
    {
        emit azDoubleSpinBox->valueChangeEnded();
        m_spinBoxChanging.clear();
    }
}

void SpinBoxWatcher::resetCursor(QAbstractSpinBox* spinBox)
{
    QStyleOptionSpinBox styleOption;
    initStyleOption(spinBox, &styleOption);
    const auto pos = spinBox->mapFromGlobal(QCursor::pos());
    const QStyle::SubControl control = spinBox->style()->hitTestComplexControl(QStyle::CC_SpinBox,
        &styleOption,
        pos,
        spinBox);

    if (((control == QStyle::SC_SpinBoxUp) || (control == QStyle::SC_SpinBoxDown)) && m_state != Dragging)
    {
        spinBox->setProperty(g_hoverControlPropertyName, control);
        spinBox->setCursor(Qt::ArrowCursor);
    }
    else
    {
        const auto enabledSteps = stepEnabled(spinBox);
        if (spinBox->property(g_spinBoxScrollIncreasingName).toBool())
        {
            if (enabledSteps & QSpinBox::StepUpEnabled)
            {
                spinBox->setCursor(m_config.scrollCursorRight);
            }
            else
            {
                spinBox->setCursor(m_config.scrollCursorRightMax);
            }
        }
        else if (spinBox->property(g_spinBoxScrollDecreasingName).toBool())
        {
            if (enabledSteps & QSpinBox::StepDownEnabled)
            {
                spinBox->setCursor(m_config.scrollCursorLeft);
            }
            else
            {
                spinBox->setCursor(m_config.scrollCursorLeftMax);
            }
        }
        else
        {
            spinBox->setCursor(m_config.scrollCursor);
        }

        // Need to update lineEdit cursor in case we started dragging using the
        // middle button on top of it
        if (auto lineEdit = spinBox->findChild<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly))
        {
            if (m_state == Dragging)
            {
                lineEdit->setCursor(spinBox->cursor());
            }
            else
            {
                lineEdit->setCursor(Qt::IBeamCursor);
            }
        }
        spinBox->setProperty(g_hoverControlPropertyName, QStyle::SC_None);
    }
}

SpinBox::Config SpinBox::loadConfig(QSettings& settings)
{
    Config config = defaultConfig();

    ConfigHelpers::read<int>(settings, QStringLiteral("PixelsPerStep"), config.pixelsPerStep);
    ConfigHelpers::read<QCursor>(settings, QStringLiteral("ScrollCursor"), config.scrollCursor);
    ConfigHelpers::read<QCursor>(settings, QStringLiteral("ScrollCursorLeft"), config.scrollCursorLeft);
    ConfigHelpers::read<QCursor>(settings, QStringLiteral("ScrollCursorLeftMax"), config.scrollCursorLeftMax);
    ConfigHelpers::read<QCursor>(settings, QStringLiteral("ScrollCursorRight"), config.scrollCursorRight);
    ConfigHelpers::read<QCursor>(settings, QStringLiteral("ScrollCursorRightMax"), config.scrollCursorRightMax);
    ConfigHelpers::read<int>(settings, QStringLiteral("LabelSize"), config.labelSize);
    ConfigHelpers::read<bool>(settings, QStringLiteral("AutoSelectAllOnClickFocus"), config.autoSelectAllOnClickFocus);

    return config;
}

SpinBox::Config SpinBox::defaultConfig()
{
    Config config;
    config.pixelsPerStep = 10;
    config.scrollCursor = QCursor(QPixmap(":/SpinBox/scrollIconDefault.svg"));
    config.scrollCursorLeft = QCursor(QPixmap(":/SpinBox/scrollIconLeft.svg"));
    config.scrollCursorLeftMax = QCursor(QPixmap(":/SpinBox/scrollIconLeftMaxed.svg"));
    config.scrollCursorRight = QCursor(QPixmap(":/SpinBox/scrollIconRight.svg"));
    config.scrollCursorRightMax = QCursor(QPixmap(":/SpinBox/scrollIconRightMaxed.svg"));
    config.labelSize = 16;
    config.autoSelectAllOnClickFocus = true;

    return config;
}

void SpinBox::setHasError(QAbstractSpinBox* spinbox, bool hasError)
{
    if (hasError != spinbox->property(HasError).toBool())
    {
        spinbox->setProperty(HasError, hasError);
    }
}

QPointer<SpinBoxWatcher> SpinBox::s_spinBoxWatcher = nullptr;
unsigned int SpinBox::s_watcherReferenceCount = 0;

void SpinBox::initializeWatcher()
{
    if (!s_spinBoxWatcher)
    {
        Q_ASSERT(s_watcherReferenceCount == 0);
        s_spinBoxWatcher = new SpinBoxWatcher;
    }

    ++s_watcherReferenceCount;
}

void SpinBox::uninitializeWatcher()
{
    Q_ASSERT(!s_spinBoxWatcher.isNull());
    Q_ASSERT(s_watcherReferenceCount > 0);

    --s_watcherReferenceCount;

    if (s_watcherReferenceCount == 0)
    {
        delete s_spinBoxWatcher;
        s_spinBoxWatcher = nullptr;
    }
}

bool SpinBox::drawSpinBox(const QProxyStyle* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const SpinBox::Config& config)
{
    Q_UNUSED(config);

    if (const auto styleOption = qstyleoption_cast<const QStyleOptionSpinBox*>(option))
    {
        // Only draw the up and down controls if the spinbox is enabled and has the mouse over it
        if (!(styleOption->state & QStyle::State_Enabled) || !(styleOption->state & QStyle::State_MouseOver))
        {
            QStyleOptionSpinBox copy = *styleOption;
            copy.subControls &= ~QStyle::SC_SpinBoxUp;
            copy.subControls &= ~QStyle::SC_SpinBoxDown;
            style->baseStyle()->drawComplexControl(QStyle::CC_SpinBox, &copy, painter, widget);
            return true;
        }
        else if (widget->property(g_spinBoxValueIncreasingName).toBool())
        {
            QStyleOptionSpinBox copy = *styleOption;
            copy.activeSubControls = QStyle::SC_SpinBoxUp;
            copy.state |= QStyle::State_Sunken;
            style->baseStyle()->drawComplexControl(QStyle::CC_SpinBox, &copy, painter, widget);
            return true;
        }
        else if (widget->property(g_spinBoxValueDecreasingName).toBool())
        {
            QStyleOptionSpinBox copy = *styleOption;
            copy.activeSubControls = QStyle::SC_SpinBoxDown;
            copy.state |= QStyle::State_Sunken;
            style->baseStyle()->drawComplexControl(QStyle::CC_SpinBox, &copy, painter, widget);
            return true;
        }
    }
    return false;
}

QRect SpinBox::editFieldRect(const QProxyStyle* style, const QStyleOptionComplex* option, const QWidget* widget, const Config& config)
{
    Q_UNUSED(config);

    if (auto spinBoxOption = qstyleoption_cast<const QStyleOptionSpinBox *>(option))
    {
        // Long values should run off the right side of the spinbox, so extend the SC_SpinBoxEditField
        // to the edge of the spinbox contents but don't overlap with the buttons
        const auto baseRect = style->baseStyle()->subControlRect(QStyle::CC_SpinBox,
                                                                 option,
                                                                 QStyle::SC_SpinBoxEditField,
                                                                 widget);
        const auto buttonRect = style->baseStyle()->subControlRect(QStyle::CC_SpinBox,
                                                                   option,
                                                                   QStyle::SC_SpinBoxUp,
                                                                   widget);
        QRect rect(baseRect);
        rect.setRight(buttonRect.x());
        rect.adjust(0, 0, -1, 0); // Don't overlap buttonRect or the spinbox border

        if (!(spinBoxOption->state & QStyle::State_Enabled) ||
            !(spinBoxOption->state & QStyle::State_MouseOver))
        {
            // The buttons aren't visible, so extend the QLineEdit to the inside of the spinbox border
            rect.adjust(0, 0, buttonRect.width(), 0);
        }
        else
        {
            // When the buttons are visible, we need remove an extra button width since the buttons are now
            // placed horizontally.
            rect.adjust(0, 0, -buttonRect.width(), 0);
        }
        return rect;
    }
    return QRect();
}

bool SpinBox::polish(QProxyStyle* style, QWidget* widget, const SpinBox::Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(config);
    Q_ASSERT(!s_spinBoxWatcher.isNull());

    if (!qobject_cast<QSpinBox*>(widget) && !qobject_cast<QDoubleSpinBox*>(widget))
    {
        // QDateTime inherits from QAbstractSpinBox and shouldn't be included here.
        return false;
    }

    if (auto spinBox = qobject_cast<QAbstractSpinBox*>(widget))
    {
        Style* newStyle = qobject_cast<Style*>(style);
        if (newStyle)
        {
            newStyle->repolishOnSettingsChange(spinBox);
        }

        s_spinBoxWatcher->m_config = config;
        // Initialize spinBox properties, and do it only once
        if (!spinBox->property(g_spinBoxInitPropertyFlagName).toBool())
        {
            spinBox->setProperty(g_spinBoxUpPressedPropertyName, false);
            spinBox->setProperty(g_spinBoxDownPressedPropertyName, false);
            spinBox->setProperty(g_spinBoxValueIncreasingName, false);
            spinBox->setProperty(g_spinBoxValueDecreasingName, false);
            spinBox->setProperty(g_spinBoxScrollIncreasingName, false);
            spinBox->setProperty(g_spinBoxScrollDecreasingName, false);
            spinBox->setProperty(g_spinBoxIntializedValueName, false);
            spinBox->setProperty(g_spinBoxMinReachedName, false);
            spinBox->setProperty(g_spinBoxMaxReachedName, false);
            spinBox->setProperty(g_spinBoxFocusedName, false);
            spinBox->setProperty(g_spinBoxDraggingName, false);
            spinBox->setProperty(g_spinBoxInitPropertyFlagName, true);
        }
        spinBox->installEventFilter(s_spinBoxWatcher);
        SpinBox::setButtonSymbolsForStyle(spinBox);

        if (auto lineEdit = spinBox->findChild<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly))
        {
            lineEdit->installEventFilter(s_spinBoxWatcher);
            LineEdit::setErrorIconEnabled(lineEdit, false);
        }
        return true;
    }
    return false;
}

bool SpinBox::unpolish(QProxyStyle* style, QWidget* widget, const SpinBox::Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(config);

    Q_ASSERT(!s_spinBoxWatcher.isNull());

    if (!qobject_cast<QSpinBox*>(widget) && !qobject_cast<QDoubleSpinBox*>(widget))
    {
        // QDateTime inherits from QAbstractSpinBox and shouldn't be included here.
        return false;
    }

    if (auto spinBox = qobject_cast<QAbstractSpinBox*>(widget))
    {
        spinBox->removeEventFilter(s_spinBoxWatcher);
        SpinBox::setButtonSymbolsForStyle(spinBox);

        if (auto lineEdit = spinBox->findChild<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly))
        {
            lineEdit->removeEventFilter(s_spinBoxWatcher);
        }
        return true;
    }
    return false;
}

void SpinBox::setButtonSymbolsForStyle(QAbstractSpinBox* spinBox)
{
    if (!spinBox)
    {
        return;
    }

    spinBox->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
}

SpinBox::SpinBox(QWidget* parent)
    : QSpinBox(parent)
{
#if !defined(AZ_PLATFORM_LINUX)
#if QT_VERSION < QT_VERSION_CHECK(5,11,1)
    setShiftIncreasesStepRate(true);
#endif
#endif //!defined(AZ_PLATFORM_LINUX)

    m_lineEdit = new internal::SpinBoxLineEdit(this);
    connect(m_lineEdit, &internal::SpinBoxLineEdit::globalUndoTriggered, this, &SpinBox::globalUndoTriggered);
    connect(m_lineEdit, &internal::SpinBoxLineEdit::globalRedoTriggered, this, &SpinBox::globalRedoTriggered);
    connect(this, &SpinBox::cutTriggered, this, [this]()
    {
        setInitialValueWasSetting(true);
        m_lineEdit->cut();
    });
    connect(this, &SpinBox::copyTriggered, this, [this]()
    {
        m_lineEdit->copy();
    });
    connect(this, &SpinBox::pasteTriggered, this, [this]()
    {
        setInitialValueWasSetting(true);
        m_lineEdit->setText(QString());
        m_lineEdit->paste();
    });
    connect(this, &SpinBox::deleteTriggered, this, [this]()
    {
        setInitialValueWasSetting(true);
        m_lineEdit->del();
    });
    connect(this, QOverload<int>::of(&SpinBox::valueChanged), this, &SpinBox::setLastValue);
    setLineEdit(m_lineEdit);
    setButtonSymbolsForStyle(this);

    setRange(0, 100);
}

void SpinBox::setValue(int value)
{
    QSpinBox::setValue(value);
    setLastValue(value);
}

QSize SpinBox::minimumSizeHint() const
{
    //  This prevents the range from affecting the size, allowing the use of user-defined size hints.
    QSize size = QSpinBox::sizeHint();
    size.setWidth(minimumWidth());
    return size;
}

bool SpinBox::isUndoAvailable() const
{
    return lineEdit()->isUndoAvailable();
}

bool SpinBox::isRedoAvailable() const
{
    return lineEdit()->isRedoAvailable();
}

void SpinBox::focusInEvent(QFocusEvent* event)
{
    // Remove the suffix while editing. Check focus reason so we don't clash
    // with context menus
    if (event->reason() != Qt::PopupFocusReason)
    {
        m_lastSuffix = suffix();
    }

    if (m_lastSuffix.length() > 0)
    {
        // Force the minimum size to the current size to prevent shrinkage when
        // the suffix is removed.
        m_lastMinimumSize = minimumSize();
        setMinimumSize(size());
        setSuffix(QString());
    }

    QSpinBox::focusInEvent(event);

    if (event->reason() == Qt::MouseFocusReason)
    {
        lineEdit()->deselect();
    }
}

void SpinBox::focusOutEvent(QFocusEvent* event)
{
    QSpinBox::focusOutEvent(event);

    // restore the suffix now, if needed
    if (event->reason() != Qt::PopupFocusReason && m_lastSuffix.length() > 0)
    {
        setSuffix(m_lastSuffix);
        // Restore the original minimum size
        setMinimumSize(m_lastMinimumSize);
        m_lastSuffix.clear();
        m_lastMinimumSize = {};
    }
}

static QAction* findAction(QList<QAction*>& actions, const QString& actionText)
{
    // would be much smarter to use QKeySequences to identify the actions we're
    // looking for, but QLineEdit doesn't actually use them in the same way
    // anything else does, so we can't
    for (QAction* action : actions)
    {
        if (action->text().contains(actionText))
        {
            return action;
        }
    }

    return nullptr;
}

template <typename SpinBoxType>
void spinBoxContextMenuEvent(QContextMenuEvent* ev, SpinBoxType* spinBox, QLineEdit* lineEdit, uint stepEnabled, bool overrideUndoRedo)
{
    // We want to override the context menu's undo/redo in the case that the lineEdit control does not currently have any undo/redo available
    // but the QAbstractSpinBox doesn't really expose any way to do that nicely.
    // As a result, we have to replicate the context menu code from QAbstractSpinBox instead,
    // and also pull out the undo and redo actions so that we can override them.

    if (QMenu* menu = lineEdit->createStandardContextMenu())
    {
        auto menuActions = menu->actions();

        QAction* undoAction = findAction(menuActions, QObject::tr("&Undo"));
        if (undoAction && overrideUndoRedo)
        {
            QObject::disconnect(undoAction, &QAction::triggered, nullptr, nullptr);
            QObject::connect(undoAction, &QAction::triggered, spinBox, &SpinBoxType::globalUndoTriggered);
        }

        QAction* redoAction = findAction(menuActions, QObject::tr("&Redo"));
        if (redoAction && overrideUndoRedo)
        {
            QObject::disconnect(redoAction, &QAction::triggered, nullptr, nullptr);
            QObject::connect(redoAction, &QAction::triggered, spinBox, &SpinBoxType::globalRedoTriggered);
        }

        QAction* selectAllAction = findAction(menuActions, QObject::tr("Select All"));
        if (selectAllAction)
        {
            QObject::disconnect(selectAllAction, &QAction::triggered, nullptr, nullptr);
            QObject::connect(selectAllAction, &QAction::triggered, spinBox, &QAbstractSpinBox::selectAll);
        }

        QAction* cutAction = findAction(menuActions, QObject::tr("Cu&t"));
        if (cutAction)
        {
            QObject::disconnect(cutAction, &QAction::triggered, nullptr, nullptr);
            QObject::connect(cutAction, &QAction::triggered, spinBox, &SpinBoxType::cutTriggered);
        }

        QAction* copyAction = findAction(menuActions, QObject::tr("&Copy"));
        if (copyAction)
        {
            QObject::disconnect(copyAction, &QAction::triggered, nullptr, nullptr);
            QObject::connect(copyAction, &QAction::triggered, spinBox, &SpinBoxType::copyTriggered);
        }

        QAction* pasteAction = findAction(menuActions, QObject::tr("&Paste"));
        if (pasteAction)
        {
            QObject::disconnect(pasteAction, &QAction::triggered, nullptr, nullptr);
            QObject::connect(pasteAction, &QAction::triggered, spinBox, &SpinBoxType::pasteTriggered);
        }

        QAction* deleteAction = findAction(menuActions, QObject::tr("Delete"));
        if (deleteAction)
        {
            QObject::disconnect(deleteAction, &QAction::triggered, nullptr, nullptr);
            QObject::connect(deleteAction, &QAction::triggered, spinBox, &SpinBoxType::deleteTriggered);
        }

        menu->addSeparator();

        QAction* upAction = menu->addAction(QObject::tr("&Step up"));
        upAction->setEnabled(stepEnabled & QAbstractSpinBox::StepUpEnabled);
        QObject::connect(upAction, &QAction::triggered, spinBox, [spinBox] {
            spinBox->setInitialValueWasSetting(true);
            spinBox->stepBy(1);
        });

        QAction* downAction = menu->addAction(QObject::tr("Step &down"));
        downAction->setEnabled(stepEnabled & QAbstractSpinBox::StepDownEnabled);
        QObject::connect(downAction, &QAction::triggered, spinBox, [spinBox] {
            spinBox->setInitialValueWasSetting(true);
            spinBox->stepBy(-1);
        });

        Q_EMIT spinBox->contextMenuAboutToShow(menu, undoAction, redoAction);

        const QPoint pos = (ev->reason() == QContextMenuEvent::Mouse)
            ? ev->globalPos() : spinBox->mapToGlobal(QPoint(ev->pos().x(), 0)) + QPoint(spinBox->width() / 2, spinBox->height() / 2);

        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->popup(pos);

        ev->accept();
    }
}

void SpinBox::setInitialValueWasSetting(bool b)
{
    lineEdit()->setProperty(g_spinBoxIntializedValueName, b);
}

void SpinBox::contextMenuEvent(QContextMenuEvent* ev)
{
    spinBoxContextMenuEvent(ev, this, lineEdit(), stepEnabled(), m_lineEdit->overrideUndoRedo());
}

void SpinBox::setLastValue(int v)
{
    m_lastValue = v;
}

DoubleSpinBox::DoubleSpinBox(QWidget* parent)
    : QDoubleSpinBox(parent)
    , m_displayDecimals(g_decimalDisplayPrecisionDefault)
{
#if !defined(AZ_PLATFORM_LINUX)
#if QT_VERSION < QT_VERSION_CHECK(5,11,1)
    setShiftIncreasesStepRate(true);
#endif
#endif // !defined(AZ_PLATFORM_LINUX)

    m_lineEdit = new internal::SpinBoxLineEdit(this);
    connect(m_lineEdit, &internal::SpinBoxLineEdit::globalUndoTriggered, this, &DoubleSpinBox::globalUndoTriggered);
    connect(m_lineEdit, &internal::SpinBoxLineEdit::globalRedoTriggered, this, &DoubleSpinBox::globalRedoTriggered);
    connect(this, &DoubleSpinBox::cutTriggered, this, [this]()
    {
        setInitialValueWasSetting(true);
        m_lineEdit->cut();
    });
    connect(this, &DoubleSpinBox::copyTriggered, this, [this]()
    {
        m_lineEdit->copy();
    });
    connect(this, &DoubleSpinBox::pasteTriggered, this, [this]()
    {
        setInitialValueWasSetting(true);
        m_lineEdit->setText(QString());
        m_lineEdit->paste();
    });
    connect(this, &DoubleSpinBox::deleteTriggered, this, [this]()
    {
        setInitialValueWasSetting(true);
        m_lineEdit->del();
    });

    setLineEdit(m_lineEdit);
    SpinBox::setButtonSymbolsForStyle(this);

    setRange(0, 100);
    setDecimals(g_decimalPrecisonDefault);

    // Our tooltip will be the full decimal value, so keep it updated
    // whenever our value changes
    connect(this, QOverload<double>::of(&DoubleSpinBox::valueChanged), this, &DoubleSpinBox::updateToolTip);
    connect(this, QOverload<double>::of(&DoubleSpinBox::valueChanged), this, &DoubleSpinBox::setLastValue);
    updateToolTip(value());
}

void DoubleSpinBox::setValue(double value)
{
    QDoubleSpinBox::setValue(value);
    setLastValue(value);
    updateToolTip(value);
}

QSize DoubleSpinBox::minimumSizeHint() const
{
    // This prevents the range from affecting the size, allowing the use of user-defined size hints.
    QSize size = QDoubleSpinBox::sizeHint();
    size.setWidth(minimumWidth());
    return size;
}

bool DoubleSpinBox::isUndoAvailable() const
{
    return lineEdit()->isUndoAvailable();
}

bool DoubleSpinBox::isRedoAvailable() const
{
    return lineEdit()->isRedoAvailable();
}

bool DoubleSpinBox::isEditing() const
{
    return SpinBox::s_spinBoxWatcher->isEditing(this);
}

void DoubleSpinBox::contextMenuEvent(QContextMenuEvent* ev)
{
    spinBoxContextMenuEvent(ev, this, lineEdit(), stepEnabled(), m_lineEdit->overrideUndoRedo());
}

QString DoubleSpinBox::stringValue(double value, bool truncated) const
{
    // Determine which decimal precision to use for displaying the value
    int numDecimals = decimals();
    if (truncated && m_displayDecimals < numDecimals)
    {
        numDecimals = m_displayDecimals;
    }
    else if (value == std::floor(value) && (!m_options.testFlag(SHOW_ONE_DECIMAL_PLACE_ALWAYS)))
    {
        numDecimals = 0;
    }

    return toString(value, numDecimals, locale(), isGroupSeparatorShown(), true);
}

void DoubleSpinBox::updateToolTip(double value)
{
    // Set our tooltip to the full decimal value
    setToolTip(stringValue(value));
}

void DoubleSpinBox::setLastValue(double v)
{
    m_lastValue = v;
}

void DoubleSpinBox::setInitialValueWasSetting(bool b)
{
    lineEdit()->setProperty(g_spinBoxIntializedValueName, b);
}

QString DoubleSpinBox::textFromValue(double value) const
{
    // If our widget is focused, then show the full decimal value, otherwise
    // show the truncated value
    return stringValue(value, !hasFocus());
}

void DoubleSpinBox::focusInEvent(QFocusEvent* event)
{
    // We need to set the special value text to an empty string, which
    // effectively makes no change, but actually triggers the line edit
    // display value to be updated so that when we receive focus to
    // begin editing, we display the full decimal precision instead of
    // the truncated display value
    setSpecialValueText(QString());

    // Remove the suffix while editing. Check focus reason so we don't clash
    // with context menus
    if (event->reason() != Qt::PopupFocusReason)
    {
        m_lastSuffix = suffix();
    }

    if (m_lastSuffix.length() > 0)
    {
        // Force the minimum size to the current size to prevent shrinkage when
        // the suffix is removed.
        m_lastMinimumSize = minimumSize();
        setMinimumSize(size());
        setSuffix(QString());
    }

    QDoubleSpinBox::focusInEvent(event);

    if (event->reason() == Qt::MouseFocusReason)
    {
        lineEdit()->deselect();
    }
}

void DoubleSpinBox::focusOutEvent(QFocusEvent* event)
{
    QDoubleSpinBox::focusOutEvent(event);

    // restore the suffix now, if needed
    if (event->reason() != Qt::PopupFocusReason && m_lastSuffix.length() > 0)
    {
        setSuffix(m_lastSuffix);
        // Restore the original minimum size
        setMinimumSize(m_lastMinimumSize);
        m_lastSuffix.clear();
        m_lastMinimumSize = {};
    }
}

namespace internal
{

    SpinBoxLineEdit::SpinBoxLineEdit(QWidget* parent)
        : QLineEdit(parent)
    {
        setMouseTracking(true);
    }

    bool SpinBoxLineEdit::overrideUndoRedo() const
    {
        return !isUndoAvailable() && !isRedoAvailable() && !isReadOnly();
    }

    bool SpinBoxLineEdit::event(QEvent* ev)
    {
        switch (ev->type())
        {
            case QEvent::FocusOut:
            {
                // Explicitly set the text again on focusOut, so that the undo/redo queue for the line edit clears and the global undo/redo can kick in
                if (const auto focusEvent = static_cast<QFocusEvent*>(ev))
                {
                    if (focusEvent->reason() != Qt::PopupFocusReason)
                    {
                        setText(text());
                    }
                }
            }
            break;
        }

        return QLineEdit::event(ev);
    }

    void SpinBoxLineEdit::keyPressEvent(QKeyEvent* ev)
    {
        if (overrideUndoRedo())
        {
            // QLineEdit overrides the key press event handler
            // so we have to trap that too, directly, without assuming
            // that QAction's with shortcuts will do it.
            if (ev->matches(QKeySequence::Undo))
            {
                Q_EMIT globalUndoTriggered();
                ev->accept();
                return;
            }
            else if (ev->matches(QKeySequence::Redo))
            {
                Q_EMIT globalRedoTriggered();
                ev->accept();
                return;
            }
            else if (ev->matches(QKeySequence::SelectAll))
            {
                Q_EMIT selectAllTriggered();
                ev->accept();
                return;
            }
        }
        if (ev->matches(QKeySequence::Cut))
        {
            Q_EMIT cutTriggered();
        }
        if (ev->matches(QKeySequence::Copy))
        {
            Q_EMIT copyTriggered();
        }
        if (ev->matches(QKeySequence::Paste))
        {
            Q_EMIT pasteTriggered();
        }
        if (ev->matches(QKeySequence::Delete))
        {
            Q_EMIT deleteTriggered();
        }
        // fall through to the default
        QLineEdit::keyPressEvent(ev);
    }

    static void setLineEditTextFormat(QLineEdit* lineEdit, const QList<QTextLayout::FormatRange>& formats)
    {
        if (!lineEdit)
            return;

        QList<QInputMethodEvent::Attribute> attributes;
        foreach (const QTextLayout::FormatRange& fr, formats)
        {
            QInputMethodEvent::AttributeType type = QInputMethodEvent::TextFormat;
            int start = fr.start;
            int length = fr.length;
            QVariant value = fr.format;
            attributes.append(QInputMethodEvent::Attribute(type, start, length, value));
        }
        QInputMethodEvent event(QString(), attributes);
        QCoreApplication::sendEvent(lineEdit, &event);
    }

    static void clearLineEditTextFormat(QLineEdit* lineEdit)
    {
        setLineEditTextFormat(lineEdit, QList<QTextLayout::FormatRange>());
    }

    void SpinBoxLineEdit::paintEvent(QPaintEvent* event)
    {
        QAbstractSpinBox* abstractSpinBox = qobject_cast<QAbstractSpinBox*>(parent());
        QSpinBox* spinBox = reinterpret_cast<QSpinBox*>(abstractSpinBox);
        bool fixLeftAlignment = !spinBox->property(g_hoveredPropertyName).toBool() && !hasFocus();
        int suffixSize = spinBox->suffix().size();
        QString beforeText;
        int cursorPos = 0;
        if (fixLeftAlignment)
        {
            // Reset the cursorposition to ensure left-aligned values
            cursorPos = cursorPosition();
            setCursorPosition(0);
        }
        if (suffixSize)
        {
            QList<QTextLayout::FormatRange> formats;
            int size = text().size();

            QTextCharFormat f;
            f.setForeground(QColor(0x888888));

            QTextLayout::FormatRange fr_task;
            fr_task.start = size - suffixSize;
            fr_task.length = suffixSize;
            fr_task.format = f;

            formats.append(fr_task);

            setLineEditTextFormat(this, formats);
        }
        QLineEdit::paintEvent(event);
        if (suffixSize)
        {
            clearLineEditTextFormat(this);
        }
        if (fixLeftAlignment)
        {
            setCursorPosition(cursorPos);
        }
    }
} // namespace internal

} // namespace AzQtComponents

#include "Components/Widgets/moc_SpinBox.cpp"

