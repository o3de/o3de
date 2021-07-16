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

class QAbstractButton;
class QBoxLayout;
class QButtonGroup;

namespace AzQtComponents
{
    //! A control used to switch between different sections of associated information.
    class AZ_QT_COMPONENTS_API SegmentBar
        : public QFrame
    {
        Q_OBJECT
        //! Orientation of the segment bar. Horizontal bars are ordered left to right, vertical ones go top to bottom.
        Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
        //! Index of the currently selected segment.
        Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
        //! The number of tabs in the SegmentBar.
        Q_PROPERTY(int count READ count)
        //! Size of the icons in the SegmentBar.
        Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize)
    public:
        explicit SegmentBar(Qt::Orientation orientation, QWidget* parent = nullptr);
        explicit SegmentBar(QWidget* parent = nullptr);

        //! Appends a new tab.
        int addTab(const QString& text);
        //! Appends a new tab with an icon.
        int addTab(const QIcon& icon, const QString& text);

        //! Inserts a new tab at the given index.
        int insertTab(int index, const QString& text);
        //! Inserts a new tab with an icon at the given index.
        int insertTab(int index, const QIcon& icon, const QString& text);

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

        //! Sets the text color to the tab at the given index.
        void setTabTextColor(int index, const QColor& color);
        //! Returns the text color of the tab at the given index.
        QColor tabTextColor(int index) const;

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
        void setOrientation(Qt::Orientation orientation);
        //! Returns the segment bar orientation.
        Qt::Orientation orientation() const;

        //! Returns the index of the currently selected tab.
        int currentIndex() const;
        //! Returns the number of tabs.
        int count() const;

        //! Sets the maximum icon size.
        //! Smaller icons will not be scaled up, but bigger icons will be shrunk. The icon will be stretched to fit.
        void setIconSize(const QSize& size);
        //! Returns the icon size.
        QSize iconSize() const;

    public Q_SLOTS:
        // Sets the current tab.
        void setCurrentIndex(int index);

    Q_SIGNALS:
        //! Triggered when the current tab is changed.
        void currentChanged(int index);
        //! Triggered when a tab is clicked by the user.
        void tabBarClicked(int index);

    protected:
        virtual QAbstractButton* createButton(const QIcon& icon, const QString& text);
        QAbstractButton* button(int index) const;
        void updateTabs();

    private:
        QBoxLayout* m_alignmentLayout;
        QBoxLayout* m_buttonsLayout;
        QButtonGroup* m_buttons;
        QSize m_iconSize;
        int m_minimumTabSize;

        void initialize(Qt::Orientation orientation);
    };
}
