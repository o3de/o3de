/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/Widgets/Internal/RectangleWidget.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleHelpers.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyle>
#include <QTimer>
#include <QPainter>

namespace AzQtComponents
{
    static QString g_containerCardHeaderClass = QStringLiteral("ContainerCardHeader");
    static QString g_sectionCardHeaderClass = QStringLiteral("SectionCardHeader");

    namespace Internal
    {
        ClickableIconLabel::ClickableIconLabel(QWidget* parent)
            : QLabel(parent)
        {
        }

        void ClickableIconLabel::SetClickable(bool clickable)
        {
            m_clickable = clickable;
        }

        bool ClickableIconLabel::GetClickable() const
        {
            return m_clickable;
        }

        void ClickableIconLabel::mouseReleaseEvent(QMouseEvent* event)
        {
            if (m_clickable)
            {
                emit clicked(event->globalPos());
            }
            else
            {
                event->ignore();
            }
        }
    }

    namespace HeaderBarConstants
    {
        // names for widgets so they can be found in stylesheet
        static const char* kBackgroundId = "Background";
        static const char* kExpanderId = "Expander";
        static const char* kIconId = "Icon";
        static const char* kWarningIconId = "WarningIcon";
        static const char* kTitleId = "Title";
        static const char* kContextMenuId = "ContextMenu";
        static const char* kContextMenuPlusIconId = "ContextMenuPlusIcon";
        static const char* khelpButtonId = "Help";
        static const char* kUnderlineRectId = "UnderlineRectangle";

        static const char* kCardHeaderIconClassName = "CardHeaderIcon";
        static const char* kCardHeaderMenuClassName = "CardHeaderMenu";
    } // namespace HeaderBarConstants

    void CardHeader::applyContainerStyle(CardHeader* header)
    {
        Style::addClass(header, g_containerCardHeaderClass);
    }

    void CardHeader::applySectionStyle(CardHeader* header)
    {
        Style::addClass(header, g_sectionCardHeaderClass);
        header->setHasContextMenu(false);
    }

    int CardHeader::s_iconSize = CardHeader::defaultIconSize();

    CardHeader::CardHeader(QWidget* parent /*= nullptr*/)
        : QFrame(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        m_backgroundFrame = new QFrame(this);
        m_backgroundFrame->setObjectName(HeaderBarConstants::kBackgroundId);
        m_backgroundFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        m_backgroundFrame->setAutoFillBackground(true);

        // expander widget
        m_expanderButton = new QCheckBox(m_backgroundFrame);
        m_expanderButton->setObjectName(HeaderBarConstants::kExpanderId);
        CheckBox::applyExpanderStyle(m_expanderButton);
        m_expanderButton->setChecked(true);
        m_expanderButton->show();
        connect(m_expanderButton, &QPushButton::toggled, this, &CardHeader::expanderChanged);

        // icon widget
        m_iconLabel = new Internal::ClickableIconLabel(m_backgroundFrame);
        m_iconLabel->setObjectName(HeaderBarConstants::kIconId);
        m_iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        Style::addClass(m_iconLabel, HeaderBarConstants::kCardHeaderIconClassName);
        m_iconLabel->hide();
        connect(m_iconLabel, &Internal::ClickableIconLabel::clicked, this, &CardHeader::triggerIconLabelClicked);

        // title widget
        m_titleLabel = new AzQtComponents::ElidingLabel(m_backgroundFrame);
        m_titleLabel->setObjectName(HeaderBarConstants::kTitleId);
        m_titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        m_titleLabel->hide();

        // warning widget
        m_warningLabel = new QLabel(m_backgroundFrame);
        m_warningLabel->setObjectName(HeaderBarConstants::kWarningIconId);
        m_warningLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_warningLabel->hide();

        m_helpButton = new QPushButton(m_backgroundFrame);
        m_helpButton->setObjectName(HeaderBarConstants::khelpButtonId);
        m_helpButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        Style::addClass(m_helpButton, HeaderBarConstants::kCardHeaderMenuClassName);
        m_helpButton->hide();
        connect(m_helpButton, &QPushButton::clicked, this, &CardHeader::triggerHelpButton);

        // context menu widget
        m_contextMenuButton = new QPushButton(m_backgroundFrame);
        m_contextMenuButton->setObjectName(HeaderBarConstants::kContextMenuId);
        m_contextMenuButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        Style::addClass(m_contextMenuButton, HeaderBarConstants::kCardHeaderMenuClassName);
        m_contextMenuButton->hide();
        connect(m_contextMenuButton, &QPushButton::clicked, this, &CardHeader::triggerContextMenuUnderButton);

        m_backgroundLayout = new QHBoxLayout(m_backgroundFrame);
        m_backgroundLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_backgroundLayout->setSpacing(0);
        m_backgroundLayout->setContentsMargins(0, 0, 0, 0);
        m_backgroundLayout->addWidget(m_expanderButton);
        m_backgroundLayout->addWidget(m_iconLabel);
        m_backgroundLayout->addWidget(m_titleLabel);
        m_backgroundLayout->addStretch(1);
        m_backgroundLayout->addWidget(m_warningLabel);
        m_backgroundLayout->addWidget(m_helpButton);
        m_backgroundLayout->addWidget(m_contextMenuButton);
        m_backgroundFrame->setLayout(m_backgroundLayout);

        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_mainLayout->setSpacing(0);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->addWidget(m_backgroundFrame);

        m_underlineWidget = new Internal::RectangleWidget(this);
        m_underlineWidget->setObjectName(HeaderBarConstants::kUnderlineRectId);
        m_underlineWidget->setFixedHeight(2);
        setUnderlineColor(QColor());
        m_mainLayout->addWidget(m_underlineWidget);

        StyleHelpers::repolishWhenPropertyChanges(this, &CardHeader::warningChanged);
        StyleHelpers::repolishWhenPropertyChanges(this, &CardHeader::readOnlyChanged);
        StyleHelpers::repolishWhenPropertyChanges(this, &CardHeader::contentModifiedChanged);

        setExpanded(true);
        setWarning(false);
        setReadOnly(false);
        setContentModified(false);
    }

