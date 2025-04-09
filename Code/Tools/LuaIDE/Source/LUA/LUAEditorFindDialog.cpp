/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/TickBus.h>
#include "LUAEditorFindDialog.hxx"
#include "LUAEditorMainWindow.hxx"
#include "LUAEditorContextMessages.h"
#include "LUAEditorBlockState.h"

#include <Source/LUA/ui_LUAEditorFindDialog.h>

#include <QMessageBox>
#include <QListWidgetItem>
#include <QTextDocument>
#include <QTimer>

namespace LUAEditorInternal
{
    // this stuff goes in  the user preferences rather than the global stuff:
    class FindSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(FindSavedState, "{2B880623-63A9-4B39-B8B9-47609590D7D2}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(FindSavedState, AZ::SystemAllocator);
        FindSavedState()
        {
            m_lastSearchInFilesMode = 0;
            m_findWrap = true;
        }

        int m_lastSearchInFilesMode;
        bool m_findWrap;

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<FindSavedState>()
                    ->Field("m_lastSearchInFilesMode", &FindSavedState::m_lastSearchInFilesMode)
                    ->Field("m_findWrap", &FindSavedState::m_findWrap);
            }
        }
    };
}

namespace LUAEditor
{
    extern AZ::Uuid ContextID;

    //////////////////////////////////////////////////////////////////////////
    //find dialog
    // QDialog(/*parent*/) strip the parent from QDialog and this window will no longer be forced on top all the time
    LUAEditorFindDialog::LUAEditorFindDialog(QWidget* parent)
        : QDialog(parent)
    {
        pLUAEditorMainWindow = qobject_cast<LUAEditorMainWindow*>(parent);
        AZ_Assert(pLUAEditorMainWindow, "Parent Widget is NULL or not the LUAEditorMainWindow!");

        m_bAnyDocumentsOpen = false;
        LUAViewMessages::Bus::Handler::BusConnect();

        m_bWasFindInAll = false;
        m_gui = azcreate(Ui::LUAEditorFindDialog, ());
        m_gui->setupUi(this);
        this->setFixedSize(this->size());
        m_gui->searchDownRadioButton->setChecked(true);
        m_gui->searchAndReplaceGroupBox->setChecked(false);
        m_gui->regularExpressionCheckBox->setChecked(false);

        auto pState = AZ::UserSettings::CreateFind<LUAEditorInternal::FindSavedState>(AZ_CRC_CE("FindInCurrent"), AZ::UserSettings::CT_LOCAL);
        m_gui->wrapCheckBox->setChecked((pState ? pState->m_findWrap : true));

        connect(m_gui->wrapCheckBox, &QCheckBox::stateChanged, this, [](int newState)
        {
            auto pState = AZ::UserSettings::CreateFind<LUAEditorInternal::FindSavedState>(AZ_CRC_CE("FindInCurrent"), AZ::UserSettings::CT_LOCAL);
            pState->m_findWrap = (newState == Qt::Checked);
        });

        m_bFoundFirst = false;
        m_bLastForward = m_gui->searchDownRadioButton->isChecked();
        m_bLastWrap = m_gui->wrapCheckBox->isChecked();

        //stored dialog state for find/replace
        m_bCaseSensitiveIsChecked = false;
        m_bWholeWordIsChecked = false;
        m_bRegExIsChecked = false;

        //find in files stuff
        m_bCancelFindSignal = false;
        m_bFindThreadRunning = false;

        //replace in files stuff
        m_bCancelReplaceSignal = false;
        m_bReplaceThreadRunning = false;

        connect(m_gui->searchWhereComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &LUAEditorFindDialog::OnSearchWhereChanged);

        connect(this, &LUAEditorFindDialog::triggerFindInFilesNext, this, &LUAEditorFindDialog::FindInFilesNext, Qt::QueuedConnection);
        connect(this, &LUAEditorFindDialog::triggerReplaceInFilesNext, this, &LUAEditorFindDialog::ReplaceInFilesNext, Qt::QueuedConnection);
        connect(this, &LUAEditorFindDialog::triggerFindNextInView, this, &LUAEditorFindDialog::FindNextInView, Qt::QueuedConnection);

        QString stylesheet(R"(QLabel[LUAEditorFindDialogLabel="true"],QGroupBox,QCheckBox,QRadioButton,QPushButton
                         {
                            font-size: 12px;
                         };

                         QLabel[IdleLabel="true"]
                         {
                            font-size:18px;
                         }
                        )");

        setStyleSheet(stylesheet);
    }

    LUAViewWidget* LUAEditorFindDialog::GetViewFromParent()
    {
        LUAViewWidget* pLUAViewWidget = pLUAEditorMainWindow->GetCurrentView();
        return pLUAViewWidget;
    }

    LUAEditorFindDialog::~LUAEditorFindDialog()
    {
        LUAViewMessages::Bus::Handler::BusDisconnect();
        azdestroy(m_gui);
    }

    void LUAEditorFindDialog::SetAnyDocumentsOpen(bool value)
    {
        m_bAnyDocumentsOpen = value;
        m_gui->searchWhereComboBox->clear();
        if (m_bAnyDocumentsOpen)
        {
            m_gui->searchWhereComboBox->addItem(tr("Current File"));
            m_gui->searchWhereComboBox->addItem(tr("All Open Files"));
            m_gui->searchWhereComboBox->setCurrentIndex(0);

            // TODO: Enable when asset database is in used
            //m_gui->searchWhereComboBox->addItem(tr("All LUA Assets"));

            // since at least one document is open take this opportunity
            // to copy any selected text block to the string to find in the dialog
            LUAViewWidget* pLUAViewWidget = GetViewFromParent();
            if (pLUAViewWidget)
            {
                if (pLUAViewWidget->HasSelectedText())
                {
                    QString qstr =  pLUAViewWidget->GetSelectedText();
                    m_gui->txtFind->setText(qstr);
                }
            }
        }
        else
        {
            // TODO: Enable when asset database is in used
            //m_gui->searchWhereComboBox->addItem(tr("All LUA Assets"));
            m_gui->searchWhereComboBox->setCurrentIndex(0);
        }



        m_gui->txtFind->setFocus();
        m_gui->txtFind->selectAll();
    }

    // make the "find next" button match the file scope from the pull-down menu
    void LUAEditorFindDialog::OnSearchWhereChanged(int index)
    {
        if (m_bAnyDocumentsOpen)
        {
            switch (index)
            {
            case 0:
                m_gui->findNextButton->setEnabled(true);
                break;
            case 1:
            case 2:
                m_gui->findNextButton->setEnabled(false);
                break;
            }
        }

        m_gui->findNextButton->setDefault(m_gui->findNextButton->isEnabled());
        m_gui->findAllButton->setDefault(!m_gui->findNextButton->isEnabled());
        m_gui->findNextButton->setAutoDefault(m_gui->findNextButton->isEnabled());
        m_gui->findAllButton->setAutoDefault(!m_gui->findNextButton->isEnabled());
    }

