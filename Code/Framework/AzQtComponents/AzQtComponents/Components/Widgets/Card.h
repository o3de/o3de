/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QFrame>
#include <QIcon>
#include <QMargins>
#include <QPoint>
#include <QString>
#include <QVector>
#endif

class QPoint;
class QSettings;
class QStyleOption;
class QVBoxLayout;

namespace AzQtComponents
{
    class CardHeader;
    class CardNotification;
    class Style;

    //! A container for other widgets that provides a title bar and can be expanded/collapsed or
    //! dragged around.
    //! To make use of a Card, set its title and a content widget. The content widget will be
    //! automatically shown and hidden as the Card is expanded and collapsed.
    //! Beyond that, a Card can also include a secondary expandable widget, for features such as
    //! Advanced Settings, and multiple "Notifications" which are labels appended to the bottom
    //! that can optionally include "feature" widgets, for example as a button to allow the user to
    //! respond to the Notification.
    //! Style information is stored in Card.qss, for Card, CardHeader and CardNotification objects.
    //! Config information is stored in CardConfig.ini
    class AZ_QT_COMPONENTS_API Card
        : public QFrame
    {
        Q_OBJECT
        //! Card selected state. Selected cards get a blue border.
        Q_PROPERTY(bool selected READ isSelected WRITE setSelected NOTIFY selectedChanged)
        //! Card expanded state. Collapsing a Card disables the widgets it contains.
        Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandStateChanged)
    public:
        //! Style configuration for the Card class.
        struct Config
        {
            int toolTipPaddingInPixels; //!< Padding for tooltips in pixels. All directions get the same padding.
            int headerIconSizeInPixels; //!< Size of the icon in pixels. Icons must be square.
            int rootLayoutSpacing;      //!< Spacing value for the main layout of the card. All directions get the same spacing.
            QString warningIcon;        //!< Path to the icon. Svg images recommended.
            QSize warningIconSize;      //!< Size of the warning icon. Size must be proportional to the icon's ratio.
            qreal disabledIconAlpha;    //!< Alpha value for disabled icons. Must be a value between 0.0 and 1.0.
        };

        static void applyContainerStyle(Card* card);
        static void applySectionStyle(Card* card);

        Card(QWidget* parent = nullptr);

        //! Sets the Primary Content Widget for this Card.
        void setContentWidget(QWidget* contentWidget);
        //! Returns a pointer to the Primary Content Widget for this Card.
        QWidget* contentWidget() const;

        //! Sets the Card's Title.
        //! Equivalent to calling setTitle on the CardHeader.
        void setTitle(const QString& title);
        //! Sets the tool tip for the card header and card header title.
        //! Equivalent to calling setTitle on the CardHeader.
        void setTitleToolTip(const QString& toolTip);
        //! Returns the Card's Title.
        QString title() const;

        //! Returns a pointer to thisCard's CardHeader
        CardHeader* header() const;

        //! Appends a notification to the Card with the message provided.
        CardNotification* addNotification(const QString& message);
        //! Clears all notifications on this Card.
        void clearNotifications();
        //! Returns the number of notifications currently attached to this Card.
        int getNotificationCount() const;

        //! Set the Card's expanded state.
        void setExpanded(bool expand);
        //! Returns the Card's expanded state.
        bool isExpanded() const;

        //! Sets the Card's selected state.
        //! Will trigger the selectedChanged() signal.
        void setSelected(bool selected);
        //! Returns the Card's selected state.
        bool isSelected() const;

        //! Marks the Card as "being dragged".
        //! Note: there is no behavior associated to this function, it's just a flag.
        void setDragging(bool dragging);
        //! Returns true if the Card is marked as "being dragged".
        bool isDragging() const;

        //! Marks the Card as "drop target".
        //! Note: there is no behavior associated to this function, it's just a flag.
        void setDropTarget(bool dropTarget);
        //! Returns true if the Card is marked as "drop target".
        bool isDropTarget() const;

        //! Set the expanded state for the Secondary Widget.
        void setSecondaryContentExpanded(bool expand);
        //! Returns the expanded state for the Secondary Widget.
        bool isSecondaryContentExpanded() const;

        //! Sets the Title for the secondary section of the Card.
        void setSecondaryTitle(const QString& secondaryTitle);
        //! Gets the Title of the secondary section of the Card.
        QString secondaryTitle() const;

        //! Sets the Title for the secondary section of the Card.
        void setSecondaryContentWidget(QWidget* secondaryContentWidget);
        //! Returns a pointer to the secondary widget of this Card.
        QWidget* secondaryContentWidget() const;

        //! Sets the hideFrame property on the Card, removing the border around it.
        void hideFrame();

        //! Shows the Card as disabled.
        //! Expand/collapse, help and context menu buttons will remain functional.
        void mockDisabledState(bool disable);

        //! Sets the Card style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the Card.
        static Config loadConfig(QSettings& settings);
        //! Gets the default Card style configuration.
        static Config defaultConfig();

    Q_SIGNALS:
        //! Triggered when the selected state for the Card changes.
        void selectedChanged(bool selected);
        //! Triggered when the Card is expanded/collapsed.
        void expandStateChanged(bool expanded);
        //! Triggered when the Card's secondary content is expanded/collapsed.
        void secondaryContentExpandStateChanged(bool expanded);
        //! Triggered when the menu button on the card is clicked and the context menu is shown.
        void contextMenuRequested(const QPoint& position);

    protected:
        // implemented this way so that AzToolsFramework::ComponentEditorHeader can still be used
        Card(CardHeader* customHeader, QWidget* parent = nullptr);

    private:
        bool hasSecondaryContent() const;
        void updateSecondaryContentVisibility();

        // methods used by Style
        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool unpolish(Style* style, QWidget* widget, const Config& config);
        static QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* option, const QWidget* widget, const Config& config);

        QVBoxLayout* m_rootLayout = nullptr;
        QFrame* m_contentContainer = nullptr;
        QWidget* m_contentWidget = nullptr;
        QWidget* m_secondaryContentWidget = nullptr;
        QFrame* m_separator = nullptr;
        QFrame* m_separatorContainer = nullptr;
        CardHeader* m_header = nullptr;
        CardHeader* m_secondaryHeader = nullptr;
        QVBoxLayout* m_mainLayout = nullptr;
        bool m_expanded = true;
        bool m_selected = false;
        bool m_dragging = false;
        bool m_dropTarget = false;
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'AzQtComponents::Card::m_notifications': class 'QVector<AzQtComponents::CardNotification *>' needs to have dll-interface to be used by clients of class 'AzQtComponents::Card'
        QVector<CardNotification*> m_notifications;
        AZ_POP_DISABLE_WARNING
        QIcon m_warningIcon;
        QSize m_warningIconSize;
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'AzQtComponents::Card::m_separatorMargins': class 'QMargins' needs to have dll-interface to be used by clients of class 'AzQtComponents::Card'
        QMargins m_separatorMargins;
        AZ_POP_DISABLE_WARNING

        friend class Style;
    };
} // namespace AzQtComponents
