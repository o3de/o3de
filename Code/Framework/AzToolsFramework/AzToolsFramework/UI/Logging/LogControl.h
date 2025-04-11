/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LOGCONTROL_H
#define LOGCONTROL_H

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>

#include "LoggingCommon.h"

#include <QWidget>
#include <QString>
#endif

class QIcon;
class QAction;
class QTableView;
class QAbstractItemModel;

namespace AzToolsFramework
{
    // to use this control, put one of these controls in your ui.  You can inherit from this class
    // and override functionality like the GetItemColumn() functions, if you want to swap the order of columns around
    // or eliminate columns because your data does not have those columns.

    // once you have the view ready, you need a data model class.  The data model can be the same as the view in fact.  Its any class
    // which exposes QAbstractItemModel interface.  Most often its another QObject, though.

    // Your data model can be derived from any item model, (but I recommend QAbstractTableModel) and should implement at least the following overrides:
    // *    virtual int rowCount(const QModelIndex&) const;
    // *    virtual int columnCount(const QModelIndex &) const;
    // *    virtual QVariant data(const QModelIndex &,int role) const;
    // *    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    // then call ConnectModelToView() to attach the model to this view.

    // for an example see LogPanel_Panel.h and cpp - see AZTracePrintFLogTab - its an implementation of this control
    // which listens on the AZ::Debug::TraceMessageBus and forwards the messages to the log.

    // for an example of a data model, see LogPanelDataModel in LogPanel_Panel.h and cpp.

    namespace LogPanel
    {
        class BaseLogView
            : public QWidget
        {
            Q_OBJECT;
        public:
            AZ_CLASS_ALLOCATOR(BaseLogView, AZ::SystemAllocator);
            BaseLogView(QWidget* pParent);
            virtual ~BaseLogView();

            // you MUST call this, it sets up the model and connects various signals.
            void ConnectModelToView(QAbstractItemModel* ptrModel);

            // whether to expand the row height of the current item to show the full message text.
            void SetCurrentItemExpandsToFit(bool expandsToFit);

            // utility functions to retrieve standard icons without having to load them or prepare them over and over.
            static QIcon& GetInformationIcon();
            static QIcon& GetWarningIcon();
            static QIcon& GetErrorIcon();
            static QIcon& GetDebugIcon();

        protected:
            QTableView* m_ptrLogView;      // this is the actual table view.  You probably don't need to do anything to this, since its model view based.

            // return -1 for any of these to indicate that your data has no such column.
            // make sure your model has the same semantics!
            virtual int GetIconColumn() { return 0; }
            virtual int GetTimeColumn() { return 1; }
            virtual int GetWindowColumn() { return 2; }
            virtual int GetMessageColumn() { return 3; }      // you may not return -1 for this one.

            // override this if you want to provide an implementation that decorates the text in some way.
            // its only used in copy and paste, so this is what formats for the clipboard.
            // the default implementation simply concatenates all columns of text with " - " seperating them.
            virtual QString ConvertRowToText(const QModelIndex& row);

            // utility function.  You can call this to determine if you need to continue to keep scrolling to the bottom whenever
            // you alter the data.  call scrollToBottom to scroll to the bottom!
            bool IsAtMaxScroll() const;

            // Does some visual tidying up on show
            void showEvent(QShowEvent *event) override;

            // Context menus.  you can add whatever else you want to it.  You don't need to override this.
            // but you can if you want to delete the actions so that you cant select.
            // just create QActions, and add them to 'this' at any time.
            virtual void CreateContextMenu();
            // Backing code to the context menu
            QAction* actionSelectAll;
            QAction* actionSelectNone;
            QAction* actionCopySelected;
            QAction* actionCopyAll;

        public slots:
            // call this base if you override this, please.  It makes sure items are the right size.
            virtual void rowsInserted(const QModelIndex& parent, int start, int end);

            // Backing code to the context menu.  You can override these to do what ever
            // the default implementation will call ConvertRowToText().
            virtual void SelectAll();
            virtual void SelectNone();
            virtual void CopyAll();
            virtual void CopySelected();

            void CurrentItemChanged(const QModelIndex& current, const QModelIndex& previous);

            // connect to this signal if you want to know when someone clicked a link in a URL in a rich text box.
        signals:
            void onLinkActivated(const QString& link);

        private:
            bool m_currentItemExpandsToFit;

            // used to avoid reloading the icons over and over...
            static int s_panelRefCount;
            static QIcon* s_criticalIcon;
            static QIcon* s_warningIcon;
            static QIcon* s_informationIcon;
            static QIcon* s_debugIcon;
        };
    } // namespace LogPanel
} // namespace AzToolsFramework

#endif
