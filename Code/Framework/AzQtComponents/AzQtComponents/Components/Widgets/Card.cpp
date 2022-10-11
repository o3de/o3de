/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>

#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/CardNotification.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleHelpers.h>
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h> // for the QMargins metatype declarations
#include <QDesktopWidget>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStyle>
#include <QPoint>
#include <QSettings>

namespace AzQtComponents
{

    static QString g_containerCardClass = QStringLiteral("ContainerCard");
    static QString g_sectionCardClass = QStringLiteral("SectionCard");

    static QPixmap ApplyAlphaToPixmap(const QPixmap& pixmap, float alpha)
    {
        QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < image.height(); ++y)
        {
            auto scanLine = reinterpret_cast<QRgb*>(image.scanLine(y));
            for (int x = 0; x < image.width(); ++x)
            {
                const auto color = scanLine[x];
                scanLine[x] = qRgba(qRed(color), qGreen(color), qBlue(color), static_cast<int>(alpha * qAlpha(color)));
            }
        }
        return QPixmap::fromImage(image);
    }

    void Card::applyContainerStyle(Card* card)
    {
        Style::addClass(card, g_containerCardClass);
        CardHeader::applyContainerStyle(card->header());
    }

    void Card::applySectionStyle(Card* card)
    {
        Style::addClass(card, g_sectionCardClass);
        CardHeader::applySectionStyle(card->header());
    }

    Card::Card(QWidget* parent /* = nullptr */)
        // DO NOT SET THE PARENT OF THE CardHeader TO THIS!
        // It will cause a crash, because the this object is not initialized enough to be used as a parent object.
        : Card(new CardHeader(), parent)
    {
    }

    Card::Card(CardHeader* customHeader, QWidget* parent)
        : QFrame(parent)
        , m_header(customHeader)
    {
        // The header is owned by the Card, so set the parent.
        m_header->setParent(this);

        Config defaultConfigValues = defaultConfig();
        m_warningIconSize = defaultConfigValues.warningIconSize;

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        m_warningIcon.addFile(defaultConfigValues.warningIcon, m_warningIconSize);

        // create header bar
        Style::addClass(m_header, "primaryCardHeader");
        m_header->setWarningIcon(m_warningIcon);
        m_header->setWarning(false);
        m_header->setReadOnly(false);
        m_header->setExpandable(true);
        m_header->setHasContextMenu(true);

        // has to be done here, otherwise it will affect all tooltips
        // as tooltips do not support any kind of css matchers other than QToolTip
        m_header->setStyleSheet(QStringLiteral("QToolTip { padding: %1px; }").arg(defaultConfigValues.toolTipPaddingInPixels));

        m_rootLayout = new QVBoxLayout(this);
        m_rootLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_rootLayout->setContentsMargins(QMargins(0, 0, 0, 0));
        m_rootLayout->addWidget(m_header);

        m_contentContainer = new QFrame(this);
        m_contentContainer->setObjectName(QStringLiteral("contentContainer"));

        m_mainLayout = new QVBoxLayout(m_contentContainer);
        m_mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_mainLayout->setContentsMargins(QMargins(0, 0, 0, 0));

        m_rootLayout->addWidget(m_contentContainer);

        // add a placeholder, so that the ordering of widgets in Cards works
        m_contentWidget = new QWidget(this);
        m_mainLayout->addWidget(m_contentWidget);


        m_separatorContainer = new QFrame(this);
        m_separatorContainer->setObjectName("SeparatorContainer");
        QBoxLayout* separatorLayout = new QHBoxLayout(m_separatorContainer);
        separatorLayout->setContentsMargins(QMargins(0, 0, 0, 0));
        m_separatorContainer->setLayout(separatorLayout);

        m_separator = new QFrame(m_separatorContainer);
        AzQtComponents::Style::addClass(m_separator, "separator");
        m_separator->setFrameShape(QFrame::HLine);
        separatorLayout->addWidget(m_separator);

        m_secondaryHeader = new AzQtComponents::CardHeader(this);
        AzQtComponents::Style::addClass(m_secondaryHeader, "secondaryCardHeader");
        m_secondaryHeader->setExpanded(false); // default the secondary header to not expanded
        m_secondaryHeader->setExpandable(true);

        m_mainLayout->addWidget(m_separatorContainer);
        m_mainLayout->addWidget(m_secondaryHeader);

        updateSecondaryContentVisibility();

        connect(m_secondaryHeader, &CardHeader::expanderChanged, this, &Card::setSecondaryContentExpanded);

        connect(m_header, &CardHeader::expanderChanged, this, &Card::setExpanded);
        connect(m_header, &CardHeader::contextMenuRequested, this, &Card::contextMenuRequested);

        StyleHelpers::repolishWhenPropertyChanges(this, &Card::selectedChanged);
        StyleHelpers::repolishWhenPropertyChanges(this, &Card::expandStateChanged);
    }

    QWidget* Card::contentWidget() const
    {
        return m_contentWidget;
    }

    void Card::setContentWidget(QWidget* contentWidget)
    {
        delete m_contentWidget;

        m_contentWidget = contentWidget;

        m_mainLayout->insertWidget(0, contentWidget);
    }

    void Card::setTitle(const QString& title)
    {
        m_header->setTitle(title);
    }
    
    void Card::setTitleToolTip(const QString& toolTip)
    {
        m_header->setTitleToolTip(toolTip);
    }

    QString Card::title() const
    {
        return m_header->title();
    }

    CardHeader* Card::header() const
    {
        return m_header;
    }

    CardNotification* Card::addNotification(const QString& message)
    {
        CardNotification* notification = new CardNotification(this, message, m_warningIcon, m_warningIconSize);

        notification->setVisible(isExpanded());
        m_notifications.push_back(notification);
        m_mainLayout->addWidget(notification);

        return notification;
    }

    void Card::clearNotifications()
    {
        for (CardNotification* notification : m_notifications)
        {
            delete notification;
        }

        m_notifications.clear();
    }

    int Card::getNotificationCount() const
    {
        return m_notifications.size();
    }


    void Card::setExpanded(bool expand)
    {
        // expandStateChanged triggers an expensive style repolish, make sure we short circuit if this is a no-op
        if (expand == m_expanded)
        {
            return;
        }
        setUpdatesEnabled(false);

        m_expanded = expand;
        m_header->setExpanded(expand);
        m_contentContainer->setVisible(expand);
        updateSecondaryContentVisibility();
        updateGeometry();

        setUpdatesEnabled(true);

        Q_EMIT expandStateChanged(expand);
    }

    bool Card::isExpanded() const
    {
        return m_expanded;
    }

    void Card::setSelected(bool selected)
    {
        if (m_selected != selected)
        {
            m_selected = selected;
            emit selectedChanged(m_selected);
        }
    }

    bool Card::isSelected() const
    {
        return m_selected;
    }

    void Card::setDragging(bool dragging)
    {
        m_dragging = dragging;
    }

    bool Card::isDragging() const
    {
        return m_dragging;
    }

    void Card::setDropTarget(bool dropTarget)
    {
        m_dropTarget = dropTarget;
    }

    bool Card::isDropTarget() const
    {
        return m_dropTarget;
    }


    void Card::setSecondaryContentExpanded(bool expand)
    {
        {
            // No need to respond to the secondary header state changing; we're causing the change
            QSignalBlocker blocker(m_secondaryHeader);
            m_secondaryHeader->setExpanded(expand);
        }

        updateSecondaryContentVisibility();

        Q_EMIT secondaryContentExpandStateChanged(expand);
    }

    bool Card::isSecondaryContentExpanded() const
    {
        return m_secondaryHeader->isExpanded();
    }

    void Card::setSecondaryTitle(const QString& secondaryTitle)
    {
        m_secondaryHeader->setTitle(secondaryTitle);

        updateSecondaryContentVisibility();
    }

    QString Card::secondaryTitle() const
    {
        return m_secondaryHeader->title();
    }

    void Card::setSecondaryContentWidget(QWidget* secondaryContentWidget)
    {
        delete m_secondaryContentWidget;

        m_secondaryContentWidget = secondaryContentWidget;

        // Layout is:
        // 0 - content widget
        // 1 - separator
        // 2 - secondary header
        // 3 - secondary widget
        // 4, etc - notifications

        m_mainLayout->insertWidget(3, secondaryContentWidget);

        updateSecondaryContentVisibility();
    }

    QWidget* Card::secondaryContentWidget() const
    {
        return m_secondaryContentWidget;
    }

    void Card::hideFrame()
    {
        setProperty("hideFrame", true);
        style()->unpolish(this);
        style()->polish(this);
    }

    void Card::mockDisabledState(bool disable)
    {
        // Only partially disable CardHeader
        if (m_header)
        {
            m_header->mockDisabledState(disable);
        }
        if (m_secondaryHeader)
        {
            m_secondaryHeader->mockDisabledState(disable);
        }

        // Disable Content
        if (m_contentWidget)
        {
            m_contentWidget->setDisabled(disable);
        }
        if (m_secondaryContentWidget)
        {
            m_secondaryContentWidget->setDisabled(disable);
        }
    }

    Card::Config Card::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();

        ConfigHelpers::read<int>(settings, QStringLiteral("HeaderIconSizeInPixels"), config.headerIconSizeInPixels);
        ConfigHelpers::read<int>(settings, QStringLiteral("ToolTipPaddingInPixels"), config.toolTipPaddingInPixels);
        ConfigHelpers::read<int>(settings, QStringLiteral("RootLayoutSpacing"), config.rootLayoutSpacing);
        ConfigHelpers::read<QString>(settings, QStringLiteral("WarningIcon"), config.warningIcon);
        ConfigHelpers::read<QSize>(settings, QStringLiteral("WarningIconSize"), config.warningIconSize);
        ConfigHelpers::read<qreal>(settings, QStringLiteral("DisabledIconAlpha"), config.disabledIconAlpha);

        return config;
    }

    Card::Config Card::defaultConfig()
    {
        Config config;

        config.toolTipPaddingInPixels = 5;
        config.headerIconSizeInPixels = CardHeader::defaultIconSize();
        config.rootLayoutSpacing = 0;
        config.warningIcon = QStringLiteral(":/Notifications/warning.svg");
        config.warningIconSize = {24, 24};
        config.disabledIconAlpha = 0.25;

        return config;
    }

    bool Card::polish(Style* style, QWidget* widget, const Card::Config& config)
    {
        bool polished = false;

        if (auto card = qobject_cast<Card*>(widget))
        {
            QIcon warningIcon;
            warningIcon.addFile(config.warningIcon, config.warningIconSize);
            card->m_warningIcon = warningIcon;
            card->m_warningIconSize = config.warningIconSize;

            card->m_rootLayout->setSpacing(config.rootLayoutSpacing);
            style->repolishOnSettingsChange(card);
            polished = true;
        }
        else if (auto cardHeader = qobject_cast<CardHeader*>(widget))
        {
            CardHeader::setIconSize(config.headerIconSizeInPixels);

            QString newStyleSheet = QStringLiteral("QToolTip { padding: %1px; }").arg(config.toolTipPaddingInPixels);
            if (newStyleSheet != cardHeader->styleSheet())
            {
                cardHeader->setStyleSheet(newStyleSheet);
            }

            cardHeader->configSettingsChanged();

            style->repolishOnSettingsChange(cardHeader);

            polished = true;
        }

        return polished;
    }

    bool Card::unpolish(Style* style, QWidget* widget, const Card::Config& config)
    {
        (void)style;
        (void)config;
        
        bool unpolished = false;

        if (auto card = qobject_cast<Card*>(widget))
        {
            card->m_rootLayout->setSpacing(3); // restore to default
            unpolished = true;
        }

        return unpolished;
    }

    QPixmap Card::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* option, const QWidget* widget, const Config& config)
    {
        const auto* iconWidget = widget ? widget : qobject_cast<QWidget*>(option->styleObject);
        if (CardHeader::isCardHeaderMenuButton(iconWidget))
        {
            // don't gray out menu icons in the header, even if the card is disabled
            return pixmap;
        }
        if (CardHeader::isCardHeaderIcon(iconWidget))
        {
            if (iconMode == QIcon::Disabled)
            {
                return ApplyAlphaToPixmap(pixmap, aznumeric_cast<float>(config.disabledIconAlpha));
            }
        }
        // return a null pixmap so that that Style takes the default path
        return {};
    }

    bool Card::hasSecondaryContent() const
    {
        return ((!m_secondaryHeader->title().isEmpty()) || (m_secondaryContentWidget != nullptr));
    }

    void Card::updateSecondaryContentVisibility()
    {
        bool updatesWereEnabled = updatesEnabled();

        if (updatesWereEnabled)
        {
            setUpdatesEnabled(false);
        }

        bool expanded = m_header->isExpanded();
        bool secondaryExpanded = hasSecondaryContent() && expanded && m_secondaryHeader->isExpanded();

        m_separatorContainer->setVisible(hasSecondaryContent() && expanded);
        m_secondaryHeader->setVisible(hasSecondaryContent() && expanded);

        if (m_secondaryContentWidget != nullptr)
        {
            m_secondaryContentWidget->setVisible(secondaryExpanded);
        }

        if (updatesWereEnabled)
        {
            setUpdatesEnabled(true);
        }
    }
} // namespace AzQtComponents

#include "Components/Widgets/moc_Card.cpp"
