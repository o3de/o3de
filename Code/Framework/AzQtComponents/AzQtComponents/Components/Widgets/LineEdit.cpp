/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/ConfigHelpers.h>

#include <QLineEdit>
#include <QAction>
#include <QSettings>
#include <QDynamicPropertyChangeEvent>
#include <QHelpEvent>
#include <QToolTip>
#include <QPainter>
#include <QStyleOption>
#include <QToolButton>
#include <QTimer>
#include <QCoreApplication>

namespace AzQtComponents
{
    static char g_enableClearButtonWhenReadOnly[] = "EnableClearButtonWhenReadOnly";

    class LineEditWatcher : public QObject
    {
    public:
        explicit LineEditWatcher(QObject* parent = nullptr)
            : QObject(parent)
        {
        }

        bool eventFilter(QObject* watched, QEvent* event) override
        {
            if (auto le = qobject_cast<QLineEdit*>(watched))
            {
                switch (event->type())
                {
                    case QEvent::DynamicPropertyChange:
                    {
                        le->update();
                    }
                    break;

                    case QEvent::MouseButtonPress:
                    {
                        if (m_config.autoSelectAllOnClickFocus)
                        {
                            QMouseEvent* me = static_cast<QMouseEvent*>(event);
                            if (me->button() == Qt::LeftButton && m_mouseFocusedLineEdit == le && !le->isReadOnly())
                            {
                                le->selectAll();
                                m_mouseFocusedLineEdit = nullptr;
                                m_mouseFocusedLineEditSingleClicked = le;
                                return true;
                            }
                            m_mouseFocusedLineEdit = nullptr;
                        }
                        break;
                    }

                    case QEvent::MouseButtonDblClick:
                    {
                        if (m_config.autoSelectAllOnClickFocus)
                        {
                            QMouseEvent* me = static_cast<QMouseEvent*>(event);
                            if (me->button() == Qt::LeftButton && m_mouseFocusedLineEditSingleClicked == le)
                            {
                                // fake a single click event to place the mouse button
                                QMouseEvent fake(QEvent::MouseButtonPress, me->localPos(), me->button(), me->buttons(), me->modifiers());
                                QCoreApplication::sendEvent(le, &fake);
                                m_mouseFocusedLineEditSingleClicked = nullptr;
                                return true;
                            }
                            m_mouseFocusedLineEditSingleClicked = nullptr;
                        }
                        break;
                    }

                    case QEvent::FocusIn:
                    {
                        if (m_config.autoSelectAllOnClickFocus)
                        {
                            QFocusEvent* fe = static_cast<QFocusEvent*>(event);
                            if (fe->reason() == Qt::MouseFocusReason)
                            {
                                m_mouseFocusedLineEdit = le;
                            }
                        }
                        updateClearButtonState(le);
                    }
                    break;

                    case QEvent::FocusOut:
                    {
                        updateClearButtonState(le);
                    }
                    break;

                    case QEvent::ToolTip:
                    {
                        if (le->property(HasError).toBool())
                        {
                            const auto he = static_cast<QHelpEvent*>(event);
                            QString message = le->property(ErrorMessage).toString();
                            if (message.isEmpty())
                            {
                                message = QLineEdit::tr("Invalid input");
                            }
                            QToolTip::showText(he->globalPos(),
                                               message,
                                               le,
                                               QRect(),
                                               1000 *10);
                            return true;
                        }
                    }
                    break;

                    case QEvent::KeyPress:
                    {
                        const auto ke = static_cast<QKeyEvent*>(event);

                        if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return)
                        {
                            updateErrorState(le);
                        }
                    }
                    break;

                    case QEvent::LayoutDirectionChange:
                    case QEvent::Resize:
                    {
                        const auto filtered = QObject::eventFilter(watched, event);
                        if (!filtered)
                        {
                            // Deliver the event
                            le->event(event);
                            // Set our own side widget positions
                            positionSideWidgets(le);
                        }
                        return true;
                    }

                    case QEvent::Show:
                    {
                        positionSideWidgets(le);
                        break;
                    }

                }
            }

            return QObject::eventFilter(watched, event);
        }

    private:
        friend class LineEdit;
        LineEdit::Config m_config;

        QLineEdit* m_mouseFocusedLineEdit = nullptr;
        QLineEdit* m_mouseFocusedLineEditSingleClicked = nullptr;

        bool isError(QLineEdit* le) const
        {
            const bool hasError = !le->hasAcceptableInput() || le->property(HasExternalError).toBool();
            return hasError;
        }

