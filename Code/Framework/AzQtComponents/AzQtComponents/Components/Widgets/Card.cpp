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
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/StyleHelpers.h>
#include <QEvent>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QRegion>
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

    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace CardInternal
    {
        // ---- The body of a card: #contentContainer, Card.qss:21-33 and :41-44, and its selected state, :35-39
        // TOKENIZE: CardBodyColor
        constexpr QColor BodyColor(0x55, 0x55, 0x55);                   // Card.qss:23, :25, :43 background-color, border-color
        // TOKENIZE: CardSelectedBorderColor
        constexpr QColor BodySelectedBorderColor(0x94, 0xD2, 0xFF);     // Card.qss:37
        // TOKENIZE: CardBodyBorderWidth
        constexpr int BodyBorderWidth = 1;                              // Card.qss:25, :37 (no top edge: :26, :38)
        // TOKENIZE: CardCornerRadius
        constexpr int BodyBottomRadius = 2;                             // Card.qss:31-32

        // ---- The title of a header (flattened GUI)
        // Card.qss:172-175 CardHeader[contentModified=true] * { color: #E57829 }. Card.cpp:443
        // TOKENIZE: CardModifiedTextColor
        constexpr QColor TitleModifiedColor(0xE5, 0x78, 0x29);
        // Card.qss:272 AzQtComponents--Card QLabel:disabled { color: #999999 }. Card.cpp:454
        // TOKENIZE: CardDisabledTextColor
        constexpr QColor TitleDisabledColor(0x99, 0x99, 0x99);

        //! The body of a card (flattened GUI), in `body`, the rect of its container, as the container's own rule painted it
        //! (QRenderRule::drawRule, from QStyleSheetStyle::drawPrimitive PE_Widget): the fill of the path through the middle of the border,
        //! antialiased, then the border over it (QRenderRule::drawBackground, drawBorder)
        void paintBody(QPainter& painter, const QRect& body, bool selected)
        {
            const BoxElement::Edge none{ 0, QColor() };
            const BoxElement::Edge edge{ BodyBorderWidth, selected ? BodySelectedBorderColor : BodyColor };
            const BoxElement::Edges edges{ none, edge, edge, edge };
            const BoxElement::Radii radii{ 0, 0, BodyBottomRadius, BodyBottomRadius };
            BoxElement::drawFrame(&painter, body, BodyColor, edges, radii);
        }

        //! The body of a card, painted by its container (flattened GUI). The legacy container was a styled frame that painted its own rule
        //! after the header, a widget below it in the card, and a card that paints its body for the container paints it before the header
        //! instead: where the two meet on a device pixel, as they do when the header ends on a half pixel (a device pixel ratio of 1.5),
        //! the pixel is the blend of the header over the body and not of the body over the header. The two overlap in a card whose frame
        //! is hidden too: the padding of -3px at the top (Card.qss:14, :48) puts the header out of its box, over the first rows of the
        //! body.
        class BodyPainter
            : public QObject
        {
        public:
            BodyPainter(const Card* card, QFrame* container)
                : QObject(container)
                , m_card(card)
                , m_container(container)
            {
                container->installEventFilter(this);
            }

            bool eventFilter(QObject* watched, QEvent* event) override
            {
                if (watched == m_container && event->type() == QEvent::Paint && StyleManager::flatStylingActive()
                    && !cardHasFlatFrame(m_card))
                {
                    QPainter painter(m_container);
                    paintBody(painter, m_container->rect(), m_card->isSelected());
                }
                return false;
            }

        private:
            const Card* m_card;
            QFrame* m_container;
        };
    } // namespace CardInternal

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
        // (an inline sheet of every card header, for the legacy GUI only: the flattened GUI pads the tooltips of a card header in
        // Style, SE_ShapedFrameContents, through Card::toolTipPadding)
        StyleManager::setLegacyStyleSheet(
            m_header, QStringLiteral("QToolTip { padding: %1px; }").arg(defaultConfigValues.toolTipPaddingInPixels));

        m_rootLayout = new QVBoxLayout(this);
        m_rootLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_rootLayout->setContentsMargins(QMargins(0, 0, 0, 0));
        m_rootLayout->addWidget(m_header);

        m_contentContainer = new QFrame(this);
        m_contentContainer->setObjectName(QStringLiteral("contentContainer"));
        if (StyleManager::flatStylingActive())
        {
            new CardInternal::BodyPainter(this, m_contentContainer);
        }

        m_mainLayout = new QVBoxLayout(m_contentContainer);
        m_mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_mainLayout->setContentsMargins(QMargins(0, 0, 0, 0));

        m_rootLayout->addWidget(m_contentContainer);

        // add a placeholder, so that the ordering of widgets in Cards works
        m_contentWidget = new QWidget(this);
        m_mainLayout->addWidget(m_contentWidget);


        // #SeparatorContainer { margin: 0; padding: 0 } (Card.qss:266-270) is the QFrame default: no box to set here
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

            // The selection is painted by this card and by its header (Card::paintEvent, CardHeader::paintEvent). selectedChanged
            // repolishes the card and each widget below it, and paints the card again as the legacy repolish did
            // (StyleHelpers::repolishWhenPropertyChanges)
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

                // .separator { color: #999999 } on the QFrame::HLine (Plain, line width 1: QFrame's defaults) is the
                // frame element Global.Frame.Separator (FrameElement.cpp), resolved when the class tag is added
            }

            style->repolishOnSettingsChange(card);
            polished = true;
        }
        else if (auto cardHeader = qobject_cast<CardHeader*>(widget))
        {
            CardHeader::setIconSize(config.headerIconSizeInPixels);

            if (StyleManager::stylesheetsDisabled())
            {
                // Card.qss header, one layer (Card.qss:80-84: margin 0, padding 0, the QFrame default). The header fill is painted by
                // CardHeader::paintEvent; #Background stays transparent and gets its
                // 2/7/3/7 padding + 1px border as the frame box (8, 3, 8, 3) from
                // Card::shapedFrameContents. The per-child margins (#Expander, #Icon,
                // #WarningIcon, #Help) are the boxes of the Global elements of
                // PropertyEditorElements.cpp: the layout has no spacing, as in Native
                // (CardHeader.cpp:152).
                cardHeader->m_backgroundFrame->setAutoFillBackground(false);
                cardHeader->m_backgroundLayout->setContentsMargins(0, 0, 0, 0);

                // Card.qss:86-89 `CardHeader * { background-color: #555555 }` fills every widget below the header that no rule of its
                // own outranks it on. Each named part has one (#Background Card.qss:98, #Title :206, #Icon and #WarningIcon :221, the
                // expander CheckBox.qss:121-123, the flat buttons PushButton.qss:48-52), so those need nothing here. A frame that
                // names none (the background of EMotionFX's RagdollCardHeader) is filled by the box element CardHeaderFrame
                // (PropertyEditorElements.cpp).

                // #Title { font-weight: bold }, normal inside a palette card: TextElement CardTitle, applied when
                // each #Title label of the header is polished (the title and the hidden label its ElidingLabel
                // measures the size hint with; ElidingLabel::setObjectName names both)

                // Card.qss:172-175 CardHeader[contentModified=true] * { color: #E57829 }
                QPalette titlePalette;
                if (cardHeader->isContentModified())
                {
                    titlePalette.setColor(QPalette::WindowText, CardInternal::TitleModifiedColor);
                }
                // Card.qss:272 `AzQtComponents--Card QLabel:disabled { color: #999999 }` (two type selectors and a pseudo-class: it outranks the
                // line above, an attribute, a type and `*`): the title of a disabled card, mock disabled or not, is #999999 either way. The
                // title is polished before its header sits in a card (ElidingLabel::elide polishes it while the header is built), so the
                // polish of the label (Style's applyFlattenedDisabledText) did not find the card, and this palette replaces whatever it set.
                if (Selectors::withinClass(cardHeader, "AzQtComponents::Card"))
                {
                    for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
                    {
                        titlePalette.setColor(QPalette::Disabled, role, CardInternal::TitleDisabledColor);
                    }
                }
                cardHeader->m_titleLabel->setPalette(titlePalette);

                // #ContextMenu / #ContextMenuPlusIcon / #Help: qproperty-icon, iconSize,
                // flat, max-width/max-height are the Global elements' too, applied
                // when each button is polished (PropertyEditorElements.cpp).
            }
            else
            {
                // Installing this per-widget sheet would re-interpose QStyleSheetStyle
                // over the flattened chain, so it is qss-mode only.
                QString newStyleSheet = QStringLiteral("QToolTip { padding: %1px; }").arg(config.toolTipPaddingInPixels);
                if (newStyleSheet != cardHeader->styleSheet())
                {
                    StyleManager::setLegacyStyleSheet(cardHeader, newStyleSheet);
                }
            }

            cardHeader->configSettingsChanged();

            style->repolishOnSettingsChange(cardHeader);

            polished = true;
        }

        return polished;
    }

    int Card::toolTipPadding(const QWidget* owner, const Config& config)
    {
        // The legacy style sheet style resolved the rules of a tooltip through the widget it is shown for (QTipLabel::placeTip,
        // `_q_stylesheet_parent`) and the widgets above it, so the sheet of a header reached the tips of the header and of every widget
        // in it. Every CardHeader had the sheet: the primary one (the constructor) and any other when it was polished. The tip is not
        // below the widget it is shown for in the flattened GUI (Qt hangs it on none on Windows): ToolTipElement::ownerOf is.
        const bool inHeader = owner->inherits("AzQtComponents::CardHeader") || Selectors::withinClass(owner, "AzQtComponents::CardHeader");
        return inHeader ? config.toolTipPaddingInPixels : 0;
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
            // Card: border-image 5/7/9/7, padding -4/-5/-4/-5, margin-bottom 4 -> (2, 1, 2, 9)     Card.qss:10-19
            // ContainerCard / SectionCard: border none, same padding          -> (-5, -4, -5, 0)   Card.qss:115-119, Card.qss:134-138
            // Card[hideFrame=true]: border 1px transparent, same padding     -> (-4, -3, -4, 1)    Card.qss:46-49
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
            // #contentContainer: padding 8/7/7/7 + 1px border (no top) -> (8, 8, 8, 8)             Card.qss:21-33
            // ContainerCard / SectionCard body: no border               -> (7, 8, 7, 7)            Card.qss:121-125, Card.qss:140-144
            // TOKENIZE: CardBodyInset
            if (auto card = qobject_cast<const Card*>(parent))
            {
                return cardHasFlatFrame(card) ? rect.adjusted(7, 8, -7, -7) : rect.adjusted(8, 8, -8, -8);
            }
        }
        else if (widget->objectName() == QLatin1String("Background"))
        {
            // CardHeader #Background: padding 2/7/3/7 + 1px border (no bottom) -> (8, 3, 8, 3)     Card.qss:91-99
            // Card[selected=true][expanded=false] > CardHeader #Background: border 1px on all four edges, bottom too
            //                                                                   -> (8, 3, 8, 4)     Card.qss:109-113
            // .secondaryCardHeader #Background: padding 2/0/2/0, no border      -> (0, 2, 0, 2)    Card.qss:166-170
            // TOKENIZE: CardHeaderInset
            if (auto header = qobject_cast<const CardHeader*>(parent))
            {
                if (Style::hasClass(header, QStringLiteral("secondaryCardHeader")))
                {
                    return rect.adjusted(0, 2, 0, -2);
                }

                // The frame reads this box when it is polished and when its style changes, as a QFrame does (QFramePrivate::
                // updateFrameWidth): a card selected and collapsed from the start has the extra pixel, one that becomes so later keeps
                // the box it had, as it did under the style sheet (its repolish sends neither event)
                const auto* card = qobject_cast<const Card*>(header->parentWidget());
                const bool selectedAndCollapsed = card && card->header() == header && card->isSelected() && !card->isExpanded();
                return selectedAndCollapsed ? rect.adjusted(8, 3, -8, -4) : rect.adjusted(8, 3, -8, -3);
            }
        }

        return QRect();
    }

    void Card::paintEvent(QPaintEvent* event)
    {
        // Not the flattened painting at the passthrough rung either: the frame as the style draws it
        if (!StyleManager::flatStylingActive())
        {
            QFrame::paintEvent(event);
            return;
        }

        // Card.qss, one layer:
        //   Card (Card.qss:10-19): border-image shadow.png sliced 5/7/9/7 over the box left after the
        //         4px bottom margin (ContainerCard Card.qss:115-119 / SectionCard Card.qss:134-138: none)
        //   Card[hideFrame=true] (Card.qss:46-49): the same border image, over borders of 1px: `border` sets the widths and the style
        //         and leaves the image, which the four 1px edges then show squashed (QRenderRule::fixupBorder keeps the widths of a rule
        //         that has a border image)
        //   #contentContainer (Card.qss:21-33, Card.qss:41-44; ContainerCard Card.qss:121-125 / SectionCard Card.qss:140-144:
        //         transparent) and its selected state (Card.qss:35-39) are the container's own rule: CardInternal::BodyPainter paints
        //         them when the container is painted, after the header, as the container did
        // TOKENIZE: CardShadowImage, CardHiddenFrameBorderWidth
        const bool flatFrame = cardHasFlatFrame(this);
        QPainter painter(this);

        if (!flatFrame)
        {
            static const QPixmap shadow(QStringLiteral(":/Cards/img/UI20/Cards/shadow.png"));
            const QMargins shadowSlices(7, 5, 7, 9); // left, top, right, bottom (Card.qss:18 border-image cuts, Card.qss:17 border widths)
            constexpr int hiddenFrameBorderWidth = 1; // Card.qss:48 border: 1px
            const QMargins shadowEdges = property("hideFrame").toBool()
                ? QMargins(hiddenFrameBorderWidth, hiddenFrameBorderWidth, hiddenFrameBorderWidth, hiddenFrameBorderWidth)
                : shadowSlices;

            // The border image is drawn smoothed (QRenderRule::drawBorderImage), which shows wherever it is scaled: its centre, and all
            // of it at a device pixel ratio above 1
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
            qDrawBorderPixmap(&painter, rect().adjusted(0, 0, 0, -4), shadowEdges, shadow, shadow.rect(), shadowSlices);
            painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        }
    }

    QPixmap Card::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* option, const QWidget* widget, const Config& config)
    {
        const auto* iconWidget = widget ? widget : qobject_cast<QWidget*>(option->styleObject);

        // The guard below is for the legacy rung. The legacy GUI never ran it: QIcon asks the application's style for the icon of a mode
        // other than Normal with a bare option (QApplicationPrivate::applyQIconStyleHelper, QStyleOption(0): no styleObject), and the
        // sheet style draws the label of a push button itself (QStyleSheetStyle::drawControl, CE_PushButtonLabel: ParentStyle::drawControl,
        // not this Style's), so this Style was drawing no widget at that moment and iconWidget was null: PushButton::generatedIconPixmap
        // recolored the Help and ContextMenu icons of a disabled card (iconButton.disabledColor #999999, PushButtonConfig.ini:49) and of
        // a focused button (activeColor). The flattened Style draws the label itself, so iconWidget is the button, and the guard would
        // keep them white. T-CD.3 (FieldCardTests) compares the pixels of both GUIs.
        if (!StyleManager::stylesheetsDisabled() && CardHeader::isCardHeaderMenuButton(iconWidget))
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

