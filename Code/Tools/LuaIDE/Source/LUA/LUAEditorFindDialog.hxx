/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LUAEDITOR_FINDDIALOG_H
#define LUAEDITOR_FINDDIALOG_H

#if !defined(Q_MOC_RUN)
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/Component/Component.h>
#include "LUAEditorView.hxx"

#include <QDialog>
#endif

#pragma once

class LUAEditorMainWindow;
class QListWidget;

namespace Ui
{
    class LUAEditorFindDialog;
}

namespace LUAEditor
{
    class LUAEditorMainWindow;
    class FindResults;

    // mode controls the main find process loops, that are now consolidated
    enum mode
    {
        CurrentDoc,
        AllOpenDocs,
        AllLUAAssets,
    };

    //////////////////////////////////////////////////////////////////////////
    // Find Dialog
    class LUAEditorFindDialog 
        : public QDialog
        , public LUAViewMessages::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(LUAEditorFindDialog, AZ::SystemAllocator);
        LUAEditorFindDialog(QWidget *parent = 0);
        ~LUAEditorFindDialog();

        void SetAnyDocumentsOpen(bool value);
        void SetToFindInAllOpen( bool findInAny );
        void SetNewSearchStarting( bool bOverride = false, bool bSearchForwards = true );
        void ResetSearch();

        virtual void showEvent( QShowEvent * event );

        static void Reflect(AZ::ReflectContext* reflection);
        void SaveState();

    private:
        LUAViewWidget::FindOperation m_findOperation;
        bool m_bFoundFirst;
        bool m_bLastForward;
        bool m_bLastWrap;
        bool m_bAnyDocumentsOpen;
        bool m_bWasFindInAll;

        // prep for deeper search options
        int m_WrapLine;
        int m_WrapIndex;
        LUAViewWidget* m_WrapWidget;
        LUAEditorMainWindow* pLUAEditorMainWindow;

        int m_lastSearchWhere;
        QString m_lastSearchText;
        void FindInView(LUAViewWidget* pLUAViewWidget, QListWidget* pCurrentFindListView);
        int ReplaceInView(LUAViewWidget* pLUAViewWidget);
        
        void BusyOn();
        void PostProcessOn();
        void PostReplaceOn();
        void BusyOff();
        
        //////////////////////////////////////////////////////////////////////////
        //LUAViewMessages::Bus
        void OnDataLoadedAndSet(const DocumentInfo& info, LUAViewWidget* pLUAViewWidget);
        //////////////////////////////////////////////////////////////////////////

        //stored dialog state for find/replace threads
        QString m_SearchText;
        bool m_bCaseSensitiveIsChecked;    
        bool m_bWholeWordIsChecked;
        bool m_bRegExIsChecked;
        AZStd::vector<AZStd::string> m_dOpenView;
        
        //find in files thread stuff
        void FindInFilesSetUp(int theMode, FindResults* resultsWidget);
        AZStd::vector<AZStd::string> m_dFindAllLUAAssetsInfo; 
        bool m_bFindThreadRunning;
        bool m_bCancelFindSignal;
        void ProcessFindItems();

        struct ResultEntry
        {
            QString m_lineText;
            int m_lineNumber;
            AZStd::vector<AZStd::pair<int, int>> m_matches; //position and length within line
        };

        struct ResultDocument
        {
            AZStd::string m_assetId;
            AZStd::vector<ResultEntry> m_entries;
        };
    
        QHash<QString, ResultDocument> m_resultList;

        //replace in files thread stuff
        bool m_bReplaceThreadRunning;
        bool m_bCancelReplaceSignal;
        void ReplaceInFilesSetUp();
        
        //utility
        LUAViewWidget* GetViewFromParent();

        struct FIFData
        {
            int m_TotalMatchesFound;
            QString m_SearchText;
            bool m_bCaseSensitiveIsChecked;    
            bool m_bWholeWordIsChecked;
            bool m_bRegExIsChecked;
            FindResults* m_resultsWidget;
            AZStd::vector<LUAViewWidget*> m_OpenView;
            AZStd::vector<AZStd::string> m_dOpenView;
            AZStd::vector<LUAViewWidget*>::iterator m_openViewIter;
            AZStd::vector< AZStd::string>::iterator m_assetInfoIter;
        };

        FIFData m_FIFData;

        struct RIFData
        {
            int m_TotalMatchesFound;
            QString m_SearchText;
            bool m_bCaseSensitiveIsChecked;    
            bool m_bWholeWordIsChecked;
            bool m_bRegExIsChecked;
            QListWidget* m_pCurrentFindListView;
            AZStd::vector<LUAViewWidget*> m_OpenView;
            AZStd::vector<AZStd::string> m_dOpenView;
            AZStd::vector</*EditorFramework::EditorAsset*/AZStd::string>::iterator m_assetInfoIter; // fix this 
            AZStd::vector</*EditorFramework::EditorAsset*/AZStd::string> m_dReplaceAllLUAAssetsInfo; // fix this 
            AZStd::vector<AZStd::string> m_dReplaceProcessList;
            AZStd::unordered_set<AZStd::string> m_waitingForOpenToComplete;
        };

        RIFData m_RIFData;

        AZStd::vector<LUAViewWidget*> m_PendingReplaceInViewOperations;
        
    signals:
        void triggerFindInFilesNext( int theMode );
        void triggerReplaceInFilesNext();
        void triggerFindNextInView(LUAViewWidget::FindOperation* operation, LUAViewWidget* pLUAViewWidget, QListWidget* pCurrentFindListView);

    public slots:
        void OnReplaceAll();
        void OnReplace();
        void OnCancel();
        void OnFindNext();
        void OnFindAll();
        void OnSearchWhereChanged( int index );
        void FindInFilesNext( int theMode );
        void ReplaceInFilesNext();
        void ProcessReplaceItems();
        void OnReplaceInViewIterate();
        void FindNextInView(LUAViewWidget::FindOperation* operation, LUAViewWidget* pLUAViewWidget, QListWidget* pCurrentFindListView);

    private:
        Ui::LUAEditorFindDialog* m_gui;
    };

}

#endif //LUAEDITOR_FINDDIALOG_H