        bool isClearButtonNeeded(QLineEdit* le) const
        {
            return !le->text().isEmpty();
        }

        void updateErrorState(QLineEdit* le)
        {
            const bool error = isError(le);

            if (error != le->property(HasError).toBool())
            {
                le->setProperty(HasError, error);
            }

            auto errorToolButton = le->findChild<QToolButton*>(ErrorToolButton);
            if (errorToolButton && LineEdit::errorIconEnabled(le))
            {
                errorToolButton->setVisible(error);
                positionSideWidgets(le);
            }
        }

        void updateClearButtonState(QLineEdit* le)
        {
            const bool clear = isClearButtonNeeded(le);

            // We don't want to impose the clear button visibility if user did not called
            // lineEdit->setClearButtonEnabled on its own.
            // So let get the clear action if available to update it only.
            QAction* clearAction = le->findChild<QAction*>(ClearAction);
            if (clearAction)
            {
                clearAction->setVisible(clear);
                if (clear && le->isReadOnly() && le->property(g_enableClearButtonWhenReadOnly).toBool())
                {
                    QToolButton* clearButton = LineEdit::getClearButton(le);
                    assert(clearButton);
                    clearButton->setEnabled(true);
                }
                positionSideWidgets(le);
            }
        }

        void updateErrorStateSlot()
        {
            if (auto le = qobject_cast<QLineEdit*>(sender()))
            {
                updateErrorState(le);
                positionSideWidgets(le);
            }
        }

        void updateClearButtonStateSlot()
        {
            if (auto le = qobject_cast<QLineEdit*>(sender()))
            {
                updateClearButtonState(le);
            }
        }

        void storeHoverAttributeState(QLineEdit* lineEdit)
        {
            if (!lineEdit || hasHoverAttributeState(lineEdit))
            {
                return;
            }

            lineEdit->setProperty(StoredHoverAttributeState, lineEdit->testAttribute(Qt::WA_Hover));
        }

        void restoreHoverAttributeState(QLineEdit* lineEdit)
        {
            if (!lineEdit || !hasHoverAttributeState(lineEdit))
            {
                return;
            }

            // Delay property change until the next event loop iteration. This prevents
            // widgets from being destroyed (and pointers invalidated) whilst the app is
            // being repolished.
            QTimer::singleShot(0, lineEdit, [lineEdit] {
                const auto hovereState = lineEdit->property(StoredHoverAttributeState);
                lineEdit->setAttribute(Qt::WA_Hover, hovereState.toBool());
            });
        }

        bool hasHoverAttributeState(QLineEdit* lineEdit)
        {
            if (!lineEdit)
            {
                return false;
            }

            const auto hoverState = lineEdit->property(StoredHoverAttributeState);
            return hoverState.isValid() && hoverState.canConvert<bool>();
        }