    void LUAEditorFindDialog::SaveState()
    {
        if (m_bAnyDocumentsOpen)
        {
            auto pState = AZ::UserSettings::CreateFind<LUAEditorInternal::FindSavedState>(m_bWasFindInAll ? AZ_CRC_CE("LUAFindInAny") : AZ_CRC_CE("FindInCurrent"), AZ::UserSettings::CT_LOCAL);
            pState->m_lastSearchInFilesMode = m_gui->searchWhereComboBox->currentIndex();
        }
    }

    void LUAEditorFindDialog::SetToFindInAllOpen(bool findInAny)
    {
        m_bWasFindInAll = findInAny;
        // restore prior global mode:
        if (m_bAnyDocumentsOpen)
        {
            auto pState = AZ::UserSettings::Find<LUAEditorInternal::FindSavedState>(findInAny ? AZ_CRC_CE("LUAFindInAny") : AZ_CRC_CE("FindInCurrent"), AZ::UserSettings::CT_LOCAL);
            if (pState)
            {
                m_gui->searchWhereComboBox->setCurrentIndex(pState->m_lastSearchInFilesMode); // theres three options!
                m_gui->findNextButton->setEnabled(m_gui->searchWhereComboBox->currentIndex() == 0);
            }
            else
            {
                // TODO: When the asset database is in use,  this test may want to default to AllLUAAssets instead
                m_gui->searchWhereComboBox->setCurrentIndex(findInAny ? AllOpenDocs : CurrentDoc);
                m_gui->findNextButton->setEnabled(m_gui->searchWhereComboBox->currentIndex() == 0);
            }
        }
        else
        {
            m_gui->searchWhereComboBox->setCurrentIndex(CurrentDoc); // theres only one option, no files are open
            m_gui->findNextButton->setEnabled(false);
        }
        m_gui->findNextButton->setDefault(m_gui->findNextButton->isEnabled());
        m_gui->findAllButton->setDefault(!m_gui->findNextButton->isEnabled());
        m_gui->findNextButton->setAutoDefault(m_gui->findNextButton->isEnabled());
        m_gui->findAllButton->setAutoDefault(!m_gui->findNextButton->isEnabled());
        m_gui->findAllButton->setEnabled(pLUAEditorMainWindow->HasAtLeastOneFileOpen());
        m_gui->replaceButton->setEnabled(pLUAEditorMainWindow->HasAtLeastOneFileOpen());
        m_gui->replaceAllButton->setEnabled(pLUAEditorMainWindow->HasAtLeastOneFileOpen());

        if (m_bWasFindInAll)
        {
            this->setWindowTitle("Find in files...");
        }
        else
        {
            this->setWindowTitle("Find...");
        }
    }

    void LUAEditorFindDialog::ResetSearch()
    {
        m_bFoundFirst = false;
    }


    // currently used to mark a wrap point for multiple-view searching
    void LUAEditorFindDialog::SetNewSearchStarting(bool bOverride, bool bSearchForwards)
    {
        if (bOverride && bSearchForwards)
        {
            m_gui->searchDownRadioButton->setChecked(true);
        }
        else if (bOverride)
        {
            m_gui->searchUpRadioButton->setChecked(true);
        }

        m_WrapWidget = GetViewFromParent();
        if (m_WrapWidget)
        {
            m_WrapWidget->GetCursorPosition(m_WrapLine, m_WrapIndex);
        }
    }

    void LUAEditorFindDialog::OnFindNext()
    {
        if (m_bFindThreadRunning)
        {
            m_bCancelFindSignal = true;
        }

        LUAViewWidget* pLUAViewWidget = pLUAEditorMainWindow->GetCurrentView();
        if (!pLUAViewWidget)
        {
            return;
        }

        if (!m_gui->txtFind->text().isEmpty())
        {
            if (m_lastSearchText != m_gui->txtFind->text() || m_bLastWrap != m_gui->wrapCheckBox->isChecked())
            {
                m_bFoundFirst = false;
            }

            //if we switched from finding forward to backward after finding a first
            //the the cursor will be in front of the last find, so the first finding backward
            //will re-find the current find (the one we are already found). This is unintuitive
            //to the user so instead we find backward to move the cursor to the beginning
            //of the current selection then call find next to actually find the next previous
            //occurrence as the user intended
            //whats strange is this only happens for find backward, find forward seems
            //to be smart enough to take this into account already.... so on do another find next
            //if searching up
            if (m_bFoundFirst &&
                m_bLastForward != m_gui->searchDownRadioButton->isChecked())
            {
                m_findOperation = pLUAViewWidget->FindFirst(m_gui->txtFind->text(),
                        m_gui->regularExpressionCheckBox->isChecked(),
                        m_gui->caseSensitiveCheckBox->isChecked(),
                        m_gui->wholeWordsCheckBox->isChecked(),
                        m_gui->wrapCheckBox->isChecked(),
                        m_gui->searchDownRadioButton->isChecked());
                if (!m_gui->searchDownRadioButton->isChecked())
                {
                    pLUAViewWidget->FindNext(m_findOperation);
                }
            }
            else if (m_bFoundFirst)
            {
                pLUAViewWidget->FindNext(m_findOperation);
            }
            else
            {
                // if moving backwards move the last result's cursor back one character
                // because it's placed at the end of the word and searching back from
                // the end of the word simply returns the word again
                if (!m_gui->searchDownRadioButton->isChecked())
                {
                    pLUAViewWidget->MoveCursor(-1);
                }
                m_findOperation = pLUAViewWidget->FindFirst(m_gui->txtFind->text(),
                        m_gui->regularExpressionCheckBox->isChecked(),
                        m_gui->caseSensitiveCheckBox->isChecked(),
                        m_gui->wholeWordsCheckBox->isChecked(),
                        m_gui->wrapCheckBox->isChecked(),
                        m_gui->searchDownRadioButton->isChecked());
            }

            m_lastSearchText = m_gui->txtFind->text();
            m_bLastForward = m_gui->searchDownRadioButton->isChecked();
            m_bLastWrap = m_gui->wrapCheckBox->isChecked();
        }
        else
        {
            QMessageBox::warning(this, "Error!", "You may not search for an empty string!");
        }

        if (!m_findOperation)
        {
            QMessageBox::warning(this, "Search failed!", tr("Could not find \"%1\" within/further this context.").arg(m_gui->txtFind->text()));
        }
    }

