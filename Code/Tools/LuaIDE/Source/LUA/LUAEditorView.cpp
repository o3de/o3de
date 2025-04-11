/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LUAEditorView.hxx"

#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>

#include <Source/LUA/moc_LUAEditorView.cpp>
#include "LUAEditorContextMessages.h"
#include "LUAEditorContextInterface.h"
#include "LUAEditorViewMessages.h"
#include "LUAEditorMainWindow.hxx"
#include "LUABreakpointTrackerMessages.h"
#include "LUAEditorSyntaxHighlighter.hxx"
#include "LUAEditorStyleMessages.h"

#include <Source/LUA/ui_LUAEditorView.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <QFileInfo>
#include <QTimer>
#include <QMessageBox>
#include <QMimeData>

namespace LUAEditor
{
    struct FindOperationImpl
    {
        AZ_CLASS_ALLOCATOR(FindOperationImpl, AZ::SystemAllocator);

        QTextCursor m_cursor;
        QString m_searchString;
        bool m_isRegularExpression;
        bool m_isCaseSensitiveSearch;
        bool m_wholeWord;
        bool m_wrap;
        bool m_searchDown;
    };

    LUAViewWidget::FindOperation::FindOperation()
        : m_impl(nullptr)
    {
    }

    LUAViewWidget::FindOperation::FindOperation(FindOperation&& other)
    {
        m_impl = nullptr;
        *this = AZStd::move(other);
    }

    LUAViewWidget::FindOperation::FindOperation(FindOperationImpl* impl)
    {
        m_impl = impl;
    }

    LUAViewWidget::FindOperation::~FindOperation()
    {
        delete m_impl;
    }

    LUAViewWidget::FindOperation& LUAViewWidget::FindOperation::operator=(FindOperation&& other)
    {
        if (m_impl)
        {
            delete m_impl;
        }
        m_impl = other.m_impl;
        other.m_impl = nullptr;
        return *this;
    }

    LUAViewWidget::FindOperation::operator bool()
    {
        return m_impl && !m_impl->m_cursor.isNull();
    }

    //////////////////////////////////////////////////////////////////////////
    //LUADockWidget
    LUADockWidget::LUADockWidget(QWidget* parent, Qt::WindowFlags flags)
        : QDockWidget("LUADockWidget", parent, flags)
    {
        connect(this, &LUADockWidget::dockLocationChanged, this, &LUADockWidget::OnDockLocationChanged);
    }

    void LUADockWidget::closeEvent(QCloseEvent* event)
    {
        if (QMainWindow* pQMainWindow = qobject_cast<QMainWindow*>(parentWidget()))
        {
            if (LUAEditorMainWindow* pLUAEditorMainWindow = qobject_cast<LUAEditorMainWindow*>(pQMainWindow->parentWidget()))
            {
                pLUAEditorMainWindow->RequestCloseDocument(m_assetId);
                event->accept();
            }
        }
    }

    void LUADockWidget::OnDockLocationChanged(Qt::DockWidgetArea newArea)
    {
        (void)newArea;
        if (QMainWindow* pQMainWindow = qobject_cast<QMainWindow*>(parentWidget()))
        {
            if (LUAEditorMainWindow* pLUAEditorMainWindow = qobject_cast<LUAEditorMainWindow*>(pQMainWindow->parentWidget()))
            {
                pLUAEditorMainWindow->OnDockWidgetLocationChanged(m_assetId);
            }
        }
    }


    //////////////////////////////////////////////////////////////////////////
    //LUAViewWidget