    void CardHeader::setTitle(const QString& title)
    {
        m_titleLabel->setText(title);
        m_titleLabel->setVisible(!title.isEmpty());
    }

    void CardHeader::setTitleToolTip(const QString& toolTip)
    {
        m_titleLabel->setToolTip(toolTip);
        setToolTip(toolTip);
    }

    void CardHeader::setFilter(const QString& filter)
    {
        m_titleLabel->setFilter(filter);
    }

    void CardHeader::refreshTitle()
    {
        m_titleLabel->style()->unpolish(m_titleLabel);
        m_titleLabel->style()->polish(m_titleLabel);
        m_titleLabel->update();
    }

    void CardHeader::setTitleProperty(const char* name, const QVariant& value)
    {
        m_titleLabel->setProperty(name, value);
    }

    QString CardHeader::title() const
    {
        return m_titleLabel->text();
    }

    AzQtComponents::ElidingLabel* CardHeader::titleLabel() const
    {
        return m_titleLabel;
    }

    void CardHeader::setIcon(const QIcon& icon)
    {
        m_icon = icon;
        updateIconLabel();
    }

    void CardHeader::setIconOverlay(const QIcon& iconOverlay)
    {
        m_iconOverlay = iconOverlay;
        updateIconLabel();
    }

    void CardHeader::setIconClickable(bool clickable)
    {
        m_iconLabel->SetClickable(clickable);
    }

    bool CardHeader::isIconClickable() const
    {
        return m_iconLabel->GetClickable();
    }

    void CardHeader::updateIconLabel()
    {
        if (!m_icon.isNull())
        {
            QPixmap pixmap = m_icon.pixmap(s_iconSize, s_iconSize);
            if (!m_iconOverlay.isNull())
            {
                QPainter paint(&pixmap);
                paint.drawPixmap(0, 0, m_iconOverlay.pixmap(s_iconSize, s_iconSize));
            }
            m_iconLabel->setPixmap(pixmap);
        }
        m_iconLabel->setVisible(!m_icon.isNull());
    }

    void CardHeader::configSettingsChanged()
    {
        if (!m_icon.isNull() && (m_iconLabel != nullptr))
        {
            updateIconLabel();
        }

        if (!m_warningIcon.isNull())
        {
            m_warningLabel->setPixmap(m_warningIcon.pixmap(s_iconSize, s_iconSize));
        }
    }

    void CardHeader::mockDisabledState(bool disabled)
    {
        m_iconLabel->setDisabled(disabled);
        m_titleLabel->setDisabled(disabled);
    }

    void CardHeader::setIconSize(int iconSize)
    {
        s_iconSize = iconSize;
    }

    int CardHeader::defaultIconSize()
    {
        return 16;
    }

    void CardHeader::setExpandable(bool expandable)
    {
        m_expanderButton->setEnabled(expandable);
        m_expanderButton->setVisible(expandable);
    }