    void LUAEditorFindDialog::FindInView(LUAViewWidget* pLUAViewWidget, QListWidget* pCurrentFindListView)
    {
        if (!pLUAViewWidget)
        {
            return;
        }

        pLUAViewWidget->SetCursorPosition(0, 0);

        auto findOperation = pLUAViewWidget->FindFirst(m_gui->txtFind->text(),
                m_gui->regularExpressionCheckBox->isChecked(),
                m_gui->caseSensitiveCheckBox->isChecked(),
                m_gui->wholeWordsCheckBox->isChecked(),
                false,
                m_gui->searchDownRadioButton->isChecked());

        if (findOperation)
        {
            emit triggerFindNextInView(&findOperation, pLUAViewWidget, pCurrentFindListView);
        }
    }

    void LUAEditorFindDialog::FindNextInView(LUAViewWidget::FindOperation* operation, LUAViewWidget* pLUAViewWidget, QListWidget* pCurrentFindListView)
    {
        int line = 0;
        int index = 0;
        pLUAViewWidget->GetCursorPosition(line, index);

        QString itemText;
        QString lineTxt;
        lineTxt.setNum(line + 1); //files are 1 based
        AZStd::string assetFileName(pLUAViewWidget->m_Info.m_assetName + ".lua");
        itemText = assetFileName.c_str();
        itemText += "(";
        itemText += lineTxt;
        itemText += "):     ";
        itemText += pLUAViewWidget->GetLineText(line).trimmed();

        QListWidgetItem* pItem = new QListWidgetItem(pCurrentFindListView);
        pItem->setText(itemText);
        //char buf[64];
        //pLUAViewWidget->m_Info.m_assetId.ToString(buf,AZ_ARRAY_SIZE(buf),true,true);
        pItem->setData(Qt::UserRole + 1, QVariant(/*buf*/ pLUAViewWidget->m_Info.m_assetId.c_str()));
        AZStd::string assetFileName2(pLUAViewWidget->m_Info.m_assetName + ".lua");
        pItem->setData(Qt::UserRole + 2, QVariant(assetFileName2.c_str()));
        pItem->setData(Qt::UserRole + 3, QVariant(line));
        pItem->setData(Qt::UserRole + 4, QVariant(index));
        pItem->setData(Qt::UserRole + 5, QVariant(m_gui->txtFind->text().length()));

        pCurrentFindListView->addItem(pItem);

        pLUAViewWidget->FindNext(*operation);

        if (operation && !m_bCancelFindSignal)
        {
            emit triggerFindNextInView(operation, pLUAViewWidget, pCurrentFindListView);
        }
        else
        {
            BusyOff();
        }
    }

    void LUAEditorFindDialog::OnFindAll()
    {
        if (m_bReplaceThreadRunning)
        {
            QMessageBox::warning(this, "Error!", "You may not run Find ALL while a Replace All is running!");
            return;
        }

        m_resultList.clear();

        m_bCancelFindSignal = false;
        m_bFindThreadRunning = true;

        int widgetIndex = 0;
        if (m_gui->m_find1RadioButton->isChecked())
        {
            widgetIndex = 0;
        }
        else if (m_gui->m_find2RadioButton->isChecked())
        {
            widgetIndex = 1;
        }
        else if (m_gui->m_find3RadioButton->isChecked())
        {
            widgetIndex = 2;
        }
        else if (m_gui->m_find4RadioButton->isChecked())
        {
            widgetIndex = 3;
        }

        pLUAEditorMainWindow->SetCurrentFindListWidget(widgetIndex);
        auto resultsWidget = pLUAEditorMainWindow->GetFindResultsWidget(widgetIndex);
        resultsWidget->Clear();

        pLUAEditorMainWindow->ResetSearchClicks();

        if (!m_gui->txtFind->text().isEmpty())
        {
            int theMode = m_gui->searchWhereComboBox->currentIndex();
            if (!m_bAnyDocumentsOpen)
            {
                // TODO: Enable when the asset database is used.
                //theMode = AllLUAAssets;
            }
            else
            {
                theMode = AllOpenDocs;
            }

            BusyOn();

            m_lastSearchWhere = theMode;
            if (theMode == CurrentDoc)
            {
                FindInFilesSetUp(theMode, resultsWidget);
                emit triggerFindInFilesNext(theMode);
            }
            else if (theMode == AllOpenDocs)
            {
                FindInFilesSetUp(theMode, resultsWidget);
                emit triggerFindInFilesNext(theMode);
            }
            else if (theMode == AllLUAAssets)
            {
                FindInFilesSetUp(theMode, resultsWidget);
                emit triggerFindInFilesNext(theMode);
            }
        }
        else
        {
            QMessageBox::warning(this, "Error!", "You may not search for an empty string!");
        }
    }

    void LUAEditorFindDialog::FindInFilesSetUp(int theMode, FindResults* resultsWidget)
    {
        m_FIFData.m_TotalMatchesFound = 0;

        m_FIFData.m_OpenView = pLUAEditorMainWindow->GetAllViews();

        if (theMode == CurrentDoc)
        {
            m_FIFData.m_openViewIter = m_FIFData.m_OpenView.begin();
            while (*m_FIFData.m_openViewIter != pLUAEditorMainWindow->GetCurrentView() && m_FIFData.m_openViewIter != m_FIFData.m_OpenView.end())
            {
                ++m_FIFData.m_openViewIter;
            }
        }
        else
        {
            m_FIFData.m_openViewIter = m_FIFData.m_OpenView.begin();
        }

        m_FIFData.m_resultsWidget = resultsWidget;


        //get a list of all the lua assets info
        m_dFindAllLUAAssetsInfo.clear();
        if (theMode == AllLUAAssets)
        {
            AZ_Assert(false, "Fix assets!");
            //AZ::u32 platformFeatureFlags = PLATFORM_FEATURE_FLAGS_ALL;
            //EditorFramework::EditorAssetCatalogMessages::Bus::BroadcastResult(
            //    platformFeatureFlags, &EditorFramework::EditorAssetCatalogMessages::Bus::Events::GetCurrentPlatformFeatureFlags);
            //EditorFramework::EditorAssetCatalogMessages::Bus::Broadcast(
            //    &EditorFramework::EditorAssetCatalogMessages::Bus::Events::FindEditorAssetsByType,
            //    m_dFindAllLUAAssetsInfo,
            //    AZ::ScriptAsset::StaticAssetType(),
            //    platformFeatureFlags);
        }


        m_SearchText = m_gui->txtFind->text();
        m_FIFData.m_dOpenView.clear();

        m_FIFData.m_bWholeWordIsChecked = m_gui->wholeWordsCheckBox->isChecked();
        m_FIFData.m_bRegExIsChecked = m_gui->regularExpressionCheckBox->isChecked();
        m_FIFData.m_bCaseSensitiveIsChecked = m_gui->caseSensitiveCheckBox->isChecked();

        if (m_FIFData.m_bWholeWordIsChecked)
        {
            if (!m_SearchText.startsWith("\b", Qt::CaseInsensitive) &&
                !m_SearchText.endsWith("\b", Qt::CaseInsensitive))
            {
                m_FIFData.m_SearchText = "\\b";
                m_FIFData.m_SearchText += m_SearchText;
                m_FIFData.m_SearchText += "\\b";
            }
        }
        else
        {
            m_FIFData.m_SearchText = m_SearchText;
        }

        m_FIFData.m_assetInfoIter = m_dFindAllLUAAssetsInfo.begin();
    }