    LUAViewWidget::LUAViewWidget(QWidget* pParent /*=NULL*/)
        : QWidget(pParent)
        , m_gui(azcreate(Ui::LUAEditorView, ()))
        , m_pLUADockWidget(NULL)
        , m_pLoadingProgressShield(NULL)
        , m_pSavingProgressShield(NULL)
        , m_pRequestingEditProgressShield(NULL)
        , m_PullRequestQueued(false)
        , m_AutoCompletionEnabled(true)
    {
        m_gui->setupUi(this);

        setAcceptDrops(true);

        m_gui->m_luaTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

        m_gui->m_breakpoints->SetTextEdit(m_gui->m_luaTextEdit);
        m_gui->m_folding->SetTextEdit(m_gui->m_luaTextEdit);

        m_Highlighter = aznew LUASyntaxHighlighter(m_gui->m_luaTextEdit->document());

        QTextDocument* doc = m_gui->m_luaTextEdit->document();
        QTextOption option = doc->defaultTextOption();
        option.setFlags(option.flags() | QTextOption::ShowTabsAndSpaces);
        doc->setDefaultTextOption(option);

        connect(m_gui->m_luaTextEdit, &LUAEditorPlainTextEdit::modificationChanged, this, &LUAViewWidget::modificationChanged);
        connect(m_gui->m_luaTextEdit, &LUAEditorPlainTextEdit::cursorPositionChanged, this, &LUAViewWidget::UpdateBraceHighlight);
        connect(m_gui->m_luaTextEdit, &LUAEditorPlainTextEdit::Scrolled, m_gui->m_folding, static_cast<void(FoldingWidget::*)()>(&FoldingWidget::update));
        connect(m_gui->m_luaTextEdit, &LUAEditorPlainTextEdit::blockCountChanged, m_gui->m_breakpoints, &LUAEditorBreakpointWidget::OnBlockCountChange);
        connect(m_gui->m_luaTextEdit->document(), &QTextDocument::contentsChange, m_gui->m_breakpoints, &LUAEditorBreakpointWidget::OnCharsRemoved);
        connect(m_gui->m_luaTextEdit, &LUAEditorPlainTextEdit::FocusChanged, this, &LUAViewWidget::OnPlainTextFocusChanged);
        connect(m_gui->m_luaTextEdit->document(), &QTextDocument::contentsChange, m_gui->m_folding, &FoldingWidget::OnContentChanged);
        connect(m_gui->m_luaTextEdit, &LUAEditorPlainTextEdit::ZoomIn, this, &LUAViewWidget::OnZoomIn);
        connect(m_gui->m_luaTextEdit, &LUAEditorPlainTextEdit::ZoomOut, this, &LUAViewWidget::OnZoomOut);
        connect(m_gui->m_luaTextEdit, &LUAEditorPlainTextEdit::cursorPositionChanged, m_gui->m_folding, static_cast<void(FoldingWidget::*)()>(&FoldingWidget::update));
        connect(m_gui->m_luaTextEdit, &LUAEditorPlainTextEdit::cursorPositionChanged, m_gui->m_breakpoints, static_cast<void(LUAEditorBreakpointWidget::*)()>(&LUAEditorBreakpointWidget::update));
        connect(m_gui->m_luaTextEdit, &LUAEditorPlainTextEdit::Scrolled, m_gui->m_breakpoints, static_cast<void(LUAEditorBreakpointWidget::*)()>(&LUAEditorBreakpointWidget::update));

        connect(this, &LUAViewWidget::RegainFocus, this, &LUAViewWidget::RegainFocusFinal, Qt::QueuedConnection);
    
        connect(m_gui->m_folding, &FoldingWidget::TextBlockFoldingChanged, m_gui->m_breakpoints, static_cast<void(LUAEditorBreakpointWidget::*)()>(&LUAEditorBreakpointWidget::update));
        connect(m_gui->m_folding, &FoldingWidget::TextBlockFoldingChanged, m_gui->m_luaTextEdit, static_cast<void(LUAEditorPlainTextEdit::*)()>(&LUAEditorPlainTextEdit::update));

        connect(m_Highlighter, &LUASyntaxHighlighter::LUANamesInScopeChanged, m_gui->m_luaTextEdit, &LUAEditorPlainTextEdit::OnScopeNamesUpdated);

        connect(m_gui->m_breakpoints, &LUAEditorBreakpointWidget::toggleBreakpoint, this, &LUAViewWidget::BreakpointToggle);
        connect(m_gui->m_breakpoints, &LUAEditorBreakpointWidget::breakpointLineMove, this, &LUAViewWidget::OnBreakpointLineMoved);
        connect(m_gui->m_breakpoints, &LUAEditorBreakpointWidget::breakpointDelete, this, &LUAViewWidget::OnBreakpointLineDeleted);

        CreateStyleSheet();

        this->setMinimumSize(300, 300);
        SetReadonly(true);

        m_gui->m_luaTextEdit->SetGetLuaName([&](const QTextCursor& cursor) {return m_Highlighter->GetLUAName(cursor); });

        UpdateFont();

        LUABreakpointTrackerMessages::Handler::BusConnect();
    }

    void LUAViewWidget::Initialize(const DocumentInfo& initialInfo)
    {
        m_Info = initialInfo;

        if (!m_pLoadingProgressShield)
        {
            m_pLoadingProgressShield = aznew AzToolsFramework::ProgressShield(this);
        }
        m_pLoadingProgressShield->setProgress(0, 0, AZStd::string::format("Loading '%s'...", initialInfo.m_displayName.c_str()).c_str());
        m_pLoadingProgressShield->show();
    }

    void LUAViewWidget::CreateStyleSheet()
    {
        auto colors = AZ::UserSettings::CreateFind<SyntaxStyleSettings>(AZ_CRC_CE("LUA Editor Text Settings"), AZ::UserSettings::CT_GLOBAL);

        auto styleSheet = QString(R"(QPlainTextEdit:focus
                                    {
                                        background-color: %1;
                                        selection-background-color:  %6;
                                        selection-color: %5;
                                    }

                                    QPlainTextEdit:!focus
                                    {
                                        background-color: %2;
                                        selection-color: %5;
                                        selection-background-color:  %6;
                                    }

                                    QPlainTextEdit[readOnly="true"]:focus
                                    {
                                        background-color: %3;
                                        selection-color: %5;
                                        selection-background-color:  %6;
                                    }

                                    QPlainTextEdit[readOnly="true"]:!focus
                                    {
                                        background-color: %4;
                                        selection-color: %5;
                                        selection-background-color:  %6;
                                    }
                                    )");

        styleSheet = styleSheet.arg(colors->GetTextFocusedBackgroundColor().name())
                               .arg(colors->GetTextUnfocusedBackgroundColor().name())
                               .arg(colors->GetTextReadOnlyFocusedBackgroundColor().name())
                               .arg(colors->GetTextReadOnlyUnfocusedBackgroundColor().name())
                               .arg(colors->GetTextSelectedColor().name())
                               .arg(colors->GetTextSelectedBackgroundColor().name());

        m_gui->m_luaTextEdit->setStyleSheet(styleSheet);

        styleSheet = QString(R"(LUAEditor--FoldingWidget:enabled { 
                                    background-color: %1;
                                }

                                LUAEditor--FoldingWidget:!enabled { 
                                    background-color: %2;
                                }
                                )");

        styleSheet = styleSheet.arg(colors->GetFoldingFocusedBackgroundColor().name())
                               .arg(colors->GetFoldingUnfocusedBackgroundColor().name());

