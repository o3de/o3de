/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LOGPANEL_PANEL_H
#define LOGPANEL_PANEL_H

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/ring_buffer.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzToolsFramework/UI/Logging/LogLine.h>

#include <QSortFilterProxyModel>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QPainter::d_ptr': class 'QScopedPointer<QPainterPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QPainter'
#include <QLayout>
AZ_POP_DISABLE_WARNING
#include <QStyledItemDelegate>
#include <QAbstractTableModel>

#include <memory>
#endif

class QTableView;
class QLabel;
class QModelIndex;
class QAbstractItemModel;

//! You can use this to make logging panels in your application
//! The usual usage is to monitor traceprintfs, but it can also be used to construct logs which take files.
//! The main classes involved here are
//! BaseLogView (in LogControl.cpp) - a base view that only shows one log in its viewing area.  Its a table view.
//!   - A standard Qt control base class used for an individual view of a log
//!   - since its a Qt Model/View based control, it expects you to set a data source from which it will pull the log lines
//!
//! BaseLogPanel is the base GUI class for a tab-based UI (where you can have multiple tabs with filters)
//!   - A Qt GUI Widget which is used to create a GUI control with tabs and the ability to customize tabs.
//!   - you derive from this class and handle the AddTab() and other such messages
//!   - It can automatically save and restore state if you give it a unique storage ID.
//!
//! LogLine
//!   - The format for a generic log line that is used by the line-based log controls below.
//!   - This is basically the format you use to submit logs to the log control if you want to manually submit them.
//!
//! RingBufferLogDataModel
//!   - A model for the BaseLogView based on a ring buffer, which expires oldest entries when more than a maximum accumulate.
//!   - Used for live logging features (such as traceprintf logging in the GUI)  when it would be poor performance to accumulate forever
//!
//! ListLogDataModel
//!   - A model for the BaseLogView based on a flat list, which keeps all entries.
//!   - Used for forensic logging when you have a specific source of log such as a data file or a past capture, when you don't want to
//!   - Expire old entries, usually in the case when you know there are not going to be a flood of messages constantly.
//!
//! In general, unless you have a very specific use-case, you will instead create an instance of GenericLogPanel or TracePrintFLogPanel
//! and feed those controls log lines, instead of subclassing these.  The exception to this would be if you have some data source
//! that is not the trace bus and is not standard log files, in which case you would create your own Panel and Tab classes (or just
//! use a class derived from BaseLogView with your own data model.

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework
{
    namespace LogPanel
    {
        //! TabSettings is what you populate in order to custom-create a specific filtered tab in a log view.
        //! first, populate TabSettings, then submit it to AddLog on the base log control
        struct TabSettings final
        {
            AZ_RTTI(TabSettings, "{2A3D8781-0EF3-4c72-B362-412000CE1A6B}");
            AZStd::string m_window;
            AZStd::string m_tabName;
            AZStd::string m_textFilter;
            AZ::u32 m_filterFlags;

            enum FilterType
            {
                FILTER_NORMAL = 0,
                FILTER_WARNING,
                FILTER_ERROR,
                FILTER_DEBUG,
                LAST_FILTER
            };

            TabSettings() { }
            TabSettings(const AZStd::string& in_tabName, const AZStd::string& in_window, const AZStd::string& in_filter)
                : m_window(in_window)
                , m_tabName(in_tabName)
                , m_textFilter(in_filter)
                , m_filterFlags(0)
            {
                m_filterFlags |= (0x01 << FILTER_NORMAL);
                m_filterFlags |= (0x01 << FILTER_WARNING);
                m_filterFlags |= (0x01 << FILTER_ERROR);
                m_filterFlags |= (0x01 << FILTER_DEBUG);
            }
            TabSettings(const AZStd::string& in_tabName, const AZStd::string& in_window, const AZStd::string& in_filter, bool in_normal, bool in_warning, bool in_error, bool in_debug)
                : m_window(in_window)
                , m_tabName(in_tabName)
                , m_textFilter(in_filter)
                , m_filterFlags(0)
            {
                m_filterFlags |= in_normal ? (0x01 << FILTER_NORMAL) : 0;
                m_filterFlags |= in_warning ? (0x01 << FILTER_WARNING) : 0;
                m_filterFlags |= in_error ? (0x01 << FILTER_ERROR) : 0;
                m_filterFlags |= in_debug ? (0x01 << FILTER_DEBUG) : 0;
            }
        };

        //! BaseLogPanel is the base GUI class that has tabs (as opposed to a single view)
        //!    you derive from this class and handle the AddTab() and other such messages
        class BaseLogPanel
            : public QWidget
        {
            Q_OBJECT
        public:
            // class allocator intentionally removed so that QT can make us in their auto-gen code (from .ui files)
            //AZ_CLASS_ALLOCATOR(Panel, AZ::SystemAllocator);
            BaseLogPanel(QWidget* pParent = nullptr);
            virtual ~BaseLogPanel();

            //! SaveState uses the storageID to store which tabs were active and what their settings were
            void SaveState();
            //! LoadState reads which tabs were active last time.
            bool LoadState();

            // call this to manually add a tab.  Usually you load state instead, though, or you rely on user clicking the add button.
            void AddLogTab(const TabSettings& settings);

            virtual QSize minimumSize() const;
            virtual QSize sizeHint() const;

            //! SetStorageID is used to save and load state
            //! Give it a unique id (best using AZ_CRC) and it will remember its state next time it starts
            //! by calling SetStorageID next time, to the same number, and then calling LoadState().
            void SetStorageID(AZ::u32 id);

            int GetTabWidgetCount();
            QWidget* GetTabWidgetAtIndex(int index);

            static void Reflect(AZ::ReflectContext* reflection);

        Q_SIGNALS:
            void TabsReset(); // all tabs have been deleted because user clicked Reset
            void onLinkActivated(const QString& link);

        protected:
            // override this to create your tab class:
            virtual QWidget* CreateTab(const TabSettings& settings) = 0;

        private:
            struct Impl;
            std::unique_ptr<Impl> m_impl;

        private Q_SLOTS:
            void onTabClosed(int whichTab);
            void onAddClicked(bool checked);
            void onResetClicked(bool checked);
            void onCopyAllClicked();
        };

        //! this is a ringbuffer-based log panel model meant for the AZ::TraceBus, but could be used by any
        //! data source that can construct LogLine structures and feed them to AppendLine.  Since its a ring buffer,
        //! it can be set to have a maximum number of lines, which will cause the older lines to drop off when they accumulate too deeply
        //! it is primarily used for incoming data streams that are constantly filling, not static sources such as files.
        //! of particular interest here is how it implements the required rowCount, columnCount, data, and flags functions
        class RingBufferLogDataModel
            : public QAbstractTableModel
        {
            Q_OBJECT
        public:
            enum class DataRoles
            {
                Icon = 0,
                Time,
                Window,
                Message
            };

            RingBufferLogDataModel(QObject* pParent);
            virtual ~RingBufferLogDataModel();

            ////////////////////////////////////////////////////////////////////////////////////////////////
            // QAbstractTableModel
            virtual int rowCount(const QModelIndex&) const;
            virtual int columnCount(const QModelIndex&) const;
            virtual Qt::ItemFlags flags(const QModelIndex& index) const;
            virtual QVariant data(const QModelIndex&, int role) const;
            ////////////////////////////////////////////////////////////////////////////////////////////////

            void AppendLine(Logging::LogLine& source);
            void CommitAdd();
            void Clear();

            const Logging::LogLine& GetLineFromIndex(const QModelIndex& index);

        private:
            AZStd::ring_buffer<Logging::LogLine> m_lines;
            int m_startLineAdd;
            int m_numLinesAdded;
            int m_numLinesRemoved;
        };

        //! this is a list-based (uses a vector) data model
        //! its meant for static or ever-growing data that we do not want to discard older lines for.  For example
        //! if you are forensically examining a previously-captured file.
        class ListLogDataModel
            : public QAbstractTableModel
        {
            Q_OBJECT
        public:
            ListLogDataModel(QObject* pParent = nullptr);
            virtual ~ListLogDataModel();

            //! call this with each line you have and want to add to the log (at any time, but from the same thread as the object belongs to)
            void AppendLine(Logging::LogLine& source);

            //! After appending line(s) they will not appear until you call CommitAdd
            void CommitAdd();
            void Clear();

            ////////////////////////////////////////////////////////////////////////////////////////////////
            // QAbstractTableModel
            virtual int rowCount(const QModelIndex&) const;
            virtual int columnCount(const QModelIndex&) const;
            virtual Qt::ItemFlags flags(const QModelIndex& index) const;
            virtual QVariant data(const QModelIndex&, int role) const;
            ////////////////////////////////////////////////////////////////////////////////////////////////

        private:
            bool m_alreadyAddingLines = false;
            int m_linesAdded = 0;

            AZStd::vector<Logging::LogLine> m_lines;
        };

        //! FilteredLogDataModel filters data based on whats in the TabSettings.
        class FilteredLogDataModel
            : public QSortFilterProxyModel
        {
        public:
            explicit FilteredLogDataModel(QObject* parent = nullptr);

            void SetTabSettings(const TabSettings& source);

        protected:
            TabSettings m_tabSettings;
            bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        };

        // ------------------------- INTERNAL IMPLEMENTATION STUFF -----------------------
        //! This log panel layout class just exists to glue the last element to the top right corner, ie, the add button
        //! The rest of the children are just standard.  usually there is only one other.
        class LogPanelLayout
            : public QLayout
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(LogPanelLayout, AZ::SystemAllocator);
            LogPanelLayout(QWidget* pParent);
            virtual ~LogPanelLayout();
            virtual void addItem(QLayoutItem*);
            virtual QLayoutItem* itemAt(int index) const;
            virtual QLayoutItem* takeAt(int index);
            virtual int count() const;
            virtual void setGeometry(const QRect& r);
            virtual QSize sizeHint() const;
            virtual QSize minimumSize() const;
            virtual Qt::Orientations expandingDirections() const;

        private:
            // log panel layout can only have two children:
            AZStd::vector<QLayoutItem*> m_children;
        };


        // this is a private class, but its here because its a Q object and needs MOC to run.
        class LogPanelItemDelegate
            : public QStyledItemDelegate
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(LogPanelItemDelegate, AZ::SystemAllocator);
            LogPanelItemDelegate(QWidget* pParent, int messageColumn);
            virtual ~LogPanelItemDelegate();

            bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;
            void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

            QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

            void setEditorData(QWidget* editor, const QModelIndex& index) const override;
            void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

            QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
            void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

            QWidget* pOwnerWidget;
            QLabel* m_painterLabel;

Q_SIGNALS:
            void onLinkActivated(const QString& link);

        private:
            int m_messageColumn; // which column contains the message data ?
        };

        class SavedState
            : public AZ::UserSettings
        {
        public:
            AZ_RTTI(SavedState, "{38930360-DB02-445A-9CA0-3D1FB07B8236}", AZ::UserSettings);
            AZ_CLASS_ALLOCATOR(SavedState, AZ::SystemAllocator);
            AZStd::vector<LogPanel::TabSettings> m_tabSettings;

            SavedState() {}

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace LogPanel
} // namespace AzToolsFramework

#endif