    void LUAEditorFindDialog::FindInFilesNext(int theMode)
    {
        // one at a time parse the open documents and store their name strings away
        if ((!m_bCancelFindSignal) && m_FIFData.m_openViewIter != m_FIFData.m_OpenView.end())
        {
            // this block mimics the original FindInView() call, localized to the FIF callback scheme

            bool syncResult = pLUAEditorMainWindow->SyncDocumentToContext((*m_FIFData.m_openViewIter)->m_Info.m_assetId);
            if (syncResult)
            {
                // successful sync between the QScintilla document and the raw buffer version that we need
                const char* buffer = nullptr;
                AZStd::size_t actualSize = 0;
                Context_DocumentManagement::Bus::Broadcast(
                    &Context_DocumentManagement::Bus::Events::GetDocumentData,
                    (*m_FIFData.m_openViewIter)->m_Info.m_assetId,
                    &buffer,
                    actualSize);

                /************************************************************************/
                /* Open files are similar but not identical to closed file processing   */
                /************************************************************************/
                char* pBuf = (char*)azmalloc(actualSize + 1);
                char* pCur = pBuf;
                pCur[actualSize] = '\0';
                memcpy(pCur, buffer, actualSize);

                AZStd::vector<char*> dLines;
                AZStd::string assetName = (*m_FIFData.m_openViewIter)->m_Info.m_assetName + ".lua";

                while (aznumeric_cast<size_t>(pCur - pBuf) <= actualSize)
                {
                    dLines.push_back(pCur);
                    while (*pCur != '\n' && aznumeric_cast<size_t>(pCur - pBuf) <= actualSize)
                    {
                        pCur++;
                    }

                    if (aznumeric_cast<size_t>(pCur - pBuf) <= actualSize)
                    {
                        *pCur = '\0';
                        pCur++;
                    }
                }
                for (int line = 0; line < dLines.size(); ++line)
                {
                    ResultEntry entry;
                    entry.m_lineText = dLines[line];

                    QRegExp regex(m_FIFData.m_SearchText, m_FIFData.m_bCaseSensitiveIsChecked ? Qt::CaseSensitive : Qt::CaseInsensitive);
                    int index = 0;
                    if (m_FIFData.m_bRegExIsChecked || m_FIFData.m_bWholeWordIsChecked)
                    {
                        index = entry.m_lineText.indexOf(regex, index);
                    }
                    else
                    {
                        index = entry.m_lineText.indexOf(m_FIFData.m_SearchText, index, m_FIFData.m_bCaseSensitiveIsChecked ? Qt::CaseSensitive : Qt::CaseInsensitive);
                    }

                    if (index > -1)
                    {
                        const auto& docInfo = (*m_FIFData.m_openViewIter)->m_Info;
                        ++m_FIFData.m_TotalMatchesFound;
                        QString qAssetName = assetName.c_str();

                        if (m_resultList.find(qAssetName) == m_resultList.end())
                        {
                            m_resultList[qAssetName].m_assetId = docInfo.m_assetId;
                        }
                        entry.m_lineNumber = line + 1;
                        entry.m_lineText = entry.m_lineText.trimmed();

                        while (index > -1)
                        {
                            if (m_FIFData.m_bRegExIsChecked || m_FIFData.m_bWholeWordIsChecked)
                            {
                                entry.m_matches.push_back(AZStd::make_pair(index, regex.matchedLength()));
                            }
                            else
                            {
                                entry.m_matches.push_back(AZStd::make_pair(index, m_FIFData.m_SearchText.length()));
                            }

                            index++;
                            if (m_FIFData.m_bRegExIsChecked || m_FIFData.m_bWholeWordIsChecked)
                            {
                                index = entry.m_lineText.indexOf(regex, index);
                            }
                            else
                            {
                                index = entry.m_lineText.indexOf(m_FIFData.m_SearchText, index, m_FIFData.m_bCaseSensitiveIsChecked ? Qt::CaseSensitive : Qt::CaseInsensitive);
                            }
                        }

                        m_resultList[qAssetName].m_entries.push_back(entry);
                    }
                }

                azfree(pBuf);
            }

            /************************************************************************/
            /*                                                                      */
            /************************************************************************/

            if (theMode == CurrentDoc)
            {
                m_FIFData.m_dOpenView.push_back((*m_FIFData.m_openViewIter)->m_Info.m_assetName + ".lua");
                m_FIFData.m_openViewIter = m_FIFData.m_OpenView.end();
            }
            else
            {
                ++m_FIFData.m_openViewIter;
                if (m_FIFData.m_openViewIter == m_FIFData.m_OpenView.end())
                {
                    for (AZStd::vector<LUAViewWidget*>::iterator openViewIter = m_FIFData.m_OpenView.begin();
                         openViewIter != m_FIFData.m_OpenView.end(); ++openViewIter)
                    {
                        m_FIFData.m_dOpenView.push_back((*openViewIter)->m_Info.m_assetName + ".lua");
                    }
                }
            }

            emit triggerFindInFilesNext(theMode);
            return;
        }

        if ((!m_bCancelFindSignal) && m_FIFData.m_assetInfoIter != m_dFindAllLUAAssetsInfo.end())
        {
            // from the previously prepared list of strings of names of open files, is THIS asset open?
            bool bIsOpen = false;
            for (AZStd::vector<AZStd::string>::iterator iter = m_FIFData.m_dOpenView.begin();
                 !bIsOpen && iter != m_FIFData.m_dOpenView.end(); ++iter)
            {
                AZ_Assert(false, "check under!");

                //if (*iter == m_FIFData.m_assetInfoIter->m_physicalPath)
                if (*iter == *m_FIFData.m_assetInfoIter)
                {
                    bIsOpen = true;
                    break;
                }
            }

            if (!bIsOpen)
            {
                /*  AZ::IO::Stream* pStream = AZ::IO::g_streamer->RegisterStream(m_FIFData.m_assetInfoIter->m_StreamName.c_str());

                    if (!pStream)
                    {
                        AZ_TracePrintf("LUA", "FindInFilesNext (%s) NULL stream\n",m_FIFData.m_assetInfoIter->m_StreamName);
                    }
                    else
                    {
                        size_t fileSize = pStream->Size();
                        char* buf = (char*)dhmalloc(fileSize+1);
                        buf[fileSize]='\0';
                        size_t offset=0;
                        while(size_t bytesRead = AZ::IO::g_streamer->Read(pStream,offset,fileSize-offset,buf+offset))
                        {
                            offset += bytesRead;
                            if(offset == fileSize)
                                break;
                        }
                        AZ::IO::g_streamer->UnRegisterStream(pStream);

                        char* pCur = buf;
                        AZStd::vector<char*> dLines;
                        while(aznumeric_cast<size_t>(pCur - buf) <= fileSize)
                        {
                            dLines.push_back(pCur);
                            while(*pCur != '\n' && aznumeric_cast<size_t>(pCur - buf) <= fileSize)
                                pCur++;

                            if(aznumeric_cast<size_t>(pCur - buf) <= fileSize)
                            {
                                *pCur = '\0';
                                pCur++;
                            }
                        }
                        for(int line=0; line<dLines.size(); ++line)
                        {
                            ResultEntry entry;
                            entry.m_lineText = dLines[line];

                            QRegExp regex(m_FIFData.m_SearchText, m_FIFData.m_bCaseSensitiveIsChecked ? Qt::CaseSensitive : Qt::CaseInsensitive);
                            int index = 0;
                            if(m_FIFData.m_bRegExIsChecked || m_FIFData.m_bWholeWordIsChecked)
                                index = entry.m_lineText.indexOf(regex, index);
                            else
                                index = entry.m_lineText.indexOf(m_FIFData.m_SearchText, index, m_FIFData.m_bCaseSensitiveIsChecked ? Qt::CaseSensitive : Qt::CaseInsensitive);

                            if(index > -1)
                            {
                                ++m_FIFData.m_TotalMatchesFound;
                                QString qAssetName = assetName.c_str();

                                if (m_resultList.find(qAssetName) == m_resultList.end())
                                {
                                    m_resultList[qAssetName].m_assetId = AZ::Data::AssetId::CreateNull();
                                }
                                entry.m_lineNumber = line;
                                entry.m_lineText = entry.m_lineText.trimmed();

                                while (index > -1)
                                {
                                    if (m_FIFData.m_bRegExIsChecked || m_FIFData.m_bWholeWordIsChecked)
                                    {
                                        entry.m_matches.push_back(AZStd::make_pair(index, regex.matchedLength()));
                                    }
                                    else
                                    {
                                        entry.m_matches.push_back(AZStd::make_pair(index, m_FIFData.m_SearchText.length()));
                                    }

                                    index++;
                                    if (m_FIFData.m_bRegExIsChecked || m_FIFData.m_bWholeWordIsChecked)
                                        index = entry.m_lineText.indexOf(regex, index);
                                    else
                                        index = entry.m_lineText.indexOf(m_FIFData.m_SearchText, index, m_FIFData.m_bCaseSensitiveIsChecked ? Qt::CaseSensitive : Qt::CaseInsensitive);
                                }

                                m_resultList[qAssetName].m_entries.push_back(entry);
                            }
                        }
                        dhfree(buf);
                    }
                    */
            }

            ++m_FIFData.m_assetInfoIter;
            emit triggerFindInFilesNext(theMode);
            return;
        }

        if (!m_resultList.empty() || m_bCancelFindSignal)
        {
            if (!m_bCancelFindSignal)
            {
                PostProcessOn();
                AZ::SystemTickBus::QueueFunction(&LUAEditorFindDialog::ProcessFindItems, this);
            }
            else
            {
                BusyOff();
            }

            m_bFindThreadRunning = false;
            AZ_TracePrintf("LUA", "Find In Files Matches Found = %d\n", m_FIFData.m_TotalMatchesFound);
        }
        else
        {
            m_bFindThreadRunning = false;
            BusyOff();
        }
    }

