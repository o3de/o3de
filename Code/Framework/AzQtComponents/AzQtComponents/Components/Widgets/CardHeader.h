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
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>

#include <QFrame>
#include <QIcon>
#include <QString>
#endif

class QCheckBox;
class QHBoxLayout;
class QVBoxLayout;
class QPushButton;
class QLabel;

namespace AzQtComponents
{
    namespace Internal
    {
        class RectangleWidget;

        // Clickable icon label
        class AZ_QT_COMPONENTS_API ClickableIconLabel
            : public QLabel
        {
            Q_OBJECT

        public:
            explicit ClickableIconLabel(QWidget* parent = nullptr);

            void SetClickable(bool clickable);
            bool GetClickable() const;

        Q_SIGNALS:
            void clicked(const QPoint& position);

        protected:
            void mouseReleaseEvent(QMouseEvent* event) override;

        private:
            bool m_clickable = false;
        };
    }

    //! Header bar for Card widgets.
    //! Provides a bar with an expander arrow, a text title and a button to trigger a context menu.
    //! Also has an optional icon and help button. 
    //! Sub widgets are hidden by default and will show once they're configured via the appropriate setter.
    //! For example, setIcon will cause the icon widget to appear.
    class AZ_QT_COMPONENTS_API CardHeader
        : public QFrame
    {
        Q_OBJECT
        //! Enable warning styling.
        Q_PROPERTY(bool warning READ isWarning WRITE setWarning NOTIFY warningChanged)
        //! Enable read-only styling.
        Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly NOTIFY readOnlyChanged)
        //! Enable content modified styling.
        Q_PROPERTY(bool contentModified READ isContentModified WRITE setContentModified NOTIFY contentModifiedChanged)
    public:
        static int s_iconSize;

        //! Enum used to determine which icon to use for the context menu button.
        enum ContextMenuIcon
        {
            Standard,   //!< Hamburger menu button.
            Plus        //!< Plus button, usually tied to add actions.
        };

        static void applyContainerStyle(CardHeader* header);
        static void applySectionStyle(CardHeader* header);

        CardHeader(QWidget* parent = nullptr);

        //! Sets the Card Header title. Passing an empty string will hide the Card Header.
        void setTitle(const QString& title);

        //! Sets the tool tip for the card header and card header title.
        void setTitleToolTip(const QString& toolTip);

        //! Returns the current title.
        QString title() const;
        //! Returns a direct pointer to the title label.
        AzQtComponents::ElidingLabel* titleLabel() const;

        //! Sets the filter string. If the title contains the filter string, it will be highlighted.
        //! Used to enhance searches.
        void setFilter(const QString& filterString);

        //! Sets a custom property to the title label.
        //! Can be used to 
        void setTitleProperty(const char *name, const QVariant &value);
        //! Forces a repaint of the title.
        //! Can be used to refresh the widget after style or property changes are applied.
        void refreshTitle();

        //! Sets the icon. Passing a null icon will hide the current icon.
        void setIcon(const QIcon& icon);

        //! Sets a secondary icon to be drawn on top of the main icon.
        void setIconOverlay(const QIcon& icon);

        //! Set whether the icon can be clicked to trigger an event.
        void setIconClickable(bool clickable);
        //! Returns true if the icon can be clicked to trigger an event, false otherwise.
        bool isIconClickable() const;

        //! Set whether the header displays an expander button.
        //! Note that the header itself will not change size or hide, it simply causes
        //! the onExpanderChanged signal to fire.
        void setExpandable(bool expandable);
        //! Returns true if the Card Header is showing and expander button, false otherwise.
        bool isExpandable() const;

        //! Sets the parent Card's expanded state.
        void setExpanded(bool expanded);
        //! Returns the parent Card's expanded state.
        bool isExpanded() const;

        //! Sets the warning state on the Card Header.
        void setWarning(bool warning);
        //! Returns true if the Card Header's warning state is set, false otherwise.
        bool isWarning() const;
        //! Sets a new QIcon for the warning state.
        void setWarningIcon(const QIcon& icon);

        //! Sets the read only state on the Card Header.
        void setReadOnly(bool readOnly);
        //! Returns true if the Card Header's read only state is set, false otherwise.
        bool isReadOnly() const;

        //! Sets the modified state on the Card Header.
        void setContentModified(bool modified);
        //! Returns true if the Card Header's modified state is set, false otherwise.
        bool isContentModified() const;

        //! Sets whether the header has a context menu widget.
        //! This determines whether or not the contextMenuRequested signal fires on right-click, or when
        //! the context menu button is pressed.
        void setHasContextMenu(bool showContextMenu);

        //! Sets the help url on the Card Header.
        //! If a url is set, a question mark icon will be displayed on the right and it will open the
        //! default browser on the url provided on click.
        void setHelpURL(const QString& url);
        //! Resets the help url and hides the question mark button.
        void clearHelpURL();
        //! Returns the help url for this Card Header if set.
        QString helpURL() const;

        //! Forces a refresh of the icon and warning icon on the Card Header.
        void configSettingsChanged();

        //! Sets the mock disabled state.
        //! This will make the Card Header look disabled, but the buttons will still work.
        void mockDisabledState(bool disabled);

        //! Sets the size of the Card header's icon.
        static void setIconSize(int iconSize);

        //! Returns the default size for the Card Header's icon.
        static int defaultIconSize();

        //! Sets the icon to be displayed for the context menu.
        void setContextMenuIcon(ContextMenuIcon iconType);

        //! Sets the small solid color underline under the header. If color is
        //! invalid or transparent, the underline will disappear completely.
        void setUnderlineColor(const QColor& color);

    Q_SIGNALS:
        //! Triggered when the context menu button is clicked, or on a right click.
        void contextMenuRequested(const QPoint& position);
        //! Triggered when the expander state is changed.
        void expanderChanged(bool expanded);
        //! Triggered when the warning state is changed.
        void warningChanged(bool warning);
        //! Triggered when the read only state is changed.
        void readOnlyChanged(bool readOnly);
        //! Triggered when the content modified state is changed.
        void contentModifiedChanged(bool modified);
        //! Triggered when the icon label is clicked.
        void iconLabelClicked(const QPoint& position);

    protected:
        friend class Card;

        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;
        void triggerContextMenuUnderButton();
        void triggerHelpButton();
        void triggerIconLabelClicked(const QPoint& position);

        //! Assign a pixmap for the icon label based on current properties
        void updateIconLabel();

        static bool isCardHeaderIcon(const QWidget* widget);
        static bool isCardHeaderMenuButton(const QWidget* widget);

        // Widgets in header
        QVBoxLayout* m_mainLayout = nullptr;
        QHBoxLayout* m_backgroundLayout = nullptr;
        QFrame* m_backgroundFrame = nullptr;
        QCheckBox* m_expanderButton = nullptr;
        Internal::ClickableIconLabel* m_iconLabel = nullptr;
        AzQtComponents::ElidingLabel* m_titleLabel = nullptr;
        QLabel* m_warningLabel = nullptr;
        QPushButton* m_contextMenuButton = nullptr;
        QPushButton* m_helpButton = nullptr;
        Internal::RectangleWidget* m_underlineWidget = nullptr;
        bool m_warning = false;
        bool m_readOnly = false;
        bool m_modified = false;

        QIcon m_warningIcon;
        QIcon m_icon;
        QIcon m_iconOverlay;
        QString m_helpUrl;

        
    };
} // namespace AzQtComponents