        m_gui->m_folding->setStyleSheet(styleSheet);
    }

    LUAViewWidget::~LUAViewWidget()
    {
        m_gui->m_breakpoints->PreDestruction();
        delete m_Highlighter;
        azdestroy(m_gui);
        LUABreakpointTrackerMessages::Handler::BusDisconnect();
    }

    template<typename Callable>
    void LUAViewWidget::UpdateCursor(Callable callable)
    {
        auto cursor = m_gui->m_luaTextEdit->textCursor();
        callable(cursor);
        m_gui->m_luaTextEdit->setTextCursor(cursor);
    }

    void LUAViewWidget::OnDocumentInfoUpdated(const DocumentInfo& newInfo)
    {
        AZ_Assert(newInfo.m_assetId == m_Info.m_assetId, "Asset ID mismatch.");

        //this is the the initial unmodified state
        bool modifiedValue = newInfo.m_bIsModified;

        //data loading///////////////////////////////////////////////
        if ((newInfo.m_bDataIsLoaded) && (!m_Info.m_bDataIsLoaded))
        {
            // load the data now that its ready!
            if (!newInfo.m_bUntitledDocument)
            {
                const char* buffer = NULL;
                AZStd::size_t actualSize = 0;
                Context_DocumentManagement::Bus::Broadcast(
                    &Context_DocumentManagement::Bus::Events::GetDocumentData, newInfo.m_assetId, &buffer, actualSize);
                m_gui->m_luaTextEdit->setPlainText(buffer);

                LUAViewMessages::Bus::Broadcast(&LUAViewMessages::Bus::Events::OnDataLoadedAndSet, newInfo, this);
            }

            //remove the loading shield
            if (m_pLoadingProgressShield)
            {
                delete m_pLoadingProgressShield;
                m_pLoadingProgressShield = NULL;
                UpdateFont(); // Loading over, inner document need the latest font settings
            }

            // scan the breakpoint store from our context and pre-set the markers to get in sync
            const LUAEditor::BreakpointMap* myData = NULL;
            LUAEditor::LUABreakpointRequestMessages::Bus::BroadcastResult(
                myData, &LUAEditor::LUABreakpointRequestMessages::Bus::Events::RequestBreakpoints);
            AZ_Assert(myData, "LUAEditor::LUABreakpointRequestMessages::Bus, RequestBreakpoints failed to return any data.");
            BreakpointsUpdate(*myData);
            UpdateCurrentEditingLine(newInfo.m_PresetLineAtOpen);

            //AZ_TracePrintf("LUA", "Document Loaded Regain\n");
            emit RegainFocus();
        }

        if ((!newInfo.m_bDataIsLoaded) && (m_Info.m_bDataIsLoaded))
        {
            // wait for new data.
            if (!m_pLoadingProgressShield)
            {
                m_pLoadingProgressShield = aznew AzToolsFramework::ProgressShield(this);
            }
            m_pLoadingProgressShield->setProgress(0, 0, AZStd::string::format("Loading... ").c_str());
            m_pLoadingProgressShield->show();
        }

        //data saving/////////////////////////////
        if ((!newInfo.m_bIsBeingSaved) && (m_Info.m_bIsBeingSaved))
        {
            //remove the saving shield!
            if (m_pSavingProgressShield)
            {
                delete m_pSavingProgressShield;
                m_pSavingProgressShield = NULL;
            }
        }

        if ((newInfo.m_bDataIsWritten) && (!m_Info.m_bDataIsWritten))
        {
            m_gui->m_luaTextEdit->document()->setModified(false);
        }

        if ((newInfo.m_bIsBeingSaved) && (!m_Info.m_bIsBeingSaved))
        {
            // show the saving shield!
            if (!m_pSavingProgressShield)
            {
                m_pSavingProgressShield = aznew AzToolsFramework::ProgressShield(this);
            }
            m_pSavingProgressShield->setProgress(0, 0, AZStd::string::format("Saving... ").c_str());
            m_pSavingProgressShield->show();
        }

        //requesting edit (checking out)/////////////////////////////
        if ((!newInfo.m_bSourceControl_BusyRequestingEdit) && (m_Info.m_bSourceControl_BusyRequestingEdit))
        {
            //remove the checking out shield
            if (m_pRequestingEditProgressShield)
            {
                delete m_pRequestingEditProgressShield;
                m_pRequestingEditProgressShield = NULL;
            }
        }

        if ((newInfo.m_bSourceControl_BusyRequestingEdit) && (!m_Info.m_bSourceControl_BusyRequestingEdit))
        {
            // wait for edit request.
            if (!m_pRequestingEditProgressShield)
            {
                m_pRequestingEditProgressShield = aznew AzToolsFramework::ProgressShield(this);
            }
            m_pRequestingEditProgressShield->setProgress(0, 0, AZStd::string::format("Checking Out... ").c_str());
            m_pRequestingEditProgressShield->show();
        }

        //update view properties//////////////////////////////////////////////
        if ((newInfo.m_bSourceControl_CanWrite) && (newInfo.m_bDataIsLoaded))
        {
            if (m_gui->m_luaTextEdit->isReadOnly())
            {
                SetReadonly(false);
                update();
            }
        }
        else
        {
            // in all other cases we are waiting for more.
            if (!m_gui->m_luaTextEdit->isReadOnly())
            {
                SetReadonly(true);
                update();
            }
        }

        QString statusString(tr("P4:"));
        {
            // this flag goes true when a file's read-only status should be verified
            bool checkWriteIsWrong = false;

            if (newInfo.m_sourceControlInfo.m_status == AzToolsFramework::SCS_ProviderIsDown)
            {
                statusString += tr(" Unknown: P4 Down");
            }
            else if (newInfo.m_sourceControlInfo.m_status == AzToolsFramework::SCS_ProviderError)
            {
                statusString += tr(" Unknown: P4 Error");
            }
            else if (newInfo.m_sourceControlInfo.m_status == AzToolsFramework::SCS_CertificateInvalid)
            {
                statusString += tr(" Unknown: P4 SSL Certificate Invalid");
            }
            else if (newInfo.m_sourceControlInfo.m_flags & AzToolsFramework::SCF_OpenByUser)
            {
                if (newInfo.m_sourceControlInfo.m_flags & AzToolsFramework::SCF_PendingAdd)
                {
                    statusString += tr(" Adding");
                }
                else if (newInfo.m_sourceControlInfo.m_flags & AzToolsFramework::SCF_PendingDelete)
                {
                    statusString += tr(" Deleting");
                }
                else
                {
                    QString msg = tr(" Checked Out");
                    msg += QString(tr(" By You"));
                    // m_StatusUser only has contents if someone other than you has this file checked out, too
                    if (newInfo.m_sourceControlInfo.m_StatusUser.length())
                    {
                        msg += QString(tr(" and Others"));
                    }
                    statusString += msg;
                }
            }
            else if (newInfo.m_sourceControlInfo.m_flags & AzToolsFramework::SCF_OtherOpen)
            {
                QString msg = tr(" Checked Out");
                msg += QString(tr(" By ")) + newInfo.m_sourceControlInfo.m_StatusUser.c_str();
                statusString += msg;

                checkWriteIsWrong = true;
            }
            else if (!newInfo.m_sourceControlInfo.IsManaged())
            {
                statusString += tr(" Not Tracked");
            }
            else
            {
                statusString += tr(" Not Checked Out");

                checkWriteIsWrong = true;
            }

            if (checkWriteIsWrong)
            {
                QFileInfo fi(newInfo.m_sourceControlInfo.m_filePath.c_str());
                if (fi.exists() && fi.isWritable())
                {
                    statusString += tr(" But Writable?");
                }
            }

            if ((!newInfo.m_bSourceControl_BusyRequestingEdit) && (!m_Info.m_bSourceControl_BusyGettingStats))
            {
                //remove the checking out shield
                if (m_pRequestingEditProgressShield)
                {
                    delete m_pRequestingEditProgressShield;
                    m_pRequestingEditProgressShield = NULL;
                }
            }
        }

        emit sourceControlStatusUpdated(statusString);

        //save new state
        m_Info = newInfo;
        m_gui->m_luaTextEdit->document()->setModified(modifiedValue);
        UpdateModifyFlag();
    }

    void LUAViewWidget::OnPlainTextFocusChanged(bool hasFocus)
    {
        if (!m_gui->m_folding || !m_gui->m_breakpoints)
        {
            return;
        }

        if (hasFocus)
        {
            m_gui->m_breakpoints->setEnabled(true);
            m_gui->m_folding->setEnabled(true);
            LUAEditorMainWindowMessages::Bus::Broadcast(&LUAEditorMainWindowMessages::Bus::Events::OnFocusInEvent, m_Info.m_assetId);
        }
        else
        {
            m_gui->m_breakpoints->setEnabled(false);
            m_gui->m_folding->setEnabled(false);
            LUAEditorMainWindowMessages::Bus::Broadcast(&LUAEditorMainWindowMessages::Bus::Events::OnFocusOutEvent, m_Info.m_assetId);
        }
    }

    void LUAViewWidget::RegainFocusFinal()
    {
        //AZ_TracePrintf("LUA", "RegainFocusFinal\n");

        show();
        activateWindow();
        setFocus(Qt::MouseFocusReason);
    }

    void LUAViewWidget::OnVisibilityChanged(bool vc)
    {
        if (vc)
        {
            if (!m_pLUADockWidget->isFloating())
            {
                RegainFocusFinal();
        }
    }
    }

    void LUAViewWidget::OnBreakpointLineMoved(int fromLineNumber, int toLineNumber)
    {
        auto breakpoint = m_Breakpoints.find(fromLineNumber);
        if (breakpoint != m_Breakpoints.end())
        {
            Context_DebuggerManagement::Bus::Broadcast(
                &Context_DebuggerManagement::Bus::Events::MoveBreakpoint, breakpoint->second.m_editorId, toLineNumber);
        }
    }

    void LUAViewWidget::OnBreakpointLineDeleted(int removedLineNumber)
    {
        auto breakpoint = m_Breakpoints.find(removedLineNumber);
        if (breakpoint != m_Breakpoints.end())
        {
            Context_DebuggerManagement::Bus::Broadcast(
                &Context_DebuggerManagement::Bus::Events::DeleteBreakpoint, breakpoint->second.m_editorId);
        }
    }

    void LUAViewWidget::modificationChanged(bool m)
    {
        Context_DocumentManagement::Bus::Broadcast(&Context_DocumentManagement::Bus::Events::NotifyDocumentModified, m_Info.m_assetId, m);
        UpdateModifyFlag();
    }

    void LUAViewWidget::UpdateModifyFlag() {
        QString displayName = QString::fromUtf8(m_Info.m_displayName.c_str());
        if (m_gui->m_luaTextEdit->document()->isModified())
        {
            displayName += "*";
        }
        this->luaDockWidget()->setWindowTitle(displayName);
    }

    void LUAViewWidget::UpdateCurrentExecutingLine(int lineNumber)
    {
        m_gui->m_breakpoints->SetCurrentlyExecutingLine(lineNumber);
    }

    void LUAViewWidget::UpdateCurrentEditingLine(int lineNumber)
    {
        SetCursorPosition(lineNumber, 0);
    }

    void LUAViewWidget::SyncToBreakpointLine(int line, AZ::Uuid existingID)
    {
        auto existingBreakPont = AZStd::find_if(m_Breakpoints.begin(), m_Breakpoints.end(), [&](const AZStd::pair<int, BreakpointData>& _data)
                {
            return _data.second.m_editorId == existingID;
        });
        if (existingBreakPont != m_Breakpoints.end())
        {
            m_gui->m_breakpoints->RemoveBreakpoint(existingBreakPont->first);
            if (existingBreakPont->first != line)
            {
                m_Breakpoints.insert(AZStd::make_pair(line, BreakpointData(existingID, line)));
                m_gui->m_breakpoints->AddBreakpoint(line);
                m_Breakpoints.erase(existingBreakPont);
            }
        }
        else
        {
            m_Breakpoints.insert(AZStd::make_pair(line, BreakpointData(existingID, line)));
            m_gui->m_breakpoints->AddBreakpoint(line);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //Debugger Messages, from the LUAEditor::LUABreakpointTrackerMessages::Bus
    void LUAViewWidget::BreakpointsUpdate(const LUAEditor::BreakpointMap& uniqueBreakpoints)
    {
        (void)uniqueBreakpoints;

        if (!m_PullRequestQueued)
        {
            QTimer::singleShot(1, this, &LUAViewWidget::PullFreshBreakpoints);
            m_PullRequestQueued = true;
            return;
        }
    }

    void LUAViewWidget::PullFreshBreakpoints()
    {
        m_PullRequestQueued = false;

        m_Breakpoints.clear();
        m_gui->m_breakpoints->ClearBreakpoints();

        const LUAEditor::BreakpointMap* myData = NULL;
        LUAEditor::LUABreakpointRequestMessages::Bus::BroadcastResult(
            myData, &LUAEditor::LUABreakpointRequestMessages::Bus::Events::RequestBreakpoints);
        AZ_Assert(myData, "Nobody responded to the request breakpoints message.");

        // and slam down a new set
        for (LUAEditor::BreakpointMap::const_iterator it = myData->begin(); it != myData->end(); ++it)
        {
            const LUAEditor::Breakpoint& bp = it->second;
            if (m_Info.m_assetName == bp.m_assetName)
            {
                SyncToBreakpointLine(bp.m_documentLine, bp.m_breakpointId);
            }
        }
    }

    void LUAViewWidget::BreakpointHit(const LUAEditor::Breakpoint& bp)
    {
        if (bp.m_assetName == this->m_Info.m_assetName)
        {
            //AZ_TracePrintf("LUA", "Matching LUAViewWidget::BreakpointHit\n");

            SetCursorPosition(bp.m_documentLine, 0);
            emit RegainFocus();
        }
    }

    void LUAViewWidget::BreakpointResume()
    {
        m_gui->m_breakpoints->update();
    }

    void LUAViewWidget::BreakpointToggle(int line)
    {
        auto breakpoint = m_Breakpoints.find(line);
        if (breakpoint == m_Breakpoints.end())
        {
            Context_DebuggerManagement::Bus::Broadcast(&Context_DebuggerManagement::Bus::Events::CreateBreakpoint, m_Info.m_assetId, line);
        }
        else
        {
            Context_DebuggerManagement::Bus::Broadcast(
                &Context_DebuggerManagement::Bus::Events::DeleteBreakpoint, breakpoint->second.m_editorId);
        }
    }

    void LUAViewWidget::keyPressEvent(QKeyEvent* ev)
    {
        if (m_gui->m_luaTextEdit->isReadOnly())
        {
            bool isUseful = false;
            switch (ev->key())
            {
            case Qt::Key_Return:
            case Qt::Key_Enter:
            case Qt::Key_Backspace:
            case Qt::Key_Delete:
                isUseful = true;
                break;
            }
            bool isModified = false;
            if (ev->modifiers() & (Qt::ControlModifier | Qt::AltModifier))
            {
                // every Ctrl+ key combination is "modified" except for Ctrl+V, which should cause a check-out request
                if (((ev->key() != Qt::Key_V) && (ev->modifiers() & Qt::ControlModifier)))
                {
                    isModified = true;
                }
            }
            if ((!isModified) && (isascii(ev->key()) || isUseful))
            {
                QMessageBox msgBox;
                msgBox.setText("Checkout This File To Edit?");
                msgBox.setInformativeText(m_Info.m_assetName.c_str());
                msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                msgBox.setDefaultButton(QMessageBox::Ok);
                msgBox.setIcon(QMessageBox::Warning);
                int ret = msgBox.exec();
                if (ret == QMessageBox::Ok)
                {
                    LUAEditorMainWindowMessages::Bus::Broadcast(
                        &LUAEditorMainWindowMessages::Bus::Events::OnRequestCheckOut, m_Info.m_assetId);
                }
            }
        }
        ev->accept();
        QWidget::keyPressEvent(ev);
    }

    void LUAViewWidget::dropEvent(QDropEvent* e)
    {
        if (e->mimeData()->hasUrls())
        {
            LUADockWidget* ldw = luaDockWidget();
            if (ldw)
            {
                if (QMainWindow* pQMainWindow = qobject_cast<QMainWindow*>(ldw->parentWidget()))
                {
                    if (LUAEditorMainWindow* pLUAEditorMainWindow = qobject_cast<LUAEditorMainWindow*>(pQMainWindow->parentWidget()))
                    {
                        e->setDropAction(Qt::CopyAction);
                        pLUAEditorMainWindow->dropEvent(e);
                        e->accept();
                    }
                }
            }
        }
        else
        {
            QWidget::dropEvent(e);
        }
    }

    bool LUAViewWidget::IsReadOnly() const
    {
        return m_gui->m_luaTextEdit->isReadOnly();
    }

    bool LUAViewWidget::IsModified() const
    {
        return m_gui->m_luaTextEdit->document()->isModified();
    }

    void LUAViewWidget::SelectAll()
    {
        m_gui->m_luaTextEdit->selectAll();
    }

    bool LUAViewWidget::HasSelectedText() const
    {
        return m_gui->m_luaTextEdit->textCursor().hasSelection();
    }

    void LUAViewWidget::RemoveSelectedText()
    {
        auto cursor = m_gui->m_luaTextEdit->textCursor();
        if (!cursor.isNull() && cursor.hasSelection())
        {
            cursor.deleteChar();
    }
    }

    void LUAViewWidget::ReplaceSelectedText(const QString& newText)
    {
        auto cursor = m_gui->m_luaTextEdit->textCursor();
        if (!cursor.isNull())
        {
            cursor.insertText(newText);
    }
    }

    QString LUAViewWidget::GetSelectedText() const
    {
        return m_gui->m_luaTextEdit->textCursor().selectedText();
    }

    int LUAViewWidget::CalcDocPosition(int line, int column)
    {
        // Offset line number by one, because line number starts from 1, not 0.
        line = line - 1;
        if (line < 0)
        {
            line = column = 0;
        }
        int blockCount = m_gui->m_luaTextEdit->document()->blockCount();
        if (line > blockCount - 1)
        {
            line = blockCount - 1;
            column = INT_MAX;
        }

        auto block = m_gui->m_luaTextEdit->document()->findBlockByLineNumber(line);
        if (!block.isValid())
        {
            return m_gui->m_luaTextEdit->document()->characterCount() - 1;
        }

        column = AZStd::max(column, 0);
        column = AZStd::min(column, block.length() - 1);
        return block.position() + column;
    }

    void LUAViewWidget::GetCursorPosition(int& line, int& column) const
    {
        auto cursor = m_gui->m_luaTextEdit->textCursor();
        if (cursor.isNull())
        {
            line = 0;
            column = 0;
        }
        else
        {
            line = cursor.blockNumber() + 1; // offset by one because line number start from 1
            column = cursor.positionInBlock();
        }
    }

    void LUAViewWidget::SetCursorPosition(int line, int column)
    {
        UpdateCursor([&](QTextCursor& cursor)
            {
            cursor.setPosition(CalcDocPosition(line, column));
        });
    }

    void LUAViewWidget::MoveCursor(int relativePosition)
    {
        UpdateCursor([&](QTextCursor& cursor)
            {
            cursor.movePosition(relativePosition > 0 ? QTextCursor::MoveOperation::Right : QTextCursor::MoveOperation::Left,
                QTextCursor::MoveMode::MoveAnchor, abs(relativePosition));
        });
    }

    LUAViewWidget::FindOperation LUAViewWidget::FindFirst(const QString& searchString, bool isRegularExpression, bool isCaseSensitiveSearch, bool wholeWord, bool wrap, bool searchDown) const
    {
        FindOperation start(aznew FindOperationImpl);
        start.m_impl->m_cursor = m_gui->m_luaTextEdit->textCursor();
        start.m_impl->m_searchString = searchString;
        start.m_impl->m_isRegularExpression = isRegularExpression;
        start.m_impl->m_isCaseSensitiveSearch = isCaseSensitiveSearch;
        start.m_impl->m_wholeWord = wholeWord;
        start.m_impl->m_wrap = wrap;
        start.m_impl->m_searchDown = searchDown;

        FindNext(start);
        return start;
    }

    void LUAViewWidget::FindNext(FindOperation& operation) const
    {
        if (!operation)
        {
            return;
        }

        int flags = 0;
        if (operation.m_impl->m_wholeWord)
        {
            flags |= QTextDocument::FindFlag::FindWholeWords;
        }
        if (operation.m_impl->m_isCaseSensitiveSearch)
        {
            flags |= QTextDocument::FindFlag::FindCaseSensitively;
        }
        if (!operation.m_impl->m_searchDown)
        {
            flags |= QTextDocument::FindFlag::FindBackward;
        }

        if (operation.m_impl->m_isRegularExpression)
        {
            QRegExp regEx;
            regEx.setCaseSensitivity(operation.m_impl->m_isCaseSensitiveSearch ? Qt::CaseSensitivity::CaseSensitive : Qt::CaseSensitivity::CaseInsensitive);
            regEx.setPattern(operation.m_impl->m_searchString);
            operation.m_impl->m_cursor = m_gui->m_luaTextEdit->document()->find(regEx, operation.m_impl->m_cursor, static_cast<QTextDocument::FindFlag>(flags));
            if (!operation && operation.m_impl->m_wrap)
            {
                if (operation.m_impl->m_searchDown)
                {
                    operation.m_impl->m_cursor.setPosition(0);
                }
                else
                {
                    operation.m_impl->m_cursor.setPosition(m_gui->m_luaTextEdit->document()->characterCount() - 1);
                }
                operation.m_impl->m_cursor = m_gui->m_luaTextEdit->document()->find(regEx, operation.m_impl->m_cursor, static_cast<QTextDocument::FindFlag>(flags));
            }
        }
        else
        {
            operation.m_impl->m_cursor = m_gui->m_luaTextEdit->document()->find(operation.m_impl->m_searchString, operation.m_impl->m_cursor, static_cast<QTextDocument::FindFlag>(flags));
            if (!operation && operation.m_impl->m_wrap)
            {
                if (operation.m_impl->m_searchDown)
                {
                    operation.m_impl->m_cursor.setPosition(0);
                }
                else
                {
                    operation.m_impl->m_cursor.setPosition(m_gui->m_luaTextEdit->document()->characterCount() - 1);
                }
                operation.m_impl->m_cursor = m_gui->m_luaTextEdit->document()->find(operation.m_impl->m_searchString, operation.m_impl->m_cursor, static_cast<QTextDocument::FindFlag>(flags));
            }
        }
        if (operation)
        {
            m_gui->m_luaTextEdit->setTextCursor(operation.m_impl->m_cursor);
    }
    }

    QString LUAViewWidget::GetLineText(int line) const
    {
        auto block = m_gui->m_luaTextEdit->document()->findBlockByLineNumber(line);
        return block.text();
    }

    bool LUAViewWidget::GetSelection(int& lineStart, int& columnStart, int& lineEnd, int& columnEnd) const
    {
        auto doc = m_gui->m_luaTextEdit->document();
        auto cursor = m_gui->m_luaTextEdit->textCursor();
        if (cursor.isNull())
        {
            lineStart = columnStart = lineEnd = columnEnd = -1;
            return false;
        }
        auto startPos = cursor.selectionStart();
        auto endPos = cursor.selectionEnd();

        auto startBlock = doc->findBlock(startPos);
        lineStart = startBlock.blockNumber();
        columnStart = startPos - startBlock.position();

        auto endBlock = doc->findBlock(endPos);
        lineEnd = endBlock.blockNumber();
        columnEnd = endPos - endBlock.position();

        return true;
    }

    void LUAViewWidget::SetSelection(int lineStart, int columnStart, int lineEnd, int columnEnd)
    {
        auto startPos = CalcDocPosition(lineStart, columnStart);
        auto endPos = CalcDocPosition(lineEnd, columnEnd);

        UpdateCursor([&](QTextCursor& cursor)
            {
            // Go back to front to keep cursor position consistent
            cursor.setPosition(endPos);
            cursor.setPosition(startPos, QTextCursor::MoveMode::KeepAnchor);
        });
    }

    QString LUAViewWidget::GetText()
    {
        return m_gui->m_luaTextEdit->toPlainText();
    }

    void LUAViewWidget::Cut()
    {
        m_gui->m_luaTextEdit->cut();
    }

    void LUAViewWidget::Copy()
    {
        m_gui->m_luaTextEdit->copy();
    }

    template<typename Callable>
    //callabe sig (QString&, QTextBlock&)
    QString LUAViewWidget::AcumulateSelectedLines(int& startLine, int& endLine, Callable callable)
    {
        int startColumn;
        int endColumn;
        GetSelection(startLine, startColumn, endLine, endColumn);
        AZ_Assert(startLine <= endLine, "assume selection is always forward");

        auto cursor = m_gui->m_luaTextEdit->textCursor();
        QString newText;
        for (int i = startLine; i <= endLine; ++i)
        {
            auto block = m_gui->m_luaTextEdit->document()->findBlockByNumber(i);
            if (block.isValid())
            {
                callable(newText, block);
            }
        }
        return newText;
    }

    template<typename Callable>
    void LUAViewWidget::CommentHelper(Callable callable)
    {
        int startLine;
        int endLine;
        auto newText = AcumulateSelectedLines(startLine, endLine, callable);

        SetSelection(startLine + 1, 0, endLine + 2, 0);
        RemoveSelectedText();
        SetCursorPosition(startLine + 1, 0);
        ReplaceSelectedText(newText);
        SetSelection(startLine + 1, 0, endLine + 1, INT_MAX);
    }

    void LUAViewWidget::CommentSelectedLines()
    {
        CommentHelper([&](QString& newText, QTextBlock& block)
            {
            newText.append("-- ");
            newText.append(block.text());
            newText.append("\n");
        });
    }

    void LUAViewWidget::UncommentSelectedLines()
    {
        CommentHelper([&](QString& newText, QTextBlock& block)
            {
            auto blockText = block.text();
            if (blockText.startsWith("--"))
            {
                int removeCount = 2;
                if (blockText.at(2).isSpace())
                    {
                    ++removeCount;
                    }
                blockText.remove(0, removeCount);
            }
            newText.append(blockText);
            newText.append("\n");
        });
    }

    void LUAViewWidget::MoveSelectedLinesUp()
    {
        int startLine;
        int endLine;
        auto currText = AcumulateSelectedLines(startLine, endLine, [&](QString& newText, QTextBlock& block)
                {
            newText.append(block.text());
            newText.append("\n");
        });
        currText.remove(currText.count() - 1, 1);

        if (startLine == 0)
        {
            return;
        }
        auto upText = GetLineText(startLine -1);
        SetSelection(startLine, 0, startLine, INT_MAX);
        RemoveSelectedText();
        SetSelection(startLine + 1, 0, endLine + 1, INT_MAX);
        RemoveSelectedText();

        SetCursorPosition(startLine , 0);
        ReplaceSelectedText(currText);

        SetCursorPosition(endLine + 1, 0);
        ReplaceSelectedText(upText);

        SetSelection(startLine, 0, endLine, INT_MAX);
    }

    void LUAViewWidget::MoveSelectedLinesDn()
    {
        int startLine;
        int endLine;
        auto newText = AcumulateSelectedLines(startLine, endLine, [&](QString& newText, QTextBlock& block)
                {
            newText.append(block.text());
            newText.append("\n");
        });
        if (endLine == m_gui->m_luaTextEdit->document()->blockCount() - 1)
        {
            return;
        }

        //hack if we are going to be the new last line
        if (endLine == m_gui->m_luaTextEdit->document()->blockCount() - 2)
        {
            newText.remove(newText.length() - 1, 1);
            newText.prepend("\n");
        }

        SetSelection(startLine + 1, 0, endLine + 2, 0);
        RemoveSelectedText();
        SetCursorPosition(startLine + 2, 0);
        ReplaceSelectedText(newText);
        SetSelection(startLine + 2, 0, endLine + 2, INT_MAX);
    }

    void LUAViewWidget::SetReadonly(bool readonly)
    {
        m_gui->m_luaTextEdit->setReadOnly(readonly);
        m_gui->m_luaTextEdit->style()->unpolish(m_gui->m_luaTextEdit);
        m_gui->m_luaTextEdit->style()->polish(m_gui->m_luaTextEdit);

        if (readonly)
        {
            // For readonly documents we set the TextSelectableByKeyboard to display a solid cursor.
            m_gui->m_luaTextEdit->setTextInteractionFlags(m_gui->m_luaTextEdit->textInteractionFlags() | Qt::TextSelectableByKeyboard);
        }
    }

    template<typename Callable>
    void LUAViewWidget::FindMatchingBrace(Callable callable)
    {
        auto doc = m_gui->m_luaTextEdit->document();

        auto cursor = m_gui->m_luaTextEdit->textCursor();
        int bracePos = cursor.position();

        QChar braceChar {
            QChar::Null
        };
        bool openingBrace {
            true
        };
        auto detect = [&]()
            {
            auto testChar = doc->characterAt(bracePos);
            if (testChar == '{' || testChar == '[' || testChar == '(')
            {
                braceChar = testChar;
                openingBrace = true;
            }
            if (testChar == '}' || testChar == ']' || testChar == ')')
            {
                braceChar = testChar;
                openingBrace = false;
            }
        };
        detect();
        if (braceChar == QChar::Null && bracePos > 0) //try previous char too so we can detect on either side of brace
        {
            --bracePos;
            detect();
        }
        if (braceChar != QChar::Null) //found one
        {
            int startPos = bracePos;
            QChar endChar {
                QChar::Null
            };
            if (braceChar == '{')
            {
                endChar = '}';
            }
            if (braceChar == '}')
            {
                endChar = '{';
            }
            if (braceChar == '[')
            {
                endChar = ']';
            }
            if (braceChar == ']')
            {
                endChar = '[';
            }
            if (braceChar == '(')
            {
                endChar = ')';
            }
            if (braceChar == ')')
            {
                endChar = '(';
            }

            int step {
                openingBrace ? 1 : -1
            };
            int level {
                0
            };
            bracePos += step;

            auto testChar = doc->characterAt(bracePos);
            while (testChar != QChar::Null)
            {
                if (testChar == braceChar)
                {
                    ++level;
                }
                else if (testChar == endChar)
                {
                    --level;
                    if (level < 0)
                    {
                        if (step > 0)
                        {
                            ++bracePos;
                        }
                        else
                        {
                            ++startPos;
                        }
                        callable(startPos, bracePos);
                        return;
                    }
                }
                bracePos += step;
                testChar = doc->characterAt(bracePos);
            }
            callable(startPos, -1); //had an opening brace, but no matching close brace
        }
    }

    void LUAViewWidget::SelectToMatchingBrace()
    {
        auto cursor = m_gui->m_luaTextEdit->textCursor();
        FindMatchingBrace([&](int startPos, int endPos)
            {
            if (endPos >= 0)
            {
                cursor.setPosition(startPos);
                cursor.setPosition(endPos, QTextCursor::MoveMode::KeepAnchor);
                m_gui->m_luaTextEdit->setTextCursor(cursor);
            }
        });
    }

    void LUAViewWidget::UpdateBraceHighlight()
    {
        m_Highlighter->SetBracketHighlighting(-1, -1);
        FindMatchingBrace([&](int startPos, int endPos)
            {
                if (endPos > 0 && endPos < startPos)
                {
                    AZStd::swap(startPos, endPos);
                }
                m_Highlighter->SetBracketHighlighting(startPos, endPos - 1);
        });

        auto cursor = m_gui->m_luaTextEdit->textCursor();
        auto text = m_gui->m_luaTextEdit->document()->toPlainText();

        m_gui->m_luaTextEdit->setExtraSelections(m_Highlighter->HighlightMatchingNames(cursor, text));

        m_gui->m_luaTextEdit->update();
    }

    void LUAViewWidget::focusInEvent(QFocusEvent* pEvent)
    {
        QWidget::focusInEvent(pEvent);
        m_gui->m_luaTextEdit->setFocus();
    }

    void LUAViewWidget::FoldAll()
    {
        m_gui->m_folding->FoldAll();
    }

    void LUAViewWidget::UnfoldAll()
    {
        m_gui->m_folding->UnfoldAll();
    }

    void LUAViewWidget::UpdateFont()
    {
        auto syntaxSettings = AZ::UserSettings::CreateFind<SyntaxStyleSettings>(AZ_CRC_CE("LUA Editor Text Settings"), AZ::UserSettings::CT_GLOBAL);
        syntaxSettings->SetZoomPercent(m_zoomPercent);
        const auto& font = syntaxSettings->GetFont();

        m_gui->m_luaTextEdit->SetTabSize(syntaxSettings->GetTabSize());
        m_gui->m_luaTextEdit->SetUseSpaces(syntaxSettings->UseSpacesInsteadOfTabs());

        m_gui->m_luaTextEdit->UpdateFont(font, syntaxSettings->GetTabSize());
        m_gui->m_breakpoints->SetFont(font);
        m_gui->m_folding->SetFont(font);

        m_gui->m_luaTextEdit->update();
        m_gui->m_breakpoints->update();
        m_gui->m_folding->update();
        m_Highlighter->rehighlight();

        CreateStyleSheet();
        m_gui->m_luaTextEdit->repaint();
    }

    void LUAViewWidget::OnZoomIn()
    {
        m_zoomPercent = AZStd::min(m_zoomPercent + m_zoomPercent / 5, 500);
        UpdateFont();
    }

    void LUAViewWidget::OnZoomOut()
    {
        m_zoomPercent = AZStd::max(m_zoomPercent - m_zoomPercent / 5, 50);
        UpdateFont();
    }

    void LUAViewWidget::ResetZoom()
    {
        m_zoomPercent = 100;
        UpdateFont();
    }
}