    void LUAEditorFindDialog::ProcessFindItems()
    {
        auto doc = m_FIFData.m_resultsWidget->Document();
        auto setFoldLevel = [&doc](int lineNum, int foldLevel, int depth)
            {
                QTBlockState blockState;
                auto block = doc->findBlockByNumber(lineNum);
                AZ_Assert(block.isValid(), "should only be setting fold level on a just added line");
                blockState.m_qtBlockState = block.userState();
                blockState.m_blockState.m_uninitialized = 0;
                blockState.m_blockState.m_folded = 0;
                blockState.m_blockState.m_foldLevel = foldLevel;
                blockState.m_blockState.m_syntaxHighlighterState = depth;
                block.setUserState(blockState.m_qtBlockState);
            };

        int currentLine = 0;
        m_FIFData.m_resultsWidget->Clear();
        QString hits;
        if (m_FIFData.m_TotalMatchesFound == 1)
        {
            hits.append("hit");
        }
        else
        {
            hits.append("hits");
        }

        QString files;
        if (m_resultList.size() == 1)
        {
            files.append("file");
        }
        else
        {
            files.append("files");
        }

        QString header("Find \"%1\" (%2 %4 in %3 %5)");
        header = header.arg(m_FIFData.m_SearchText).arg(m_FIFData.m_TotalMatchesFound).arg(m_resultList.size()).arg(hits).arg(files);
        m_FIFData.m_resultsWidget->AppendPlainText(header);
        setFoldLevel(currentLine, 0, 0);

        for (auto iter = m_resultList.begin(); iter != m_resultList.end(); ++iter)
        {
            ++currentLine;
            if (iter->m_entries.size() == 1)
            {
                hits = "hit";
            }
            else
            {
                hits = "hits";
            }

            QString text("\t\"%1\" (%2 %3)");
            text = text.arg(iter.key()).arg(iter->m_entries.size()).arg(hits);
            m_FIFData.m_resultsWidget->AppendPlainText(text);
            setFoldLevel(currentLine, 1, 1);
            for (auto& entry : iter->m_entries)
            {
                ++currentLine;
                text = "\t\t\tLine %1: %2";
                text = text.arg(entry.m_lineNumber).arg(entry.m_lineText);
                m_FIFData.m_resultsWidget->AppendPlainText(text);

                AZ_Assert(!entry.m_matches.empty(), "shouldn't be an entry at all if there wasn't at least one match");
                setFoldLevel(currentLine, 1, 2);

                auto block = doc->findBlockByNumber(currentLine);
                AZ_Assert(block.isValid(), "should only be setting data on a just added line");
                auto resultsWidget = m_FIFData.m_resultsWidget;
                auto assignAssetId = [resultsWidget](const AZStd::string& assetName, const AZStd::string& assetId) {resultsWidget->AssignAssetId(assetName, assetId); };
                block.setUserData(aznew FindResultsBlockInfo {iter->m_assetId, iter.key().toUtf8().data(), entry.m_lineNumber,
                                                              entry.m_matches[0].first, assignAssetId});
            }

            setFoldLevel(currentLine, 0, 2);
        }
        m_FIFData.m_resultsWidget->FinishedAddingText(m_FIFData.m_SearchText, m_FIFData.m_bRegExIsChecked,
            m_FIFData.m_bWholeWordIsChecked, m_FIFData.m_bCaseSensitiveIsChecked);

        BusyOff();

        int findWindow = 0;
        if (m_gui->m_find1RadioButton->isChecked())
        {
            findWindow = 0;
        }
        else if (m_gui->m_find2RadioButton->isChecked())
        {
            findWindow = 1;
        }
        else if (m_gui->m_find3RadioButton->isChecked())
        {
            findWindow = 2;
        }
        else if (m_gui->m_find4RadioButton->isChecked())
        {
            findWindow = 3;
        }

        //get the find tab and let see it
        QTabWidget* pLuaFindTabWidget = pLUAEditorMainWindow->GetFindTabWidget();
        AZ_Assert(pLuaFindTabWidget, "LUAEditorMainWindow cant find the FindTabWidget!");
        pLuaFindTabWidget->show();
        pLuaFindTabWidget->raise();
        pLuaFindTabWidget->activateWindow();
        pLuaFindTabWidget->setFocus();
        pLUAEditorMainWindow->OnOpenFindView(findWindow);

        m_resultList.clear();
    }

