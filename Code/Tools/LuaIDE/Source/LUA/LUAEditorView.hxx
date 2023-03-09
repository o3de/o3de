/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LUAEDITORVIEW_H
#define LUAEDITORVIEW_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QtWidgets/QDockWidget>

#include "LUAEditorContextInterface.h"
#include "LUABreakpointTrackerMessages.h"
#endif

#pragma once

class QWidget;
class QFocusEvent;

namespace Ui
{
    class LUAEditorView;
}

namespace AzToolsFramework
{
    class ProgressShield;
}

namespace LUAEditor
{
    struct FindOperationImpl;
    
    // this is just a wrapper so we can override close events and so on
    class LUADockWidget 
        : public QDockWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(LUADockWidget,AZ::SystemAllocator,0);
        LUADockWidget(QWidget *parent, Qt::WindowFlags flags = Qt::WindowFlags());

        virtual void closeEvent(QCloseEvent *event);
        const AZStd::string& assetId() const { return m_assetId; }
        void setAssetId(const AZStd::string& assetId){ m_assetId = assetId; }
    private:
        AZStd::string m_assetId;
        
    private slots:
        void OnDockLocationChanged(Qt::DockWidgetArea newArea);
    };

    class LUAViewWidget : public QWidget, LUAEditor::LUABreakpointTrackerMessages::Bus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(LUAViewWidget, AZ::SystemAllocator);

        LUAViewWidget(QWidget *pParent = NULL);
        virtual ~LUAViewWidget();

        DocumentInfo m_Info; // last updated doc info - we will get updates to this.

        void Initialize(const DocumentInfo& initialInfo);
        void OnDocumentInfoUpdated(const DocumentInfo& newInfo);
        LUADockWidget* luaDockWidget(){ return m_pLUADockWidget;}
        void SetLuaDockWidget(LUADockWidget* pLUADockWidget){m_pLUADockWidget = pLUADockWidget;}

        void dropEvent(QDropEvent *e) override;

        // point a little arrow at this line.  -1 means remove it.
        void UpdateCurrentExecutingLine(int lineNumber);

        // auto-move the insert cursor to the beginning of lineNumber
        void UpdateCurrentEditingLine(int lineNumber);
        
        //////////////////////////////////////////////////////////////////////////
        //Debugger Messages, from the LUAEditor::LUABreakpointTrackerMessages::Bus
        void BreakpointsUpdate(const LUAEditor::BreakpointMap& uniqueBreakpoints) override;
        void BreakpointHit(const LUAEditor::Breakpoint& breakpoint) override;
        void BreakpointResume() override;
        
        bool IsReadOnly() const;
        bool IsModified() const;
        void SelectAll();
        bool HasSelectedText() const;
        QString GetSelectedText() const;
        void RemoveSelectedText();
        void ReplaceSelectedText(const QString& newText);
        void GetCursorPosition(int& line, int& column) const;
        void SetCursorPosition(int line, int column);
        //returns false if there is no selection
        bool GetSelection(int& lineStart, int& columnStart, int& lineEnd, int& columnEnd) const;
        void SetSelection(int lineStart, int columnStart, int lineEnd, int columnEnd);

        void MoveCursor(int relativePosition);

        QString GetLineText(int line) const;

        void FoldAll();
        void UnfoldAll();
        void SelectToMatchingBrace();
        void CommentSelectedLines();
        void UncommentSelectedLines();

        void MoveSelectedLinesUp();
        void MoveSelectedLinesDn();

        struct FindOperation
        {
            FindOperation();
            ~FindOperation();
            FindOperation(FindOperation&&);
            FindOperation& operator=(FindOperation&&);
            friend class LUAViewWidget;
            operator bool();

        private:
            FindOperation(FindOperationImpl* impl);
            FindOperation(const FindOperation&) = delete;
            FindOperation& operator=(const FindOperation&) = delete;

            FindOperationImpl* m_impl;
        };
        FindOperation FindFirst(const QString& searchString, bool isRegularExpression, bool isCaseSensitiveSearch, bool wholeWord, bool wrap, bool searchDown) const;
        void FindNext(FindOperation& previousOperation) const;

        void SetAutoCompletionEnabled(bool enabled) { m_AutoCompletionEnabled = enabled; }
        bool IsAutoCompletionEnabled() const { return m_AutoCompletionEnabled; }

        QString GetText();

        void Cut();
        void Copy();

        void ResetZoom();

        void UpdateFont();

    signals:
        void gainedFocus();
        void initialUpdate(LUAViewWidget* pThis);
        void sourceControlStatusUpdated(QString newValue);
        void RegainFocus();
        
    private:
        void UpdateModifyFlag();
        void keyPressEvent(QKeyEvent *ev) override;
        int CalcDocPosition(int line, int column);
        template<typename Callable> //callabe must take a const QTextCursor& as a parameter
        void UpdateCursor(Callable callable);
        template<typename Callable> //callabe sig (QString&, QTextBlock&)
        QString AcumulateSelectedLines(int& startLine, int& endLine, Callable callable);
        template<typename Callable> //callabe sig (QString&, QTextBlock&)
        void CommentHelper(Callable callable);
        void SetReadonly(bool readonly);
        //will call callable with 2 ints(start and end brace). if no matching braces currently, then callable is not called.
        //if currently on a bracket, but its not matched then callable will still get called, but with -1 for end brace
        template<typename Callable>
        void FindMatchingBrace(Callable callable);
        void focusInEvent(QFocusEvent* pEvent) override;

        void OnPlainTextFocusChanged(bool hasFocus);
        void CreateStyleSheet();

        Ui::LUAEditorView* m_gui;

        LUADockWidget* m_pLUADockWidget;
        AzToolsFramework::ProgressShield *m_pLoadingProgressShield;
        AzToolsFramework::ProgressShield *m_pSavingProgressShield;
        AzToolsFramework::ProgressShield *m_pRequestingEditProgressShield;

        struct BreakpointData
        {
            AZ::Uuid m_editorId; // globally unique
            int m_lastKnownLine; // where it was, for detecting shifts.
            BreakpointData(const AZ::Uuid& uuid = AZ::Uuid::CreateNull(), int lastKnownLine = -1) : m_editorId(uuid), m_lastKnownLine(lastKnownLine) {}
        };

        typedef AZStd::unordered_map<int, BreakpointData> BreakpointMap;
        BreakpointMap m_Breakpoints;

        void SyncToBreakpointLine( int line, AZ::Uuid existingID );

        bool m_PullRequestQueued;
        bool m_AutoCompletionEnabled;

        class LUASyntaxHighlighter* m_Highlighter;
        int m_zoomPercent{100}; //following visual studio, always zoom in or out 10% of current zoom value

        AZStd::mutex m_extraHighlightingMutex;

    private slots: 
        void OnBreakpointLineMoved(int fromLineNumber, int toLineNumber);
        void OnBreakpointLineDeleted(int removedLineNumber);

    public slots:
        void modificationChanged(bool m);
        void PullFreshBreakpoints();
        void RegainFocusFinal();
        void UpdateBraceHighlight();
        void OnVisibilityChanged(bool vc);
        void OnZoomIn();
        void OnZoomOut();
        void BreakpointToggle(int line);
    };

    class LUAViewMessages : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we have multiple listeners.
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<LUAViewMessages> Bus;
        typedef Bus::Handler Handler;    

        virtual ~LUAViewMessages() {}

        virtual void OnDataLoadedAndSet(const DocumentInfo& info, LUAViewWidget* pLUAViewWidget) = 0;
    };

};

#endif //LUAEDITORVIEW_H