        void positionSideWidgets(QLineEdit* lineEdit)
        {
            const auto contentsRect = lineEdit->rect();
            int delta = m_config.iconMargin;

            QToolButton* clearToolButton = LineEdit::getClearButton(lineEdit); // May be nullptr

            auto errorToolButton = lineEdit->findChild<QToolButton*>(ErrorToolButton);
            const QList<QToolButton*> rightButtons = {clearToolButton, errorToolButton};

            // Position the buttons on the right hand side horizontally
            for (auto button : rightButtons)
            {
                if (button && button->isVisible())
                {
                    QRect geometry = button->geometry();
                    geometry.moveRight(contentsRect.right() - delta);
                    button->setGeometry(geometry);
                    delta += geometry.width() + m_config.iconSpacing;
                }
            }

            // Position all buttons vertically
            const auto childToolButtons = lineEdit->findChildren<QToolButton*>();
            for (auto button : childToolButtons)
            {
                QRect geometry = button->geometry();
                geometry.moveTop((contentsRect.height() - geometry.height()) / 2);
                button->setGeometry(geometry);
            }
        }
    };

    QPointer<LineEditWatcher> LineEdit::s_lineEditWatcher = nullptr;
    unsigned int LineEdit::s_watcherReferenceCount = 0;

    int LineEdit::Config::getLineWidth(const QStyleOption* option, bool hasError, bool dropTarget) const
    {
        const QStyleOptionFrame* frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option);
        if (!frameOption)
        {
            return 0;
        }

        const bool isFocused = option->state.testFlag(QStyle::State_HasFocus);
        const bool isHovered = option->state.testFlag(QStyle::State_MouseOver) && option->state.testFlag(QStyle::State_Enabled);
        const int defaultLineWidth = 1;

        if (hasError)
        {
            return errorLineWidth;
        }

        if (isFocused || dropTarget)
        {
            return focusedLineWidth;
        }

        if (isHovered)
        {
            return hoverLineWidth;
        }

        return defaultLineWidth;
    }

    QColor LineEdit::Config::getBorderColor(const QStyleOption* option, bool hasError, bool dropTarget) const
    {
        const QStyleOptionFrame* frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option);
        if (!frameOption)
        {
            return {};
        }

        const bool isFocused = option->state.testFlag(QStyle::State_HasFocus);
        const bool isHovered = option->state.testFlag(QStyle::State_MouseOver) && option->state.testFlag(QStyle::State_Enabled);

        return hasError
                ? errorBorderColor
                : (isFocused || dropTarget
                    ? focusedBorderColor
                    : (isHovered
                        ? hoverBorderColor
                        :(borderColor.isValid()
                            ? borderColor
                            : Qt::transparent)));
    }

    QColor LineEdit::Config::getBackgroundColor(const QStyleOption* option, bool hasError, bool isDropTarget, const QWidget* widget) const
    {
        Q_UNUSED(hasError);
        const QStyleOptionFrame* frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option);
        if (!frameOption)
        {
            return {};
        }

        const bool isFocused = option->state.testFlag(QStyle::State_HasFocus);
        return isFocused || isDropTarget ? hoverBackgroundColor : option->palette.color(widget->backgroundRole());
    }

    void LineEdit::applySearchStyle(QLineEdit* lineEdit)
    {
        if (lineEdit->property(HasSearchAction).toBool())
        {
            return;
        }

        lineEdit->setProperty(HasSearchAction, true);

        auto searchToolButton = lineEdit->findChild<QToolButton*>(SearchToolButton);
        if (!searchToolButton)
        {
            // Adding a QAction to a QLineEdit results in a private QToolButton derived widget being
            // added as a child of the QLineEdit.
            // We cannot simply call QWidget::removeAction in LineEdit::removeSearchStyle because
            // this results in a crash when LineEdit::removeSearchStyle is called during
            // QApplication::setStyle. QWidget::removeAction deletes the private widget and then
            // QApplication attempts to unpolish the deleted private widget.
            // To work around this we find the private widget after it has been added and set it's
            // object name so we can find it again in LineEdit::removeSearchStyle.
            auto searchAction = lineEdit->addAction(QIcon(":/stylesheet/img/search-dark.svg"), QLineEdit::LeadingPosition);
            searchAction->setEnabled(false);

            auto childToolButtons = lineEdit->findChildren<QToolButton*>();
            for (auto toolButton : childToolButtons)
            {
                if (toolButton->defaultAction() == searchAction)
                {
                    toolButton->setObjectName(SearchToolButton);
                    break;
                }
            }
            return;
        }

        searchToolButton->show();
    }

    void LineEdit::removeSearchStyle(QLineEdit* lineEdit)
    {
        if (!lineEdit->property(HasSearchAction).toBool())
        {
            return;
        }

        lineEdit->setProperty(HasSearchAction, false);

        auto searchToolButton = lineEdit->findChild<QToolButton*>(SearchToolButton);
        searchToolButton->hide();
    }

    void LineEdit::applyDropTargetStyle(QLineEdit* lineEdit, bool valid)
    {
        LineEdit::removeDropTargetStyle(lineEdit);
        Style::addClass(lineEdit, valid ? ValidDropTarget : InvalidDropTarget);
    }

    void LineEdit::removeDropTargetStyle(QLineEdit* lineEdit)
    {
        Style::removeClass(lineEdit, ValidDropTarget);
        Style::removeClass(lineEdit, InvalidDropTarget);
    }

    LineEdit::Config LineEdit::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();

         // Keep radius in sync with css, it is not possible to get it back from the style option :/
        ConfigHelpers::read<int>(settings, QStringLiteral("BorderRadius"), config.borderRadius);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("BorderColor"), config.borderColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("HoverBackgroundColor"), config.hoverBackgroundColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("HoverBorderColor"), config.hoverBorderColor);
        ConfigHelpers::read<int>(settings, QStringLiteral("HoverLineWidth"), config.hoverLineWidth);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("FocusedBorderColor"), config.focusedBorderColor);
        ConfigHelpers::read<int>(settings, QStringLiteral("FocusedLineWidth"), config.focusedLineWidth);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("ErrorBorderColor"), config.errorBorderColor);
        ConfigHelpers::read<int>(settings, QStringLiteral("ErrorLineWidth"), config.errorLineWidth);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("PlaceHolderTextColor"), config.placeHolderTextColor);
        ConfigHelpers::read<QString>(settings, QStringLiteral("ClearImage"), config.clearImage);
        ConfigHelpers::read<QString>(settings, QStringLiteral("ClearImageDisabled"), config.clearImageDisabled);
        ConfigHelpers::read<QSize>(settings, QStringLiteral("ClearImageSize"), config.clearImageSize);
        ConfigHelpers::read<QString>(settings, QStringLiteral("ErrorImage"), config.errorImage);
        ConfigHelpers::read<QSize>(settings, QStringLiteral("ErrorImageSize"), config.errorImageSize);
        ConfigHelpers::read<int>(settings, QStringLiteral("IconSpacing"), config.iconSpacing);
        ConfigHelpers::read<int>(settings, QStringLiteral("IconMargin"), config.iconMargin);
        ConfigHelpers::read<bool>(settings, QStringLiteral("AutoSelectAllOnClickFocus"), config.autoSelectAllOnClickFocus);
        ConfigHelpers::read<int>(settings, QStringLiteral("DropFrameOffset"), config.dropFrameOffset);
        ConfigHelpers::read<int>(settings, QStringLiteral("DropFrameRadius"), config.dropFrameRadius);

        return config;
    }

    LineEdit::Config LineEdit::defaultConfig()
    {
        Config config;

        // Keep radius in sync with css, it is not possible to get it back from the style option :/
        config.borderRadius = 2;
        // An invalid color will take current background color
        config.borderColor = QColor(Qt::transparent);
        config.hoverBackgroundColor = QColor("#FFFFFF");
        config.hoverBorderColor = QColor("#222222");
        config.hoverLineWidth = 1;
        config.focusedBorderColor = QColor("#94D2FF");
        config.focusedLineWidth = 1;
        config.errorBorderColor = QColor("#E25243");
        config.errorLineWidth = 2;
        config.placeHolderTextColor = QColor("#888888");
        config.clearImage = QStringLiteral(":/stylesheet/img/UI20/lineedit-close.svg");
        config.clearImageDisabled = QStringLiteral(":/stylesheet/img/UI20/lineedit-close-disabled.svg");
        config.clearImageSize = {14, 14};
        config.errorImage = QStringLiteral(":/stylesheet/img/UI20/lineedit-error.svg");
        config.errorImageSize = {14, 14};
        config.iconSpacing = 0; // Due to 2px padding in icons, use 0 here to give 4px spacing between images
        config.iconMargin = 2;
        config.autoSelectAllOnClickFocus = true;
        config.dropFrameOffset = 2;
        config.dropFrameRadius = 1;

        return config;
    }

    void LineEdit::setErrorMessage(QLineEdit* lineEdit, const QString& error)
    {
        lineEdit->setProperty(ErrorMessage, error);
    }

    void LineEdit::setExternalError(QLineEdit* lineEdit, bool hasExternalError)
    {
        lineEdit->setProperty(HasExternalError, hasExternalError);
        lineEdit->style()->unpolish(lineEdit);
        lineEdit->style()->polish(lineEdit);
    }

    void LineEdit::setErrorIconEnabled(QLineEdit* lineEdit, bool enabled)
    {
        lineEdit->setProperty(ErrorIconEnabled, enabled);
        lineEdit->style()->unpolish(lineEdit);
        lineEdit->style()->polish(lineEdit);
    }

    bool LineEdit::errorIconEnabled(QLineEdit* lineEdit)
    {
        const auto enabled = lineEdit->property(ErrorIconEnabled);
        return enabled.isValid() ? enabled.toBool() : true;
    }

    QToolButton* LineEdit::getClearButton(const QLineEdit* lineEdit)
    {
        auto clearToolButton = lineEdit->findChild<QToolButton*>(ClearToolButton);
        if (!clearToolButton)
        {
            QAction* clearAction = lineEdit->findChild<QAction*>(ClearAction);
            if (!clearAction)
            {
                return nullptr;
            }
            auto childToolButtons = lineEdit->findChildren<QToolButton*>();
            for (auto toolButton : childToolButtons)
            {
                if (toolButton->defaultAction() == clearAction)
                {
                    clearToolButton = toolButton;
                    clearToolButton->setObjectName(ClearToolButton);
                    break;
                }
            }
        }
        return clearToolButton;
    }

    void LineEdit::setEnableClearButtonWhenReadOnly(QLineEdit* lineEdit, bool enabled)
    {
        lineEdit->setProperty(g_enableClearButtonWhenReadOnly, enabled);
    }

    void LineEdit::initializeWatcher()
    {
        if (!s_lineEditWatcher)
        {
            Q_ASSERT(s_watcherReferenceCount == 0);
            s_lineEditWatcher = new LineEditWatcher;
        }

        ++s_watcherReferenceCount;
    }

    void LineEdit::uninitializeWatcher()
    {
        Q_ASSERT(!s_lineEditWatcher.isNull());
        Q_ASSERT(s_watcherReferenceCount > 0);

        --s_watcherReferenceCount;

        if (s_watcherReferenceCount == 0)
        {
            delete s_lineEditWatcher;
            s_lineEditWatcher = nullptr;
        }
    }

    bool LineEdit::polish(QProxyStyle* style, QWidget* widget, const LineEdit::Config& config)
    {
        Q_ASSERT(!s_lineEditWatcher.isNull());

        auto lineEdit = qobject_cast<QLineEdit*>(widget);

        if (lineEdit)
        {
            QObject::connect(lineEdit, &QLineEdit::textChanged, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);
            QObject::connect(lineEdit, &QLineEdit::textChanged, s_lineEditWatcher, &LineEditWatcher::updateErrorStateSlot);
            QObject::connect(lineEdit, &QLineEdit::returnPressed, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);
            QObject::connect(lineEdit, &QLineEdit::editingFinished, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);

            QPalette pal = lineEdit->palette();
#if !defined(AZ_PLATFORM_LINUX)
            pal.setColor(QPalette::PlaceholderText, config.placeHolderTextColor);
#endif // !defined(AZ_PLATFORM_LINUX)

            s_lineEditWatcher->storeHoverAttributeState(lineEdit);
            lineEdit->setAttribute(Qt::WA_Hover, true);

            if (LineEdit::errorIconEnabled(lineEdit))
            {
                LineEdit::applyErrorStyle(lineEdit, config);
            }
            s_lineEditWatcher->positionSideWidgets(lineEdit);

            lineEdit->setPalette(pal);
            lineEdit->installEventFilter(s_lineEditWatcher);

            // initial state checks
            s_lineEditWatcher->updateErrorState(lineEdit);
            s_lineEditWatcher->updateClearButtonState(lineEdit);

            Style* newStyle = qobject_cast<Style*>(style);
            if (newStyle)
            {
                newStyle->repolishOnSettingsChange(lineEdit);
            }
            s_lineEditWatcher->m_config = config;
        }

        return lineEdit;
    }

    bool LineEdit::unpolish(QProxyStyle* style, QWidget* widget, const LineEdit::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        Q_ASSERT(!s_lineEditWatcher.isNull());

        auto lineEdit = qobject_cast<QLineEdit*>(widget);

        if (lineEdit)
        {
            QObject::disconnect(lineEdit, &QLineEdit::textChanged, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);
            QObject::disconnect(lineEdit, &QLineEdit::textChanged, s_lineEditWatcher, &LineEditWatcher::updateErrorStateSlot);
            QObject::disconnect(lineEdit, &QLineEdit::returnPressed, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);
            QObject::disconnect(lineEdit, &QLineEdit::editingFinished, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);

            lineEdit->removeEventFilter(s_lineEditWatcher);
            s_lineEditWatcher->restoreHoverAttributeState(lineEdit);

            LineEdit::removeErrorStyle(lineEdit);

            if(lineEdit->isClearButtonEnabled())
            {
                // We turn clear action visiblity on and off in UI2.0.
                // Make sure that 1.0 gets it in native, visible state
                auto clearAction = lineEdit->findChild<QAction*>(ClearAction);
                if(clearAction)
                {
                    clearAction->setVisible(true);
                }
            }
        }

        return lineEdit;
    }

    bool LineEdit::drawFrame(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const LineEdit::Config& config)
    {
        auto lineEdit = qobject_cast<const QLineEdit*>(widget);
        if (!lineEdit || !Style::hasStyle(lineEdit))
        {
            return false;
        }

        if (lineEdit->hasFrame())
        {
            const bool hasError = lineEdit->property(HasError).toBool();
            const bool validDropTarget = Style::hasClass(lineEdit, ValidDropTarget);
            const bool invalidDropTarget = Style::hasClass(lineEdit, InvalidDropTarget);
            const bool dropTarget = validDropTarget || invalidDropTarget;
            const int lineWidth = config.getLineWidth(option, hasError, dropTarget);
            const QColor backgroundColor = config.getBackgroundColor(option, hasError, dropTarget, lineEdit);

            if (lineWidth > 0)
            {
                const QColor frameColor = config.getBorderColor(option, hasError, dropTarget);
                const auto borderRect = style->borderLineEditRect(option->rect, lineWidth, config.borderRadius);
                Style::drawFrame(painter, borderRect, Qt::NoPen, frameColor);
            }

            const auto frameRect = style->lineEditRect(option->rect, lineWidth, config.borderRadius);
            Style::drawFrame(painter, frameRect, Qt::NoPen, backgroundColor);

            if (validDropTarget)
            {
                const int offset = config.dropFrameOffset;
                const QRect dropFrameRect = option->rect.adjusted(offset, offset, -offset, -offset);
                const QPainterPath dropFramePath = style->lineEditRect(dropFrameRect, lineWidth, config.dropFrameRadius);
                Style::drawFrame(painter, dropFramePath, Qt::NoPen, config.focusedBorderColor);
            }
        }
        else
        {
            return false;
        }

        return true;
    }

    void LineEdit::applyErrorStyle(QLineEdit* lineEdit, const Config& config)
    {
        auto errorToolButton = lineEdit->findChild<QToolButton*>(ErrorToolButton);
        if (!errorToolButton)
        {
            QIcon icon;
            icon.addFile(config.errorImage, config.errorImageSize);
            auto errorAction = lineEdit->addAction(icon, QLineEdit::TrailingPosition);

            auto childToolButtons = lineEdit->findChildren<QToolButton*>();
            for (auto toolButton : childToolButtons)
            {
                if (toolButton->defaultAction() == errorAction)
                {
                    toolButton->setObjectName(ErrorToolButton);
                    break;
                }
            }
        }
        else
        {
            // If we switch from UI 1.0, the action for error might be invisible
            errorToolButton->defaultAction()->setVisible(true);
        }
    }

    void LineEdit::removeErrorStyle(QLineEdit* lineEdit)
    {
        auto errorToolButton = lineEdit->findChild<QToolButton*>(ErrorToolButton);
        if(errorToolButton)
        {
            errorToolButton->defaultAction()->setVisible(false);
        }
    }

    QIcon LineEdit::clearButtonIcon(const QStyleOption* option, const QWidget* widget, const Config& config)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);

        QIcon icon;
        icon.addFile(config.clearImage, config.clearImageSize, QIcon::Normal);
        icon.addFile(config.clearImageDisabled, config.clearImageSize, QIcon::Disabled);
        return icon;
    }

    QRect LineEdit::lineEditContentsRect(const Style* style, QStyle::SubElement element, const QStyleOption* option, const QWidget* widget, const Config& config)
    {
        Q_UNUSED(config);

        const QLineEdit* lineEdit = qobject_cast<const QLineEdit*>(widget);
        if (element != QStyle::SE_LineEditContents || lineEdit == nullptr)
        {
            return QRect();
        }

        // first check what the base style would do
        QRect r = style->baseStyle()->subElementRect(element, option, widget);

        QToolButton* clearToolButton = LineEdit::getClearButton(lineEdit); // May be nullptr
        auto errorToolButton = lineEdit->findChild<QToolButton*>(ErrorToolButton);
        const QList<QToolButton*> rightButtons = {clearToolButton, errorToolButton};
        int numButtons = 0;
        int leftMostButtonPosition = r.right();
        for (auto button : rightButtons)
        {
            if (button && button->isVisibleTo(button->parentWidget()))
            {
                ++numButtons;
                leftMostButtonPosition = std::min(button->geometry().left(), leftMostButtonPosition);
            }
        }

        // then move the right side to match the position of the left-most button
        r.setRight(leftMostButtonPosition);

        if (numButtons > 0)
        {
            // and finally add the right margins QLineEdit removes to make the buttons fit (it thinks)
            const int iconSize = style->pixelMetric(QStyle::PM_SmallIconSize, nullptr, widget);
            const int delta = iconSize / 4 + iconSize + 6;
            r.setRight(r.right() + delta * numButtons);
        }

        return r;
    }

} // namespace AzQtComponents