    void LUAEditorFindDialog::OnCancel()
    {
        m_bCancelFindSignal = true;
        m_bCancelReplaceSignal = true;
        //this->close();
    }

    void LUAEditorFindDialog::OnReplace()
    {
        if (m_bFindThreadRunning)
        {
            m_bCancelFindSignal = true;
        }

        LUAViewWidget* pLUAViewWidget = GetViewFromParent();
        if (!pLUAViewWidget)
        {
            return;
        }
        if (pLUAViewWidget->HasSelectedText())
        {
            if (pLUAViewWidget->m_Info.m_bSourceControl_BusyRequestingEdit ||
                pLUAViewWidget->m_Info.m_bSourceControl_BusyGettingStats ||
                pLUAViewWidget->m_Info.m_bSourceControl_Ready == false)
            {
                AZ::SystemTickBus::QueueFunction(&LUAEditorFindDialog::OnReplace, this);
            }
            else if (!pLUAViewWidget->m_Info.m_bSourceControl_CanWrite &&
                     pLUAViewWidget->m_Info.m_bSourceControl_CanCheckOut)
            {
                // check it out for edit
                Context_DocumentManagement::Bus::Broadcast(
                    &Context_DocumentManagement::Bus::Events::DocumentCheckOutRequested, pLUAViewWidget->m_Info.m_assetId);
                AZ::SystemTickBus::QueueFunction(&LUAEditorFindDialog::OnReplace, this);
            }
            else if (!pLUAViewWidget->m_Info.m_bSourceControl_CanWrite)
            {
                QMessageBox::warning(this, "Error!", "Can not check out file for replace!");
            }
            else
            {
                pLUAViewWidget->ReplaceSelectedText(m_gui->txtReplaceWith->text());

                int startLine = 0;
                int startIndex = 0;
                pLUAViewWidget->GetCursorPosition(startLine, startIndex);
                pLUAViewWidget->SetCursorPosition(startLine, startIndex + 1);

                OnFindNext();
            }
        }
        else
        {
            OnFindNext();
        }
    }

    void LUAEditorFindDialog::ReplaceInFilesSetUp()
    {
        BusyOn();

        m_bReplaceThreadRunning = true;
        m_bCancelReplaceSignal = false;

        //get a list of all the lua assets info
        m_RIFData.m_dReplaceProcessList.clear();
        m_RIFData.m_dReplaceAllLUAAssetsInfo.clear();

        AZ_Assert(false, "Fix assets!");

        //AZ::u32 platformFeatureFlags = PLATFORM_FEATURE_FLAGS_ALL;
        //EditorFramework::EditorAssetCatalogMessages::Bus::BroadcastResult(
        //    platformFeatureFlags, &EditorFramework::EditorAssetCatalogMessages::Bus::Events::GetCurrentPlatformFeatureFlags);
        //EditorFramework::EditorAssetCatalogMessages::Bus::Broadcast(
        //    &EditorFramework::EditorAssetCatalogMessages::Bus::Events::FindEditorAssetsByType,
        //    m_RIFData.m_dReplaceAllLUAAssetsInfo,
        //    AZ::ScriptAsset::StaticAssetType(),
        //    platformFeatureFlags);

        m_RIFData.m_OpenView = pLUAEditorMainWindow->GetAllViews();

        m_SearchText = m_gui->txtFind->text();
        m_RIFData.m_dOpenView.clear();

        for (auto iter = m_RIFData.m_OpenView.begin(); iter != m_RIFData.m_OpenView.end(); ++iter)
        {
            m_RIFData.m_dOpenView.push_back((*iter)->m_Info.m_assetName + ".lua");
        }

        m_RIFData.m_bWholeWordIsChecked = m_gui->wholeWordsCheckBox->isChecked();
        m_RIFData.m_bRegExIsChecked = m_gui->regularExpressionCheckBox->isChecked();
        m_RIFData.m_bCaseSensitiveIsChecked = m_gui->caseSensitiveCheckBox->isChecked();

        if (m_RIFData.m_bWholeWordIsChecked)
        {
            if (!m_SearchText.startsWith("\b", Qt::CaseInsensitive) &&
                !m_SearchText.endsWith("\b", Qt::CaseInsensitive))
            {
                m_RIFData.m_SearchText = "\\b";
                m_RIFData.m_SearchText += m_SearchText;
                m_RIFData.m_SearchText += "\\b";
            }
        }
        else
        {
            m_RIFData.m_SearchText = m_SearchText;
        }

        m_RIFData.m_assetInfoIter = m_RIFData.m_dReplaceAllLUAAssetsInfo.begin();
    }

