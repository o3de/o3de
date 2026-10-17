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
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/StyleHelpers.h>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStyle>
#include <QPoint>
#include <QSettings>
#include <QStyleOption>
#include <qdrawutil.h>

namespace AzQtComponents
{

    static QString g_containerCardClass = QStringLiteral("ContainerCard");
    static QString g_sectionCardClass = QStringLiteral("SectionCard");

    // ContainerCard / SectionCard have no shadow border-image and no body fill (Card.qss).
    static bool cardHasFlatFrame(const Card* card)
    {
        return Style::hasClass(card, g_containerCardClass) || Style::hasClass(card, g_sectionCardClass);
    }

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
        return aznumeric_cast<int>(m_notifications.size());
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

            if (StyleManager::stylesheetsDisabled())
            {
                // Card.qss resolved to one layer (legacy capture 2026-09-23): the boxes
                // come from Card::shapedFrameContents (SE_ShapedFrameContents, the same
                // contact point QStyleSheetStyle used), the fills from Card::paintEvent.
                // The frames themselves stay unfilled and their layouts carry no margins.
                card->setAutoFillBackground(false);
                card->m_contentContainer->setAutoFillBackground(false);
                card->m_rootLayout->setContentsMargins(0, 0, 0, 0);
                card->m_mainLayout->setContentsMargins(0, 0, 0, 0);

                // .separator { color: #999999 } on the QFrame::HLine: a Plain shadow
                // paints the line with WindowText. TOKENIZE: CardSeparatorColor
                card->m_separator->setFrameShadow(QFrame::Plain);
                card->m_separator->setLineWidth(1);
                QPalette separatorPalette = card->m_separator->palette();
                separatorPalette.setColor(QPalette::WindowText, QColor(0x99, 0x99, 0x99));
                card->m_separator->setPalette(separatorPalette);
            }

