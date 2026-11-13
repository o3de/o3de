/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/Widgets/Internal/RectangleWidget.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleHelpers.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QIcon>
#include <QPainterPath>
#include <QPixmap>

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
                emit clicked(event->globalPosition().toPoint());
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

    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace CardHeaderInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        // ---- The box of #Background in a state: the edges and the corners the rules that reach it leave, in the cascade's order
        // Card.qss:95-96 `border: 1px solid transparent; border-bottom: none`
        // TOKENIZE: CardHeaderBorderWidth
        constexpr int IdleBorderWidth = 1;
        // Card.qss:103 `border: 1px solid #94D2FF; border-bottom: none` (:111 on the four edges)
        // TOKENIZE: CardSelectedBorderWidth, CardSelectedBorderColor
        constexpr int SelectedBorderWidth = 1;
        constexpr QColor SelectedBorderColor(0x94, 0xD2, 0xFF);
        // Card.qss:179-180 (warning) and :105-106 (selected) `border-top-left-radius: 2px; border-top-right-radius: 2px`, and :112
        // `border-radius: 2px` on the four corners
        // TOKENIZE: CardHeaderCornerRadius
        constexpr int TopCornerRadius = 2;

        // ---- The fill of the box of #Background by the class of the header
        // Card.qss:153-158 (.primaryCardHeader) and :127-132 (ContainerCardHeader) `background-color: #333333`. CardHeader.cpp:501
        // TOKENIZE: CardHeaderColor
        constexpr QColor HeaderColor(0x33, 0x33, 0x33);
        // Card.qss:160-164 (.secondaryCardHeader) `background-color: #555555`. CardHeader.cpp:501
        // TOKENIZE: CardSecondaryHeaderColor
        constexpr QColor SecondaryHeaderColor(0x55, 0x55, 0x55);

        // ---- The warning tile: Card.qss:184
        // TOKENIZE: CardWarningTile
        constexpr const char* WarningTileResource = ":/Cards/img/UI20/Cards/error-conclict-state.svg";

        //! What reaches the background frame of a header: its class and the state of the card it is the header of
        struct BackgroundLook
        {
            bool m_secondary = false;           //!< `.secondaryCardHeader #Background` (Card.qss:166-170): no border
            bool m_warning = false;             //!< `CardHeader[warning=true] #Background` (Card.qss:177-185)
            bool m_selected = false;            //!< `Card[selected=true] > CardHeader #Background` (Card.qss:101-107)
            bool m_collapsed = false;           //!< with `[expanded=false]` (Card.qss:109-113)
        };

        //! The edges and the corners of the background frame, by specificity and, for equal ones, the later rule: Card.qss:91-99,
        //! :166-170, :177-185, :101-107, :109-113 (0x101, 0x110, 0x111, 0x112, 0x122)
        struct BackgroundBox
        {
            BoxElement::Edges m_edges;
            BoxElement::Radii m_radii;
        };

        BackgroundBox backgroundBoxOf(const BackgroundLook& look)
        {
            const QColor invisible(Qt::transparent);
            const BoxElement::Edge idle{ IdleBorderWidth, invisible };
            const BoxElement::Edge selected{ SelectedBorderWidth, SelectedBorderColor };
            const BoxElement::Edge none{ 0, invisible };
            const BoxElement::Radii roundAtTheTop{ TopCornerRadius, TopCornerRadius, 0, 0 };
            BackgroundBox box;

            // Card.qss:95-96 (:168 for a secondary header: no border)
            box.m_edges = look.m_secondary ? BoxElement::Edges{ none, none, none, none } : BoxElement::Edges{ idle, idle, none, idle };

            // Card.qss:179-180
            if (look.m_warning)
            {
                box.m_radii = roundAtTheTop;
            }

            // Card.qss:103-106: the edges at the top and at the sides, the bottom still none
            if (look.m_selected)
            {
                box.m_edges = { selected, selected, none, selected };
                box.m_radii = roundAtTheTop;
            }

            // Card.qss:111-112: `border` sets the bottom edge too, and `border-radius` the bottom corners
            if (look.m_selected && look.m_collapsed)
            {
                box.m_edges = { selected, selected, selected, selected };
                box.m_radii = { TopCornerRadius, TopCornerRadius, TopCornerRadius, TopCornerRadius };
            }
            return box;
        }

        // ------------------------------------------------------------------------------
        // Helpers: painting, as QRenderRule paints the rule of the background frame
        // ------------------------------------------------------------------------------

        //! QRenderRule::drawBackgroundImage for an image that repeats in both directions from the top left of the border box
        //! (`background-origin: border; background-clip: border`, Card.qss:182-183): the file at its natural size
        //! (QStyleSheetStyle::loadPixmap), tiled by QPainter::drawTiledPixmap with the offset the sheet gives it, and clipped to the path
        //! through the middle of the border (QRenderRule::borderClip) on the painter as the style hands it over: the clip is not
        //! antialiased, and the tile is smoothed where it is scaled (QWidgetPrivate::drawWidget paints the background of a widget with
        //! SmoothPixmapTransform set)
        void drawWarningTile(QPainter& painter, const QRect& frame, const BackgroundBox& box)
        {
            const QPixmap tile = Style::cachedPixmap(QString::fromLatin1(WarningTileResource));
            if (tile.isNull())
            {
                return;
            }
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing, false);
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
            const QPainterPath clip = BoxElement::borderPath(frame, box.m_edges, box.m_radii);
            if (!clip.isEmpty())
            {
                painter.setClipPath(clip, Qt::IntersectClip);
            }
            const QSize size = tile.size() / tile.devicePixelRatio();
            painter.drawTiledPixmap(frame, tile, QPoint(size.width(), size.height()));
            painter.restore();
        }
    } // namespace CardHeaderInternal

    void CardHeader::paintEvent(QPaintEvent* event)
    {
        // Not the flattened painting at the passthrough rung either: the frame as the style draws it
        if (!StyleManager::flatStylingActive())
        {
            QFrame::paintEvent(event);
            return;
        }

        // Card.qss header box, one layer (the underline widget sits below the box):
        //   .primaryCardHeader (Card.qss:153-158): #333333, top corners 2px
        //   ContainerCardHeader (Card.qss:127-132): #333333, square
        //   .secondaryCardHeader (Card.qss:160-164): #555555 (same as the body), square
        //   SectionCardHeader (Card.qss:146-151): transparent
        //   [warning=true] #Background (Card.qss:177-185): error-conclict-state.svg tiled over the box
        //   Card[selected=true] > CardHeader #Background (Card.qss:101-107): 1px #94D2FF, top corners 2px
        //         (all four corners while the card is collapsed: Card.qss:109-113)
        // TOKENIZE: CardCornerRadius, CardWarningTile, CardSelectedBorderColor
        const bool sectionHeader = Style::hasClass(this, g_sectionCardHeaderClass);
        const bool containerHeader = Style::hasClass(this, g_containerCardHeaderClass);
        const bool secondaryHeader = Style::hasClass(this, QStringLiteral("secondaryCardHeader"));
        // ColorPicker.qss:51-54 PaletteCardBase CardHeader { background-color: transparent }
        const bool paletteHeader = Selectors::childOfClass(this, "AzQtComponents::PaletteCardBase");
        const qreal radius = (sectionHeader || containerHeader || secondaryHeader) ? 0.0 : 2.0;
        const QRectF box(m_backgroundFrame->geometry());

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);

        if (!sectionHeader && !paletteHeader)
        {
            using CardHeaderInternal::HeaderColor;
            using CardHeaderInternal::SecondaryHeaderColor;
            painter.setBrush(secondaryHeader ? SecondaryHeaderColor : HeaderColor);

            // One outline, painted once, as the style sheet filled one path (QRenderRule::drawBackground): a rounded rect and a rect
            // painted over it paint the rows both cover twice, and a path of both adds their coverage at an edge both lie on. Either
            // shows on an edge that falls between device pixels: the bottom of a header of 25px at ratio 1.5 (a card that is selected
            // and collapsed from the start has one, Card.qss:109-113). The corners at the top are round, the ones at the bottom square.
            QPainterPath fill;
            if (radius > 0.0)
            {
                fill.moveTo(box.left(), box.bottom());
                fill.lineTo(box.left(), box.top() + radius);
                fill.arcTo(QRectF(box.left(), box.top(), 2 * radius, 2 * radius), 180, -90);
                fill.lineTo(box.right() - radius, box.top());
                fill.arcTo(QRectF(box.right() - 2 * radius, box.top(), 2 * radius, 2 * radius), 90, -90);
                fill.lineTo(box.right(), box.bottom());
                fill.closeSubpath();
            }
            else
            {
                fill.addRect(box);
            }
            painter.drawPath(fill);
        }

        // The background frame's own rule, over the fill: the warning tile, then the selected outline. They are the rule of the state of
        // the card (CardHeaderInternal::backgroundBoxOf), whatever the class of the header: the rules name #Background, not the header.
        const auto* card = qobject_cast<Card*>(parentWidget());
        const bool selectedHeader = card && card->isSelected() && card->header() == this;
        CardHeaderInternal::BackgroundLook look;
        look.m_secondary = secondaryHeader;
        look.m_warning = m_warning;
        look.m_selected = selectedHeader;
        look.m_collapsed = selectedHeader && !card->isExpanded();
        const CardHeaderInternal::BackgroundBox background = CardHeaderInternal::backgroundBoxOf(look);
        const QRect frame = m_backgroundFrame->geometry();

        if (m_warning)
        {
            CardHeaderInternal::drawWarningTile(painter, frame, background);
        }

        if (selectedHeader)
        {
            // drawn as the style sheet style draws a border (qDrawBorder)
            BoxElement::drawEdges(&painter, frame, background.m_edges, background.m_radii);
        }
    }

    void CardHeader::setUnderlineColor(const QColor& color)
    {
        m_underlineWidget->setColor(color);

        const bool underlineVisible = color.isValid() && color.alpha() != 0;
        m_underlineWidget->setVisible(underlineVisible);
    }

} // namespace AzQtComponents