    void LUAEditorFindDialog::ReplaceInFilesNext()
    {
        if (m_bCancelReplaceSignal)
        {
            BusyOff();
            m_bReplaceThreadRunning = false;
            m_bCancelReplaceSignal = false;
            return; // return with no timer set to call back ends this run
        }

        if (m_RIFData.m_assetInfoIter != m_RIFData.m_dReplaceAllLUAAssetsInfo.end())
        {
            //for each asset name, if the asset is not open already, open it search it and close it
            {
                bool bIsOpen = false;
                for (AZStd::vector<AZStd::string>::iterator openViewIter = m_RIFData.m_dOpenView.begin();
                     openViewIter != m_RIFData.m_dOpenView.end(); ++openViewIter)
                {
                    // is this the right check?
                    if (*openViewIter == *m_RIFData.m_assetInfoIter)
                    //if (*openViewIter == m_RIFData.m_assetInfoIter->m_physicalPath)
                    {
                        bIsOpen = true;
                        break;
                    }
                }

                if (!bIsOpen)
                {
                    /*
                    AZ::IO::Stream* pStream = AZ::IO::g_streamer->RegisterStream(m_RIFData.m_assetInfoIter->m_StreamName.c_str());

                    if (!pStream)
                    {
                        AZ_TracePrintf("LUA", "ReplaceInFilesNext (%s) NULL stream\n",m_FIFData.m_assetInfoIter->m_StreamName);
                    }
                    else
                    {
                        size_t fileSize = pStream->Size();
                        char* buf = (char*)dhmalloc(fileSize+1);
                        buf[fileSize]=NULL;
                        size_t offset=0;
                        while(size_t bytesRead = AZ::IO::g_streamer->Read(pStream,offset,fileSize-offset,buf+offset))
                        {
                            offset += bytesRead;
                            if(offset == fileSize)
                                break;
                        }
                        AZ::IO::g_streamer->UnRegisterStream(pStream);

                        char* pCur = buf;
                        AZStd::vector<char*> dLines;
                        while(aznumeric_cast<size_t>(pCur - buf) <= fileSize)
                        {
                            dLines.push_back(pCur);
                            while(*pCur != '\n' && aznumeric_cast<size_t>(pCur - buf) <= fileSize)
                                pCur++;

                            if(aznumeric_cast<size_t>(pCur - buf) <= fileSize)
                            {
                                *pCur = NULL;
                                pCur++;
                            }
                        }
                        for(AZStd::size_t line=0; line<dLines.size(); ++line)
                        {
                            QString str(dLines[line]);
                            QRegExp regex(m_RIFData.m_SearchText, m_bCaseSensitiveIsChecked ? Qt::CaseSensitive : Qt::CaseInsensitive);
                            int index = 0;
                            if(m_bRegExIsChecked || m_bWholeWordIsChecked)
                                index = str.indexOf(regex, index);
                            else
                                index = str.indexOf(m_RIFData.m_SearchText, index, m_bCaseSensitiveIsChecked ? Qt::CaseSensitive : Qt::CaseInsensitive);

                            if(index > -1)
                            {
                                m_RIFData.m_dReplaceProcessList.push_back(assetName.c_str());
                                line = dLines.size();
                            }
                        }
                        delete buf;
                    }
                */
                }
            }

            ++m_RIFData.m_assetInfoIter;

            // are we done with the search and dispatch?
            if (m_RIFData.m_assetInfoIter == m_RIFData.m_dReplaceAllLUAAssetsInfo.end())
            {
                if (!m_RIFData.m_dReplaceProcessList.empty())
                {
                    PostReplaceOn();
                    QTimer::singleShot(0, this, &LUAEditorFindDialog::ProcessReplaceItems);
                }
                else
                {
                    m_bReplaceThreadRunning = false;
                    m_bCancelReplaceSignal = false;
                    BusyOff();
                }

                return;
            }

            QTimer::singleShot(1, this, &LUAEditorFindDialog::ReplaceInFilesNext);
        }
    }

    void LUAEditorFindDialog::ProcessReplaceItems()
    {
        if (m_bCancelReplaceSignal)
        {
            BusyOff();
            m_bReplaceThreadRunning = false;
            m_bCancelReplaceSignal = false;
            return; // return with no timer set to call back ends this run
        }

        if (!m_RIFData.m_dReplaceProcessList.empty())
        {
            AZStd::string assetName = AZStd::move(m_RIFData.m_dReplaceProcessList.back());
            m_RIFData.m_dReplaceProcessList.pop_back();
            m_RIFData.m_waitingForOpenToComplete.insert(assetName);

            //split physical path into the components saved by the database
            AZStd::string projectRoot, databaseRoot, databasePath, databaseFile, fileExtension;
            if (!AZ::StringFunc::AssetDatabasePath::Split(assetName.c_str(), &projectRoot, &databaseRoot, &databasePath, &databaseFile, &fileExtension))
            {
                AZ_Warning("LUAEditorFindDialog", false, AZStd::string::format("<span severity=\"err\">Path is invalid: '%s'</span>", assetName.c_str()).c_str());
                return;
            }

            //find it in the database
            //AZStd::vector<EditorFramework::EditorAsset> dAssetInfo;
            //EditorFramework::EditorAssetCatalogMessages::Bus::Broadcast(
            //    &EditorFramework::EditorAssetCatalogMessages::Bus::Events::FindEditorAssetsByName,
            //    dAssetInfo,
            //    databaseRoot.c_str(),
            //    databasePath.c_str(),
            //    databaseFile.c_str(),
            //    fileExtension.c_str());
            //if (dAssetInfo.empty())
            //  return;

            //request it be opened
            //EditorFramework::AssetManagementMessages::Bus::Event(
            //    LUAEditor::ContextID,
            //    &EditorFramework::AssetManagementMessages::Bus::Events::AssetOpenRequested,
            //    dAssetInfo[0].m_databaseAsset.m_assetId,
            //    AZ::ScriptAsset::StaticAssetType());

            QTimer::singleShot(0, this, &LUAEditorFindDialog::ProcessReplaceItems);
        }
        else
        {
            // getting here means we've dispatched all the requests to open the files.
            if ((m_PendingReplaceInViewOperations.empty()) && (m_RIFData.m_waitingForOpenToComplete.empty()))
            {
                BusyOff();
                m_bReplaceThreadRunning = false;
                m_bCancelReplaceSignal = false;
            }
        }
    }

    void LUAEditorFindDialog::OnDataLoadedAndSet(const DocumentInfo& info, LUAViewWidget* pLUAViewWidget)
    {
        auto searcher = m_RIFData.m_waitingForOpenToComplete.find(info.m_assetName);
        if (searcher != m_RIFData.m_waitingForOpenToComplete.end())
        {
            m_RIFData.m_waitingForOpenToComplete.erase(searcher);
            bool wasEmpty = m_PendingReplaceInViewOperations.empty();
            m_PendingReplaceInViewOperations.push_back(pLUAViewWidget);

            // only start iterating the first time:
            if (wasEmpty)
            {
                QTimer::singleShot(0, this, &LUAEditorFindDialog::OnReplaceInViewIterate);
            }
        }
    }