            style->repolishOnSettingsChange(card);
            polished = true;
        }
        else if (auto cardHeader = qobject_cast<CardHeader*>(widget))
        {
            CardHeader::setIconSize(config.headerIconSizeInPixels);

            if (StyleManager::stylesheetsDisabled())
            {
                // Card.qss header, one layer. The header fill is painted by
                // CardHeader::paintEvent; #Background stays transparent and gets its
                // 2/7/3/7 padding + 1px border as the frame box (8, 3, 8, 3) from
                // Card::shapedFrameContents. The per-child 4px margins (#Expander,
                // #Icon, #WarningIcon, #Help) collapse into one layout spacing.
                // TOKENIZE: CardHeaderItemSpacing
                cardHeader->m_backgroundFrame->setAutoFillBackground(false);
                cardHeader->m_backgroundLayout->setSpacing(4);
                cardHeader->m_backgroundLayout->setContentsMargins(0, 0, 0, 0);

                // #Title { font-weight: bold }, normal inside a palette card: TextElement CardTitle, applied when
                // each #Title label of the header is polished (the title and the hidden label its ElidingLabel
                // measures the size hint with; ElidingLabel::setObjectName names both)

                // CardHeader[contentModified=true] * { color: #E57829 }
                // TOKENIZE: CardModifiedTextColor
                QPalette titlePalette;
                if (cardHeader->isContentModified())
                {
                    titlePalette.setColor(QPalette::WindowText, QColor(0xE5, 0x78, 0x29));
                }
                cardHeader->m_titleLabel->setPalette(titlePalette);

                // #ContextMenu / #ContextMenuPlusIcon / #Help: qproperty-icon, iconSize,
                // flat, max-width/max-height. TOKENIZE: CardHeaderMenuIcon, CardHeaderHelpIcon
                const bool plusIcon = cardHeader->m_contextMenuButton->objectName() == QLatin1String("ContextMenuPlusIcon");
                cardHeader->m_contextMenuButton->setIcon(QIcon(plusIcon
                    ? QStringLiteral(":/Gallery/Add.svg")
                    : QStringLiteral(":/Cards/img/UI20/Cards/menu_ico.svg")));
                cardHeader->m_contextMenuButton->setIconSize(QSize(18, 18));
                cardHeader->m_contextMenuButton->setFlat(true);
                cardHeader->m_contextMenuButton->setMaximumSize(plusIcon ? QSize(18, 18) : QSize(28, 18));
                cardHeader->m_helpButton->setIcon(QIcon(QStringLiteral(":/Cards/img/UI20/Cards/help.svg")));
                cardHeader->m_helpButton->setIconSize(QSize(16, 16));
                cardHeader->m_helpButton->setFlat(true);
                cardHeader->m_helpButton->setMaximumSize(QSize(16, 16));
            }
            else
            {
                // Installing this per-widget sheet would re-interpose QStyleSheetStyle
                // over the flattened chain, so it is qss-mode only.
                QString newStyleSheet = QStringLiteral("QToolTip { padding: %1px; }").arg(config.toolTipPaddingInPixels);
                if (newStyleSheet != cardHeader->styleSheet())
                {
                    cardHeader->setStyleSheet(newStyleSheet);
                }
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

    QRect Card::shapedFrameContents(const QStyleOption* option, const QWidget* widget)
    {
        // Card.qss box model, resolved to the frame widths QStyleSheetStyle realized
        // (read off the legacy capture, 2026-09-23). Negative values are what the
        // negative qss padding produced and are kept as-is.
        if (!option || !widget)
        {
            return QRect();
        }
        const QRect rect = option->rect;

        if (auto card = qobject_cast<const Card*>(widget))
        {
            // Card: border-image 5/7/9/7, padding -4/-5/-4/-5, margin-bottom 4 -> (2, 1, 2, 9)
            // ContainerCard / SectionCard: border none, same padding          -> (-5, -4, -5, 0)
            // Card[hideFrame=true]: border 1px transparent, same padding     -> (-4, -3, -4, 1)
            // TOKENIZE: CardShadowInset
            if (cardHasFlatFrame(card))
            {
                return rect.adjusted(-5, -4, 5, 0);
            }
            if (card->property("hideFrame").toBool())
            {
                return rect.adjusted(-4, -3, 4, -1);
            }
            return rect.adjusted(2, 1, -2, -9);
        }

        const QWidget* parent = widget->parentWidget();
        if (widget->objectName() == QLatin1String("contentContainer"))
        {
            // #contentContainer: padding 8/7/7/7 + 1px border (no top) -> (8, 8, 8, 8)
            // ContainerCard / SectionCard body: no border               -> (7, 8, 7, 7)
            // TOKENIZE: CardBodyInset
            if (auto card = qobject_cast<const Card*>(parent))
            {
                return cardHasFlatFrame(card) ? rect.adjusted(7, 8, -7, -7) : rect.adjusted(8, 8, -8, -8);
            }
        }
        else if (widget->objectName() == QLatin1String("Background"))
        {
            // CardHeader #Background: padding 2/7/3/7 + 1px border (no bottom) -> (8, 3, 8, 3)
            // .secondaryCardHeader #Background: padding 2/0/2/0, no border      -> (0, 2, 0, 2)
            // TOKENIZE: CardHeaderInset
            if (auto header = qobject_cast<const CardHeader*>(parent))
            {
                return Style::hasClass(header, QStringLiteral("secondaryCardHeader"))
                    ? rect.adjusted(0, 2, 0, -2)
                    : rect.adjusted(8, 3, -8, -3);
            }
        }

        return QRect();
    }

    void Card::paintEvent(QPaintEvent* event)
    {
        if (!StyleManager::stylesheetsDisabled())
        {
            QFrame::paintEvent(event);
            return;
        }

        // Card.qss, one layer:
        //   Card: border-image shadow.png sliced 5/7/9/7 over the box left after the
        //         4px bottom margin (ContainerCard / SectionCard / hideFrame: none)
        //   #contentContainer: #555555 with its 1px same-colour border, bottom corners
        //         2px (ContainerCard / SectionCard: transparent)
        //   Card[selected=true] > #contentContainer: 1px #94D2FF border, no top edge
        // TOKENIZE: CardShadowImage, CardBodyColor, CardCornerRadius, CardSelectedBorderColor
        const bool flatFrame = cardHasFlatFrame(this);
        QPainter painter(this);

        if (!flatFrame && !property("hideFrame").toBool())
        {
            static const QPixmap shadow(QStringLiteral(":/Cards/img/UI20/Cards/shadow.png"));
            const QMargins shadowSlices(7, 5, 7, 9); // left, top, right, bottom
            qDrawBorderPixmap(&painter, rect().adjusted(0, 0, 0, -4), shadowSlices, shadow);
        }

        if (!flatFrame && !m_contentContainer->isHidden())
        {
            const QRectF body(m_contentContainer->geometry());
            const qreal radius = 2.0;
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0x55, 0x55, 0x55));
            painter.drawRoundedRect(body, radius, radius);
            painter.drawRect(body.adjusted(0, 0, 0, -radius)); // square top corners

            if (m_selected)
            {
                const QRectF stroke = body.adjusted(0.5, 0.5, -0.5, -0.5);
                QPainterPath outline;
                outline.moveTo(stroke.left(), stroke.top());
                outline.lineTo(stroke.left(), stroke.bottom() - radius);
                outline.arcTo(QRectF(stroke.left(), stroke.bottom() - 2 * radius, 2 * radius, 2 * radius), 180, 90);
                outline.lineTo(stroke.right() - radius, stroke.bottom());
                outline.arcTo(QRectF(stroke.right() - 2 * radius, stroke.bottom() - 2 * radius, 2 * radius, 2 * radius), 270, 90);
                outline.lineTo(stroke.right(), stroke.top());
                painter.setPen(QPen(QColor(0x94, 0xD2, 0xFF), 1.0));
                painter.setBrush(Qt::NoBrush);
                painter.drawPath(outline);
            }
        }
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