    bool CardHeader::isExpandable() const
    {
        return m_expanderButton->isEnabled();
    }

    void CardHeader::setExpanded(bool expanded)
    {
        m_expanderButton->setChecked(expanded);
    }

    bool CardHeader::isExpanded() const
    {
        return m_expanderButton->isChecked();
    }

    void CardHeader::setWarningIcon(const QIcon& icon)
    {
        m_warningIcon = icon;

        m_warningLabel->setPixmap(!m_warningIcon.isNull() ? m_warningIcon.pixmap(s_iconSize, s_iconSize) : QPixmap());
    }

    void CardHeader::setWarning(bool warning)
    {
        if (m_warning != warning)
        {
            m_warning = warning;
            m_warningLabel->setVisible(m_warning);
            emit warningChanged(m_warning);
        }
    }

    bool CardHeader::isWarning() const
    {
        return m_warning;
    }

    void CardHeader::setReadOnly(bool readOnly)
    {
        if (m_readOnly != readOnly)
        {
            m_readOnly = readOnly;
            emit readOnlyChanged(m_readOnly);
        }
    }

    bool CardHeader::isReadOnly() const
    {
        return m_readOnly;
    }

    void CardHeader::setContentModified(bool modified)
    {
        if (m_modified != modified)
        {
            m_modified = modified;
            emit contentModifiedChanged(m_modified);
        }
    }

    bool CardHeader::isContentModified() const
    {
        return m_modified;
    }

    void CardHeader::setHasContextMenu(bool showContextMenu)
    {
        m_contextMenuButton->setVisible(showContextMenu);
    }

    void CardHeader::mouseDoubleClickEvent(QMouseEvent* event)
    {
        // allow double click to expand/contract
        if (event->button() == Qt::LeftButton && isExpandable())
        {
            bool expand = !isExpanded();
            setExpanded(expand);
            emit expanderChanged(expand);
        }

        QFrame::mouseDoubleClickEvent(event);
    }

    void CardHeader::contextMenuEvent(QContextMenuEvent* event)
    {
        Q_EMIT contextMenuRequested(event->globalPos());
        event->accept();
    }

    void CardHeader::triggerContextMenuUnderButton()
    {
        Q_EMIT contextMenuRequested(mapToGlobal(m_contextMenuButton->pos() + QPoint(0, m_contextMenuButton->height())));
    }

    void CardHeader::triggerHelpButton()
    {
        QDesktopServices::openUrl(QUrl(m_helpUrl));
    }

    void CardHeader::triggerIconLabelClicked(const QPoint& position)
    {
        Q_EMIT iconLabelClicked(position);
    }

    void CardHeader::setHelpURL(const QString& url)
    {
        m_helpUrl = url;
        m_helpButton->setVisible(!m_helpUrl.isEmpty());
    }

    void CardHeader::clearHelpURL()
    {
        m_helpUrl = "";
        m_helpButton->setVisible(false);
    }

    QString CardHeader::helpURL() const
    {
        return m_helpUrl;
    }

    bool CardHeader::isCardHeaderIcon(const QWidget* widget)
    {
        if (widget)
        {
            if (const Style* style = qobject_cast<Style*>(widget->style()))
            {
                return style->hasClass(widget, HeaderBarConstants::kCardHeaderIconClassName);
            }
        }
        return false;
    }

    bool CardHeader::isCardHeaderMenuButton(const QWidget* widget)
    {
        if (widget)
        {
            if (const Style* style = qobject_cast<Style*>(widget->style()))
            {
                return style->hasClass(widget, HeaderBarConstants::kCardHeaderMenuClassName);
            }
        }
        return false;
    }

    void CardHeader::setContextMenuIcon(ContextMenuIcon iconType)
    {
        switch (iconType)
        {
        case ContextMenuIcon::Plus:
            // Object names defined in Card.qss.
            m_contextMenuButton->setObjectName(HeaderBarConstants::kContextMenuPlusIconId);
            break;
        default:
            m_contextMenuButton->setObjectName(HeaderBarConstants::kContextMenuId);
            break;
        }
        Style::addClass(m_contextMenuButton, HeaderBarConstants::kCardHeaderMenuClassName);
    }

    void CardHeader::setUnderlineColor(const QColor& color)
    {
        m_underlineWidget->setColor(color);

        const bool underlineVisible = color.isValid() && color.alpha() != 0;
        m_underlineWidget->setVisible(underlineVisible);
    }

} // namespace AzQtComponents

#include "Components/Widgets/moc_CardHeader.cpp"