    void LUAEditorFindDialog::OnReplaceInViewIterate()
    {
        if ((m_PendingReplaceInViewOperations.empty()) && (m_RIFData.m_waitingForOpenToComplete.empty()))
        {
            BusyOff();
            m_bCancelReplaceSignal = false;
            m_bReplaceThreadRunning = false;
            return;
        }

        LUAViewWidget* pWidget = m_PendingReplaceInViewOperations.back();
        int result = ReplaceInView(pWidget);

        if (m_bCancelReplaceSignal)
        {
            BusyOff();
            m_bReplaceThreadRunning = false;
            m_bCancelReplaceSignal = false;
            return;
        }

        // any result besides -2 means we're done with this item:
        if (result != -2)
        {
            m_PendingReplaceInViewOperations.pop_back();
        }

        if (!m_PendingReplaceInViewOperations.empty())
        {
            QTimer::singleShot(0, this, &LUAEditorFindDialog::OnReplaceInViewIterate);
        }

        if ((m_PendingReplaceInViewOperations.empty()) && (m_RIFData.m_waitingForOpenToComplete.empty()))
        {
            BusyOff();
            m_bCancelReplaceSignal = false;
            m_bReplaceThreadRunning = false;
            return;
        }
    }

    int LUAEditorFindDialog::ReplaceInView(LUAViewWidget* pLUAViewWidget)
    {
        if (m_bCancelReplaceSignal)
        {
            BusyOff();
            m_bReplaceThreadRunning = false;
            m_bCancelReplaceSignal = false;
            return 0;
        }


        if (pLUAViewWidget->m_Info.m_bSourceControl_BusyRequestingEdit ||
            pLUAViewWidget->m_Info.m_bSourceControl_BusyGettingStats ||
            pLUAViewWidget->m_Info.m_bSourceControl_Ready == false)
        {
            return -2;
        }
        else if (!pLUAViewWidget->m_Info.m_bSourceControl_CanWrite &&
                 pLUAViewWidget->m_Info.m_bSourceControl_CanCheckOut)
        {
            // check it out for edit
            Context_DocumentManagement::Bus::Broadcast(
                &Context_DocumentManagement::Bus::Events::DocumentCheckOutRequested, pLUAViewWidget->m_Info.m_assetId);
            return -2;
        }
        else if (!pLUAViewWidget->m_Info.m_bSourceControl_CanWrite)
        {
            QMessageBox::warning(this, "Can not check out file!", (pLUAViewWidget->m_Info.m_assetName + ".lua").c_str());
            return -1;
        }

        int count = 0;
        pLUAViewWidget->SetCursorPosition(0, 0);
        const int advance = m_gui->txtReplaceWith->text().size();
        int firstFoundLine = 0;
        int firstFoundIndex = 0;
        if (pLUAViewWidget->FindFirst(m_gui->txtFind->text(),
            m_gui->regularExpressionCheckBox->isChecked(),
            m_gui->caseSensitiveCheckBox->isChecked(),
            m_gui->wholeWordsCheckBox->isChecked(),
            m_gui->wrapCheckBox->isChecked(),
            m_gui->searchDownRadioButton->isChecked()))
        {
            pLUAViewWidget->GetCursorPosition(firstFoundLine, firstFoundIndex);
            pLUAViewWidget->ReplaceSelectedText(m_gui->txtReplaceWith->text());
            count++;

            while (pLUAViewWidget->FindFirst(m_gui->txtFind->text(),
                m_gui->regularExpressionCheckBox->isChecked(),
                m_gui->caseSensitiveCheckBox->isChecked(),
                m_gui->wholeWordsCheckBox->isChecked(),
                m_gui->wrapCheckBox->isChecked(),
                m_gui->searchDownRadioButton->isChecked()))
            {
                int startLine = 0;
                int startIndex = 0;
                pLUAViewWidget->GetCursorPosition(startLine, startIndex);

                if (startLine == firstFoundLine && startIndex == firstFoundIndex)
                {
                    break;
                }

                pLUAViewWidget->ReplaceSelectedText(m_gui->txtReplaceWith->text());
                pLUAViewWidget->GetCursorPosition(startLine, startIndex);
                pLUAViewWidget->SetCursorPosition(startLine, startIndex + advance);

                count++;
            }
        }

        return count;
    }

    void LUAEditorFindDialog::OnReplaceAll()
    {
        if (m_bFindThreadRunning)
        {
            QMessageBox::warning(this, "Error!", "You may not run Replace ALL while a Find All is running!");
            return;
        }

        if (!m_gui->txtFind->text().isEmpty())
        {
            int theMode = m_gui->searchWhereComboBox->currentIndex();
            if (!m_bAnyDocumentsOpen)
            {
                theMode = AllLUAAssets;
            }

            m_lastSearchWhere = theMode;

            if (theMode == CurrentDoc)
            {
                BusyOn();
                m_PendingReplaceInViewOperations.push_back(pLUAEditorMainWindow->GetCurrentView());
                QTimer::singleShot(0, this, &LUAEditorFindDialog::OnReplaceInViewIterate);
            }
            else if (theMode == AllOpenDocs || theMode == AllLUAAssets)
            {
                BusyOn();

                AZStd::vector<LUAViewWidget*> dOpenView = pLUAEditorMainWindow->GetAllViews();
                for (AZStd::vector<LUAViewWidget*>::iterator openViewIter = dOpenView.begin(); openViewIter != dOpenView.end(); ++openViewIter)
                {
                    m_PendingReplaceInViewOperations.push_back(*openViewIter);
                }
                QTimer::singleShot(0, this, &LUAEditorFindDialog::OnReplaceInViewIterate);

                if (theMode == AllLUAAssets)
                {
                    ReplaceInFilesSetUp();
                    emit triggerReplaceInFilesNext();
                }
            }
        }
        else
        {
            QMessageBox::warning(this, "Error!", "You may not replace an empty string!");
        }
    }

    void LUAEditorFindDialog::BusyOn()
    {
        m_gui->cancelButton->setEnabled(true);
        m_gui->busyLabel->setText("Working");
    }
    void LUAEditorFindDialog::BusyOff()
    {
        m_gui->cancelButton->setEnabled(false);
        m_gui->busyLabel->setText("Idle");
    }
    void LUAEditorFindDialog::PostProcessOn()
    {
        m_gui->cancelButton->setEnabled(false);
        m_gui->busyLabel->setText("List Prep");
    }
    void LUAEditorFindDialog::PostReplaceOn()
    {
        BusyOff();
        m_bReplaceThreadRunning = false;

        m_gui->cancelButton->setEnabled(true);
        m_gui->busyLabel->setText("Replacing");
    }

    void LUAEditorFindDialog::showEvent(QShowEvent* event)
    {
        raise();
        QDialog::showEvent(event);
    }

    void LUAEditorFindDialog::Reflect(AZ::ReflectContext* reflection)
    {
        LUAEditorInternal::FindSavedState::Reflect(reflection);
    }
}//namespace LUAEditor

#include <Source/LUA/moc_LUAEditorFindDialog.cpp>
