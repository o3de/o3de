/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/IconElidingLabel.h>

#include <QEvent>
#include <QStyle>
#include <QStyleOption>
#include <QStylePainter>
#include <QToolButton>

namespace AzQtComponents
{
    namespace IconElidingLabelInternal
    {
        //! The tool button the icon's area is drawn as. To its style and to style sheets it is a plain QToolButton (it has no meta
        //! object of its own); the label reads the option the button would draw itself with.
        class IconButton
            : public QToolButton
        {
        public:
            using QToolButton::QToolButton;
            using QToolButton::initStyleOption;
        };

        QString IconButtonName()
        {
            return QStringLiteral("IconElidingLabelButton");
        }
    } // namespace IconElidingLabelInternal

    // ----------------------------------------------------------------------------------
    // Construction and the icon
    // ----------------------------------------------------------------------------------

    IconElidingLabel::IconElidingLabel(QWidget* parent)
        : ElidingLabel(parent)
    {
        // The policy of the container it stands in for (a plain QWidget's): the same in the row's layout
        setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    }

    void IconElidingLabel::setIcon(const QIcon& icon, const QSize& iconSize)
    {
        m_icon = icon;
        if (m_iconSize != iconSize)
        {
            m_iconSize = iconSize;
            m_buttonSize = QSize();
            updateIndent();
            updateGeometry();
        }
        update();
    }

    // ----------------------------------------------------------------------------------
    // The button: the hidden tool button of the window, and its size
    // ----------------------------------------------------------------------------------

    QToolButton* IconElidingLabel::iconButton() const
    {
        using IconElidingLabelInternal::IconButton;

        QWidget* labelWindow = window();
        if (labelWindow == this)
        {
            return nullptr;
        }

        // One for every label of the window, made at the first one's need, and gone with the window. It is hidden, so it is never
        // in the Tab chain.
        auto* button = static_cast<IconButton*>(
            labelWindow->findChild<QToolButton*>(IconElidingLabelInternal::IconButtonName(), Qt::FindDirectChildrenOnly));
        if (!button)
        {
            button = new IconButton(labelWindow);
            button->setObjectName(IconElidingLabelInternal::IconButtonName());
            button->hide();
        }

        // A style set on the label (and not a style sheet's, which reaches the button through the window) is the button's too, as the
        // owner of a container set it on the button beside the label
        QStyle* labelStyle = style();
        if (button->style() != labelStyle && !labelStyle->inherits("QStyleSheetStyle"))
        {
            button->setStyle(labelStyle);
        }
        return button;
    }

    QSize IconElidingLabel::iconButtonSize() const
    {
        if (!m_buttonSize.isValid())
        {
            QToolButton* button = iconButton();
            if (!button || !m_iconSize.isValid())
            {
                return QSize(0, 0);
            }
            button->setIconSize(m_iconSize);
            m_buttonSize = button->sizeHint()
                               .expandedTo(button->minimumSizeHint())
                               .boundedTo(button->maximumSize())
                               .expandedTo(button->minimumSize());
            button->resize(m_buttonSize);
        }
        return m_buttonSize;
    }

    // ----------------------------------------------------------------------------------
    // Layout: the text after the button, the hints of the two side by side
    // ----------------------------------------------------------------------------------

    void IconElidingLabel::updateIndent()
    {
        // The label beside the button had no indent (-1): a framed one has its text half an x in, less the margin
        // (QLabelPrivate::documentRect). That stays, after the button's width.
        int indent = iconButtonSize().width();
        if (frameWidth())
        {
            indent += qMax(0, fontMetrics().horizontalAdvance(QLatin1Char('x')) / 2 - margin());
        }
        setIndent(indent);
    }

    QRect IconElidingLabel::TextRect()
    {
        // ElidingLabel::TextRect of a label without an indent (a framed label loses half an x on each side), less the button
        QRect textRect = contentsRect();
        if (frameWidth())
        {
            const int textMargin = fontMetrics().horizontalAdvance(QLatin1Char('x')) / 2;
            textRect.adjust(textMargin, 0, -textMargin, 0);
        }
        textRect.setLeft(textRect.left() + iconButtonSize().width());
        return textRect;
    }

    QSize IconElidingLabel::sizeHint() const
    {
        // QBoxLayout's hint for the two: the button's size, and the label's hint within its limits (QWidgetItem::sizeHint)
        const QSize button = iconButtonSize();
        const QSize label = ElidingLabel::sizeHint().boundedTo(maximumSize()).expandedTo(minimumSize());
        return QSize(button.width() + label.width(), qMax(button.height(), label.height()));
    }

    QSize IconElidingLabel::minimumSizeHint() const
    {
        // QBoxLayout's minimum for the two: the button's size, and the label's limits (QWidgetItem::minimumSize; the label has no
        // minimum of its own, ElidingLabel::minimumSizeHint)
        const QSize button = iconButtonSize();
        const QSize label = minimumSize();
        return QSize(button.width() + label.width(), qMax(button.height(), label.height()));
    }

    bool IconElidingLabel::event(QEvent* event)
    {
        const bool result = ElidingLabel::event(event);
        switch (event->type())
        {
        case QEvent::StyleChange:   // the button's size is its style's
        case QEvent::ParentChange:  // and the button is the window's
            m_buttonSize = QSize();
            updateIndent();
            updateGeometry();
            break;
        case QEvent::FontChange:    // the half x of a framed label
            updateIndent();
            break;
        default:
            break;
        }
        return result;
    }

    // ----------------------------------------------------------------------------------
    // Painting: the label, then the button where the layout put it
    // ----------------------------------------------------------------------------------

    void IconElidingLabel::paintEvent(QPaintEvent* event)
    {
        ElidingLabel::paintEvent(event);

        auto* button = static_cast<IconElidingLabelInternal::IconButton*>(iconButton());
        if (!button || !m_iconSize.isValid())
        {
            return;
        }

        // At the left, centred in the label's height, as QWidgetItem::setGeometry placed a fixed size button in the layout's cell
        const QSize buttonSize = iconButtonSize().boundedTo(size());
        const QPoint buttonPosition(0, (height() - buttonSize.height()) / 2);

        // The option the button beside the label drew itself with: the window's button gives the style's and the rules' part (and the
        // window's active state); the label gives the icon and its own state, enabled or not, and the colour group of its palette
        QStyleOptionToolButton option;
        button->initStyleOption(&option);
        option.rect = QRect(QPoint(0, 0), buttonSize);
        option.icon = m_icon;
        option.iconSize = m_iconSize;
        option.state.setFlag(QStyle::State_Enabled, isEnabled());
        option.palette.setCurrentColorGroup(palette().currentColorGroup());

        // Painted as QToolButton::paintEvent paints: through a QStylePainter, whose smooth pixmap transform scales the icon's pixmap
        // to a high DPI screen as the button did
        QStylePainter painter(this, button);
        painter.translate(buttonPosition);
        painter.drawComplexControl(QStyle::CC_ToolButton, option);
    }
} // namespace AzQtComponents
