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
#endif

class QBoxLayout;
class QStackedLayout;

namespace AzQtComponents
{
    class SegmentBar;

    //! A container that uses a SegmentBar to switch between different sections of associated information.
    class AZ_QT_COMPONENTS_API SegmentControl : public QFrame
    {
        Q_OBJECT
        
        //! Orientation of the segment bar. Horizontal bars are ordered left to right, vertical ones go top to bottom.
        Q_PROPERTY(Qt::Orientation tabOrientation READ tabOrientation WRITE setTabOrientation)
        //! Position of the content related to the segment bar.
        Q_PROPERTY(SegmentControl::TabPosition tabPosition READ tabPosition WRITE setTabPosition)
        //! Index of the currently selected segment.
        Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
        //! The number of tabs in the SegmentControl.
        Q_PROPERTY(int count READ count)
        //! Size of the icons in the SegmentControl.
        Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize)

    public:

        //! Enum listing the tab position relative to the segment bar.
        enum TabPosition
        {
            North,
            South,
            West,
            East
        };
        Q_ENUM(TabPosition)

        explicit SegmentControl(QWidget* parent = nullptr);
        explicit SegmentControl(SegmentControl::TabPosition position, QWidget* parent = nullptr);
        explicit SegmentControl(SegmentBar* bar, SegmentControl::TabPosition position, QWidget* parent = nullptr);

        //! Appends a content widget as a new tab.
        int addTab(QWidget* widget, const QString& text);
        //! Appends a content widget as a new tab with an icon.
        int addTab(QWidget* widget, const QIcon& icon, const QString& text);

        //! Inserts a content widget as a new tab at the given index.
        int insertTab(int index, QWidget* widget, const QString& text);
        //! Inserts a content widget as a new tab with an icon at the given index.
        int insertTab(int index, QWidget* widget, const QIcon& icon, const QString& text);

        //! Removes the tab at the given index.
        void removeTab(int index);
        //! Moves the tab from a position to another.
        //! The tabs between 'to' and 'from' will have their indices shifted to fill the void.
        void moveTab(int from, int to);

        //! Sets enabled state for the tab at the given index.
        void setTabEnabled(int index, bool enabled);
        //! Returns the enabled state for the tab at the given index.
        bool isTabEnabled(int index) const;

        //! Sets the text to the tab at the given index.
        void setTabText(int index, const QString& text);
        //! Returns the text of the tab at the given index.
        QString tabText(int index) const;

        //! Sets the icon to the tab at the given index.
        void setTabIcon(int index, const QIcon& icon);
        //! Returns the icon of the tab at the given index.
        QIcon tabIcon(int index) const;

#ifndef QT_NO_TOOLTIP
        //! Sets the tooltip text for the tab at the given index.
        void setTabToolTip(int index, const QString& tip);
        //! Returns the tooltip text of the tab at the given index.
        QString tabToolTip(int index) const;
#endif

#ifndef QT_NO_WHATSTHIS
        //! Sets the "What's this?" text to the tab at the given index.
        void setTabWhatsThis(int index, const QString& text);
        //! Returns the "What's this?" text of the tab at the given index.
        QString tabWhatsThis(int index) const;
#endif

        //! Sets the segment bar orientation.
        void setTabOrientation(Qt::Orientation orientation);
        //! Returns the segment bar orientation.
        Qt::Orientation tabOrientation() const;

        //! Sets the position of the content tabs relative to the segment bar.
        void setTabPosition(SegmentControl::TabPosition position);
        //! Returns the position of the content tabs relative to the segment bar.
        SegmentControl::TabPosition tabPosition() const;

        //! Returns the index of the currently selected tab.
        int currentIndex() const;
        //! Returns a pointer to the widget tied to the currently selected tab.
        QWidget* currentWidget() const;
        //! Returns a pointer to the widget at the index provided.
        QWidget* widget(int index) const;
        //! Returns the tab index of the widget provided, if found in this container.
        //! \return The index of the tab containing the widget, or -1 if not found.
        int indexOf(QWidget* widget) const;
        //! Returns the number of tabs.
        int count() const;

        //! Sets the maximum icon size.
        //! Smaller icons will not be scaled up, but bigger icons will be shrunk. The icon will be stretched to fit.
        void setIconSize(const QSize& size);
        //! Returns the icon size.
        QSize iconSize() const;

        //! Removes all tabs from the container.
        void clear();

        //! Returns a pointer to the underlying segment bar.
        SegmentBar* tabBar() const;

    public Q_SLOTS:
        // Sets the current tab to the index provided.
        void setCurrentIndex(int index);
        // Sets the current tab to the widget provided.
        void setCurrentWidget(QWidget* widget);

    Q_SIGNALS:
        //! Triggered when the current tab is changed.
        void currentChanged(int index);
        //! Triggered when a tab is clicked by the user.
        void tabBarClicked(int index);

    private:
        QBoxLayout* m_layout;
        SegmentBar* m_segmentBar;
        QStackedLayout* m_widgets;

        void initialize(Qt::Orientation tabOrientation, SegmentControl::TabPosition position);
        void initialize(SegmentBar* bar, SegmentControl::TabPosition position);
    };
}
