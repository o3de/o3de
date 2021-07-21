/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "AttachmentsWindow.h"
#include <AzFramework/API/ApplicationAPI.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AttachmentNode.h>
#include <EMotionFX/Source/AttachmentSkin.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/FileManager.h>
#include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/SaveChangedFilesManager.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <QAction>
#include <QCheckBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <QShortcut>
#include <QTableWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace EMStudio
{
    // constructor
    AttachmentsWindow::AttachmentsWindow(QWidget* parent, bool deformable)
        : QWidget(parent)
    {
        mTableWidget            = nullptr;
        mActorInstance          = nullptr;
        mNodeSelectionWindow    = nullptr;
        mWaitingForAttachment   = false;
        mIsDeformableAttachment = deformable;
        mEscapeShortcut         = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    }


    // destructor
    AttachmentsWindow::~AttachmentsWindow()
    {
    }


    // init the geometry lod window
    void AttachmentsWindow::Init()
    {
        mTempString.reserve(16384);

        setObjectName("StackFrameOnlyBG");
        setAcceptDrops(true);

        // create the lod information table
        mTableWidget = new QTableWidget();

        // set the alternating row colors
        mTableWidget->setAlternatingRowColors(true);

        // set the table to row single selection
        mTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        mTableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

        // make the table items read only
        mTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // set the minimum size and the resizing policy
        mTableWidget->setMinimumHeight(125);
        mTableWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        // automatically adjust the size of the last entry to make it always fitting the table widget size
        QHeaderView* horizontalHeader = mTableWidget->horizontalHeader();
        horizontalHeader->setStretchLastSection(true);

        // disable the corner button between the row and column selection thingies
        mTableWidget->setCornerButtonEnabled(false);

        // enable the custom context menu for the motion table
        mTableWidget->setContextMenuPolicy(Qt::DefaultContextMenu);

        // set the column count
        mTableWidget->setColumnCount(6);

        // set header items for the table
        mTableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem("Vis"));
        mTableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem("ID"));
        mTableWidget->setHorizontalHeaderItem(2, new QTableWidgetItem("Name"));
        mTableWidget->setHorizontalHeaderItem(3, new QTableWidgetItem("IsSkin"));
        mTableWidget->setHorizontalHeaderItem(4, new QTableWidgetItem("Node"));
        mTableWidget->setHorizontalHeaderItem(5, new QTableWidgetItem("Nodes"));

        // set the horizontal header alignement
        horizontalHeader->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignLeft);

        // set the vertical header not visible
        QHeaderView* verticalHeader = mTableWidget->verticalHeader();
        verticalHeader->setVisible(false);

        // set the vis fast updates and IsSkin columns fixed
        horizontalHeader->setSectionResizeMode(0, QHeaderView::Fixed);
        horizontalHeader->setSectionResizeMode(3, QHeaderView::Fixed);
        horizontalHeader->setSectionResizeMode(5, QHeaderView::Fixed);

        // set the width of the other columns
        mTableWidget->setColumnWidth(0, 25);
        mTableWidget->setColumnWidth(1, 25);
        mTableWidget->setColumnWidth(2, 100);
        mTableWidget->setColumnWidth(3, 44);
        mTableWidget->setColumnWidth(4, 100);
        mTableWidget->setColumnWidth(5, 32);

        // create buttons for the attachments dialog
        mOpenAttachmentButton           = new QToolButton();
        mOpenDeformableAttachmentButton = new QToolButton();
        mRemoveButton                   = new QToolButton();
        mClearButton                    = new QToolButton();
        mCancelSelectionButton          = new QToolButton();

        EMStudioManager::MakeTransparentButton(mOpenAttachmentButton,              "Images/Icons/Open.svg",   "Open actor from file and add it as regular attachment");
        EMStudioManager::MakeTransparentButton(mOpenDeformableAttachmentButton,    "Images/Icons/Open.svg",   "Open actor from file and add it as skin attachment");
        EMStudioManager::MakeTransparentButton(mRemoveButton,                      "Images/Icons/Minus.svg",  "Remove selected attachments");
        EMStudioManager::MakeTransparentButton(mClearButton,                       "Images/Icons/Clear.svg",  "Remove all attachments");
        EMStudioManager::MakeTransparentButton(mCancelSelectionButton,             "Images/Icons/Remove.svg", "Cancel attachment selection");

        // create the buttons layout
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setAlignment(Qt::AlignLeft);
        buttonLayout->addWidget(mOpenAttachmentButton);
        buttonLayout->addWidget(mOpenDeformableAttachmentButton);
        buttonLayout->addWidget(mRemoveButton);
        buttonLayout->addWidget(mClearButton);

        // create the buttons layout for selection mode
        QHBoxLayout* buttonLayoutSelectionMode = new QHBoxLayout();
        buttonLayoutSelectionMode->setSpacing(0);
        buttonLayoutSelectionMode->setAlignment(Qt::AlignLeft);
        buttonLayoutSelectionMode->addWidget(mCancelSelectionButton);

        // create info widgets
        mWaitingForAttachmentWidget = new QWidget();
        mNoSelectionWidget          = new QWidget();
        mWaitingForAttachmentLayout = new QVBoxLayout();
        mNoSelectionLayout          = new QVBoxLayout();
        QLabel* waitingForAttachmentLabel = new QLabel("Please select an actor instance.");
        QLabel* noSelectionLabel = new QLabel("No attachments to show.");

        mWaitingForAttachmentLayout->addLayout(buttonLayoutSelectionMode);
        mWaitingForAttachmentLayout->addWidget(waitingForAttachmentLabel);
        mWaitingForAttachmentLayout->setAlignment(waitingForAttachmentLabel, Qt::AlignCenter);
        mWaitingForAttachmentWidget->setLayout(mWaitingForAttachmentLayout);
        mWaitingForAttachmentWidget->setHidden(true);

        mNoSelectionLayout->addWidget(noSelectionLabel);
        mNoSelectionLayout->setAlignment(noSelectionLabel, Qt::AlignCenter);
        mNoSelectionWidget->setLayout(mNoSelectionLayout);
        mNoSelectionWidget->setHidden(true);

        // create the layouts
        mAttachmentsWidget  = new QWidget();
        mAttachmentsLayout  = new QVBoxLayout();
        mMainLayout         = new QVBoxLayout();
        mMainLayout->setMargin(0);
        mMainLayout->setSpacing(2);
        mAttachmentsLayout->setMargin(0);
        mAttachmentsLayout->setSpacing(2);

        // fill the attachments layout
        mAttachmentsLayout->addLayout(buttonLayout);
        mAttachmentsLayout->addWidget(mTableWidget);
        mAttachmentsWidget->setLayout(mAttachmentsLayout);
        mAttachmentsWidget->setObjectName("StackFrameOnlyBG");

        // settings for the selection mode widgets
        mWaitingForAttachmentWidget->setObjectName("StackFrameOnlyBG");
        mWaitingForAttachmentWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        mWaitingForAttachmentLayout->setSpacing(0);
        mWaitingForAttachmentLayout->setMargin(0);
        mNoSelectionWidget->setObjectName("StackFrameOnlyBG");
        mNoSelectionWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        // fill the main layout
        mMainLayout->addWidget(mAttachmentsWidget);
        mMainLayout->addWidget(mWaitingForAttachmentWidget);
        mMainLayout->addWidget(mNoSelectionWidget);
        setLayout(mMainLayout);

        // create the node selection window
        mNodeSelectionWindow = new NodeSelectionWindow(this, true);

        // connect the controls to the slots
        connect(mTableWidget,                          &QTableWidget::itemSelectionChanged, this, &AttachmentsWindow::OnSelectionChanged);
        connect(mOpenAttachmentButton,                 &QToolButton::clicked,              this, &AttachmentsWindow::OnOpenAttachmentButtonClicked);
        connect(mOpenDeformableAttachmentButton,       &QToolButton::clicked,              this, &AttachmentsWindow::OnOpenDeformableAttachmentButtonClicked);
        connect(mRemoveButton,                         &QToolButton::clicked,              this, &AttachmentsWindow::OnRemoveButtonClicked);
        connect(mClearButton,                          &QToolButton::clicked,              this, &AttachmentsWindow::OnClearButtonClicked);
        connect(mNodeSelectionWindow->GetNodeHierarchyWidget(),                        static_cast<void (NodeHierarchyWidget::*)(MCore::Array<SelectionItem>)>(&NodeHierarchyWidget::OnSelectionDone), this, &AttachmentsWindow::OnAttachmentNodesSelected);
        connect(mNodeSelectionWindow,                                                  &NodeSelectionWindow::rejected,             this, &AttachmentsWindow::OnCancelAttachmentNodeSelection);
        connect(mNodeSelectionWindow->GetNodeHierarchyWidget()->GetTreeWidget(),       &QTreeWidget::itemSelectionChanged, this, &AttachmentsWindow::OnNodeChanged);
        connect(mEscapeShortcut, &QShortcut::activated, this, &AttachmentsWindow::OnEscapeButtonPressed);
        connect(mCancelSelectionButton, &QToolButton::clicked, this, &AttachmentsWindow::OnEscapeButtonPressed);

        // reinit the window
        ReInit();
    }


    // reinit the geometry lod window
    void AttachmentsWindow::ReInit()
    {
        // get the selected actor instance
        const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        mActorInstance = selection.GetSingleActorInstance();

        // disable controls if no actor instance is selected
        if (mActorInstance == nullptr)
        {
            // set the row count
            mTableWidget->setRowCount(0);

            // update the interface
            UpdateInterface();

            // stop here
            return;
        }

        // the number of existing attachments
        const uint32 numAttachments = mActorInstance->GetNumAttachments();

        // set table size and add header items
        mTableWidget->setRowCount(numAttachments);

        // loop trough all attachments and add them to the table
        for (uint32 i = 0; i < numAttachments; ++i)
        {
            EMotionFX::Attachment* attachment = mActorInstance->GetAttachment(i);
            if (attachment == nullptr)
            {
                continue;
            }

            EMotionFX::ActorInstance*   attachmentInstance  = attachment->GetAttachmentActorInstance();
            EMotionFX::Actor*           attachmentActor     = attachmentInstance->GetActor();
            EMotionFX::Actor*           attachedToActor     = mActorInstance->GetActor();
            uint32                      attachedToNodeIndex = MCORE_INVALIDINDEX32;
            EMotionFX::Node*            attachedToNode      = nullptr;

            if (!attachment->GetIsInfluencedByMultipleJoints())
            {
                attachedToNodeIndex = static_cast<EMotionFX::AttachmentNode*>(attachment)->GetAttachToNodeIndex();
            }

            if (attachedToNodeIndex != MCORE_INVALIDINDEX32)
            {
                attachedToNode = attachedToActor->GetSkeleton()->GetNode(attachedToNodeIndex);
            }

            // create table items
            mTempString = AZStd::string::format("%i", attachmentInstance->GetID());
            QTableWidgetItem* tableItemID           = new QTableWidgetItem(mTempString.c_str());
            AzFramework::StringFunc::Path::GetFileName(attachmentActor->GetFileNameString().c_str(), mTempString);
            QTableWidgetItem* tableItemName         = new QTableWidgetItem(mTempString.c_str());
            mTempString = attachment->GetIsInfluencedByMultipleJoints() ? "Yes" : "No";
            QTableWidgetItem* tableItemDeformable   = new QTableWidgetItem(mTempString.c_str());
            mTempString = AZStd::string::format("%i", attachmentInstance->GetNumNodes());
            QTableWidgetItem* tableItemNumNodes     = new QTableWidgetItem(mTempString.c_str());
            QTableWidgetItem* tableItemNodeName     = new QTableWidgetItem("");
            // set node name if exists
            if (attachedToNode)
            {
                tableItemNodeName->setWhatsThis(attachedToNode->GetName());

                auto nodeSelectionButton = new AzQtComponents::BrowseEdit();
                nodeSelectionButton->setPlaceholderText(attachedToNode->GetName());
                nodeSelectionButton->setStyleSheet("text-align: left;");
                mTableWidget->setCellWidget(i, 4, nodeSelectionButton);
                connect(nodeSelectionButton, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &AttachmentsWindow::OnSelectNodeButtonClicked);
            }

            // create the checkboxes
            QCheckBox* isVisibleCheckBox = new QCheckBox();
            isVisibleCheckBox->setStyleSheet("background: transparent; padding-left: 6px;");
            isVisibleCheckBox->setProperty("attachmentInstanceID", attachmentInstance->GetID());
            isVisibleCheckBox->setChecked(true);

            // add table items to the current row
            mTableWidget->setCellWidget(i, 0, isVisibleCheckBox);
            mTableWidget->setItem(i, 1, tableItemID);
            mTableWidget->setItem(i, 2, tableItemName);
            mTableWidget->setItem(i, 3, tableItemDeformable);
            mTableWidget->setItem(i, 4, tableItemNodeName);
            mTableWidget->setItem(i, 5, tableItemNumNodes);

            // connect the controls to the functions
            connect(isVisibleCheckBox,         &QCheckBox::stateChanged, this, &AttachmentsWindow::OnVisibilityChanged);

            // set the row height
            mTableWidget->setRowHeight(i, 21);
        }

        // update the interface
        UpdateInterface();
    }


    // update the enabled state of the remove/clear button depending on the table entries
    void AttachmentsWindow::OnUpdateButtonsEnabled()
    {
        mRemoveButton->setEnabled(mTableWidget->selectedItems().size() != 0);
        mClearButton->setEnabled(mTableWidget->rowCount() != 0);
    }


    // updates the whole interface
    void AttachmentsWindow::UpdateInterface()
    {
        // enable/disable widgets, based on the selection state
        mAttachmentsWidget->setHidden(mWaitingForAttachment);
        mWaitingForAttachmentWidget->setHidden((mWaitingForAttachment == false));

        // update remove/clear buttons
        OnUpdateButtonsEnabled();
    }


    // when dropping stuff in our window
    void AttachmentsWindow::dropEvent(QDropEvent* event)
    {
        // check if we dropped any files to the application
        const QMimeData* mimeData = event->mimeData();
        if (mimeData->hasUrls())
        {
            // read out the dropped file names
            const QList<QUrl> urls = mimeData->urls();

            // clear the drop filenames
            mDropFileNames.clear();
            mDropFileNames.reserve(urls.count());

            // get the number of urls and iterate over them
            AZStd::string filename;
            AZStd::string extension;
            const int numUrls = urls.count();
            for (int i = 0; i < numUrls; ++i)
            {
                // get the complete file name and extract the extension
                filename = urls[i].toLocalFile().toUtf8().data();
                AzFramework::StringFunc::Path::GetExtension(filename.c_str(), extension, false /* include dot */);

                if (extension == "actor")
                {
                    mDropFileNames.push_back(filename);
                }
            }

            // get the number of dropped sound files
            if (mDropFileNames.empty())
            {
                MCore::LogWarning("Drag and drop failed. No valid actor file dropped.");
            }
            else
            {
                // create the import context menu
                QMenu menu(this);

                QAction* attachmentAction   = menu.addAction("Open Regular Attachment");
                QAction* deformableAction   = menu.addAction("Open Skin Attachment");
                menu.addSeparator();
                /*QAction* cancelAction     =*/menu.addAction("Cancel");

                connect(attachmentAction, &QAction::triggered, this, &AttachmentsWindow::OnDroppedAttachmentsActors);
                connect(deformableAction, &QAction::triggered, this, &AttachmentsWindow::OnDroppedDeformableActors);

                // show the menu at the given position
                menu.exec(mapToGlobal(event->pos()));
            }
        }

        event->acceptProposedAction();
    }


    // drag enter
    void AttachmentsWindow::dragEnterEvent(QDragEnterEvent* event)
    {
        // this is needed to actually reach the drop event function
        event->acceptProposedAction();
    }


    // add an attachment
    void AttachmentsWindow::AddAttachment(const AZStd::string& filename)
    {
        AZStd::vector<AZStd::string> filenames;
        filenames.push_back(filename);
        AddAttachments(filenames);
    }


    // add attachments
    void AttachmentsWindow::AddAttachments(const AZStd::vector<AZStd::string>& filenames)
    {
        // create our command group
        AZStd::string outString;
        MCore::CommandGroup commandGroup("Add attachments");

        // skip adding if no actor instance is selected
        if (mActorInstance == nullptr)
        {
            return;
        }

        // get name of the first node
        EMotionFX::Actor* actor = mActorInstance->GetActor();
        if (actor == nullptr)
        {
            return;
        }

        MCORE_ASSERT(actor->GetNumNodes() > 0);
        const char* nodeName = actor->GetSkeleton()->GetNode(0)->GetName();

        // loop trough all filenames and add the attachments
        for (AZStd::string filename : filenames)
        {
            EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

            const uint32 actorIndex = EMotionFX::GetActorManager().FindActorIndexByFileName(filename.c_str());

            // create instance for the attachment
            if (actorIndex == MCORE_INVALIDINDEX32)
            {
                commandGroup.AddCommandString(AZStd::string::format("ImportActor -filename \"%s\"", filename.c_str()).c_str());
                commandGroup.AddCommandString("CreateActorInstance -actorID %LASTRESULT%");
            }
            else
            {
                EMotionFX::Actor* attachmentActor = EMotionFX::GetActorManager().GetActor(actorIndex);
                uint32 attachmentActorID = attachmentActor->GetID();
                commandGroup.AddCommandString(AZStd::string::format("CreateActorInstance -actorID %i", attachmentActorID).c_str());
            }

            // add the attachment
            if (mIsDeformableAttachment == false)
            {
                commandGroup.AddCommandString(AZStd::string::format("AddAttachment -attachmentID %%LASTRESULT%% -attachToID %i -attachToNode \"%s\"", mActorInstance->GetID(), nodeName).c_str());
            }
            else
            {
                commandGroup.AddCommandString(AZStd::string::format("AddDeformableAttachment -attachmentID %%LASTRESULT%% -attachToID %i", mActorInstance->GetID()).c_str());
            }
        }

        // select the old actorinstance
        commandGroup.AddCommandString("Unselect -actorInstanceID SELECT_ALL -actorID SELECT_ALL");
        commandGroup.AddCommandString(AZStd::string::format("Select -actorinstanceID %i", mActorInstance->GetID()).c_str());

        // execute the command group
        GetCommandManager()->ExecuteCommandGroup(commandGroup, outString);
    }


    // removes a row from a table
    void AttachmentsWindow::RemoveTableItems(const QList<QTableWidgetItem*>& items)
    {
        // import and start playing the animation
        AZStd::string outResult;
        MCore::CommandGroup group(AZStd::string("Remove Attachment Actor").c_str());

        // iterate trough all selected items
        const uint32 numItems = items.length();
        for (uint32 i = 0; i < numItems; ++i)
        {
            QTableWidgetItem* item = items[i];
            if (item == nullptr || item->column() != 1)
            {
                continue;
            }

            // the attachment id
            const uint32 id                 = GetIDFromTableRow(item->row());
            const AZStd::string nodeName    = GetNodeNameFromTableRow(item->row());

            group.AddCommandString(AZStd::string::format("RemoveAttachment -attachmentID %i -attachToID %i -attachToNode \"%s\"", id, mActorInstance->GetID(), nodeName.c_str()).c_str());
        }

        // execute the group command
        GetCommandManager()->ExecuteCommandGroup(group, outResult);

        // Reinit the table
        ReInit();
    }


    // called if an actor has been dropped for normal attachments
    void AttachmentsWindow::OnDroppedAttachmentsActors()
    {
        mIsDeformableAttachment = false;

        // add attachments to the selected actorinstance
        AddAttachments(mDropFileNames);

        // clear the attachments array
        mDropFileNames.clear();
    }


    // called if an actor has been dropped for deformable attachments
    void AttachmentsWindow::OnDroppedDeformableActors()
    {
        mIsDeformableAttachment = true;

        // add attachments to the selected actorinstance
        AddAttachments(mDropFileNames);

        // clear the attachments array
        mDropFileNames.clear();
    }


    // connects two selected actor instances
    void AttachmentsWindow::OnAttachmentSelected()
    {
        if (mWaitingForAttachment == false)
        {
            return;
        }

        // get the selected actor instance
        const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        EMotionFX::ActorInstance* attachmentInstance = selection.GetSingleActorInstance();

        if (mActorInstance == nullptr || attachmentInstance == nullptr)
        {
            return;
        }

        // get name of the first node
        EMotionFX::Actor* actor = mActorInstance->GetActor();
        if (actor == nullptr)
        {
            return;
        }

        assert(actor->GetNumNodes() > 0);
        const char* nodeName = actor->GetSkeleton()->GetNode(0)->GetName();

        // remove the attachment in case it is already attached
        mActorInstance->RemoveAttachment(attachmentInstance);

        // execute command for the attachment
        AZStd::string outResult;
        MCore::CommandGroup commandGroup("Add Attachment");

        // add the attachment
        if (mIsDeformableAttachment == false)
        {
            commandGroup.AddCommandString(AZStd::string::format("AddAttachment -attachToID %i -attachmentID %i -attachToNode \"%s\"", mActorInstance->GetID(), attachmentInstance->GetID(), nodeName).c_str());
        }
        else
        {
            commandGroup.AddCommandString(AZStd::string::format("AddDeformableAttachment -attachToID %i -attachmentID %i", mActorInstance->GetID(), attachmentInstance->GetID()).c_str());
        }

        // clear selection and select the actor instance the attachment is attached to
        commandGroup.AddCommandString(AZStd::string::format("ClearSelection"));
        commandGroup.AddCommandString(AZStd::string::format("Select -actorInstanceID %i", mActorInstance->GetID()));

        // reset the state for selection
        mWaitingForAttachment = false;

        // execute the command group
        EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult);

        // reinit the dialog as we added another attachment
        ReInit();
    }


    void AttachmentsWindow::OnNodeChanged()
    {
        NodeHierarchyWidget* hierarchyWidget = mNodeSelectionWindow->GetNodeHierarchyWidget();
        AZStd::vector<SelectionItem>& selectedItems = hierarchyWidget->GetSelectedItems();
        if (selectedItems.size() != 1)
        {
            return;
        }

        const uint32                actorInstanceID = selectedItems[0].mActorInstanceID;
        const char*                 nodeName        = selectedItems[0].GetNodeName();
        EMotionFX::ActorInstance*   actorInstance   = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            return;
        }

        assert(actorInstance == mActorInstance);

        EMotionFX::Actor*   actor   = actorInstance->GetActor();
        EMotionFX::Node*    node    = actor->GetSkeleton()->FindNodeByName(nodeName);
        if (node == nullptr)
        {
            return;
        }

        // get the attachment
        EMotionFX::ActorInstance* attachment = GetSelectedAttachment();
        if (attachment == nullptr)
        {
            return;
        }

        // reapply the attachment
        mActorInstance->RemoveAttachment(attachment);

        // create and add the new attachment
        EMotionFX::AttachmentNode* newAttachment = EMotionFX::AttachmentNode::Create(mActorInstance, node->GetNodeIndex(), attachment);
        mActorInstance->AddAttachment(newAttachment);
    }


    EMotionFX::ActorInstance* AttachmentsWindow::GetSelectedAttachment()
    {
        // get the attachment id
        const QList<QTableWidgetItem*> selectedTableItems = mTableWidget->selectedItems();
        if (selectedTableItems.length() < 1)
        {
            return nullptr;
        }

        const uint32 attachmentID = GetIDFromTableRow(selectedTableItems[0]->row());
        if (attachmentID == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        // get the attachment
        EMotionFX::ActorInstance* attachment = EMotionFX::GetActorManager().FindActorInstanceByID(attachmentID);
        return attachment;
    }


    void AttachmentsWindow::OnCancelAttachmentNodeSelection()
    {
        // get the attachment
        EMotionFX::ActorInstance* attachment = GetSelectedAttachment();
        if (attachment == nullptr)
        {
            return;
        }

        mActorInstance->RemoveAttachment(attachment);

        EMotionFX::Actor* actor = mActorInstance->GetActor();
        EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(mNodeBeforeSelectionWindow.c_str());

        if (node)
        {
            EMotionFX::AttachmentNode* newAttachment = EMotionFX::AttachmentNode::Create(mActorInstance, node->GetNodeIndex(), attachment);
            mActorInstance->AddAttachment(newAttachment);
            //mActorInstance->AddAttachment(node->GetNodeIndex(), attachment);
        }
    }

    // open a new attachment
    void AttachmentsWindow::OnOpenAttachmentButtonClicked()
    {
        // set to normal attachment
        mIsDeformableAttachment = false;

        AZStd::vector<AZStd::string> filenames = GetMainWindow()->GetFileManager()->LoadActorsFileDialog(this);
        if (filenames.empty())
        {
            return;
        }

        AddAttachments(filenames);
    }


    // open a new skin attachment
    void AttachmentsWindow::OnOpenDeformableAttachmentButtonClicked()
    {
        // set to skin attachment
        mIsDeformableAttachment = true;

        AZStd::vector<AZStd::string> filenames = GetMainWindow()->GetFileManager()->LoadActorsFileDialog(this);
        if (filenames.empty())
        {
            return;
        }

        AddAttachments(filenames);
    }


    // remove selected attachments
    void AttachmentsWindow::OnRemoveButtonClicked()
    {
        uint32 lowestSelectedRow = MCORE_INVALIDINDEX32;
        const QList<QTableWidgetItem*> selectedItems = mTableWidget->selectedItems();
        const int numSelectedItems = selectedItems.size();
        for (int i = 0; i < numSelectedItems; ++i)
        {
            if ((uint32)selectedItems[i]->row() < lowestSelectedRow)
            {
                lowestSelectedRow = (uint32)selectedItems[i]->row();
            }
        }

        RemoveTableItems(selectedItems);

        if (lowestSelectedRow > ((uint32)mTableWidget->rowCount() - 1))
        {
            mTableWidget->selectRow(lowestSelectedRow - 1);
        }
        else
        {
            mTableWidget->selectRow(lowestSelectedRow);
        }
    }


    // remove all attachments
    void AttachmentsWindow::OnClearButtonClicked()
    {
        mTableWidget->selectAll();
        RemoveTableItems(mTableWidget->selectedItems());
    }


    // open the node selection dialog for the node
    void AttachmentsWindow::OnSelectNodeButtonClicked()
    {
        // get the sender widget
        QLabel* widget = (QLabel*)(QWidget*)sender();
        if (widget == nullptr)
        {
            return;
        }

        // select the clicked row
        const int row = GetRowContainingWidget(widget);
        if (row != -1)
        {
            mTableWidget->selectRow(row);
        }

        mNodeBeforeSelectionWindow = GetSelectedNodeName();

        // show the node selection window
        mNodeSelectionWindow->Update(mActorInstance->GetID());
        mNodeSelectionWindow->show();
    }


    // called when the node selection is done
    void AttachmentsWindow::OnAttachmentNodesSelected(MCore::Array<SelectionItem> selection)
    {
        // check if selection is valid
        if (selection.GetLength() != 1)
        {
            MCore::LogDebug("No valid attachment selected.");
            return;
        }

        const uint32 actorInstanceID    = selection[0].mActorInstanceID;
        const char* nodeName            = selection[0].GetNodeName();
        if (EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID) == nullptr)
        {
            return;
        }

        // create command group
        AZStd::string outResult;
        MCore::CommandGroup commandGroup("Adjust attachment node");

        // get the attachment
        EMotionFX::ActorInstance* attachment = GetSelectedAttachment();
        if (attachment == nullptr)
        {
            return;
        }

        AZStd::string oldNodeName = GetSelectedNodeName();

        // remove and readd the attachment
        commandGroup.AddCommandString(AZStd::string::format("RemoveAttachment -attachmentID %i -attachToID %i -attachToNode \"%s\"", attachment->GetID(), mActorInstance->GetID(), oldNodeName.c_str()).c_str());
        commandGroup.AddCommandString(AZStd::string::format("AddAttachment -attachToID %i -attachmentID %i -attachToNode \"%s\"", mActorInstance->GetID(), attachment->GetID(), nodeName).c_str());

        // execute the command group
        GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult);
    }


    // get the selected node name
    AZStd::string AttachmentsWindow::GetSelectedNodeName()
    {
        const QList<QTableWidgetItem*> items = mTableWidget->selectedItems();
        const uint32 numItems = items.length();
        if (numItems < 1)
        {
            return AZStd::string();
        }

        return GetNodeNameFromTableRow(items[0]->row());
    }


    // called if the visibility of an actor instance has changed
    void AttachmentsWindow::OnVisibilityChanged(int visibility)
    {
        MCORE_UNUSED(visibility);

        // get the sender widget
        QCheckBox* widget = (QCheckBox*)(QWidget*)sender();
        if (widget == nullptr)
        {
            return;
        }

        // get the id from the checkbox
        const int id = widget->property("attachmentInstanceID").toInt();

        // execute visible command
        AZStd::string outResult;
        GetCommandManager()->ExecuteCommand(AZStd::string::format("AdjustActorInstance -actorInstanceID %d -doRender %s", id, AZStd::to_string(widget->isChecked()).c_str()), outResult);
    }


    // extracts the actor instance id from a given row
    int AttachmentsWindow::GetIDFromTableRow(int row)
    {
        QTableWidgetItem* item = mTableWidget->item(row, 1);
        if (item == nullptr)
        {
            return MCORE_INVALIDINDEX32;
        }

        AZStd::string id;
        FromQtString(item->text(), &id);

        return AzFramework::StringFunc::ToInt(id.c_str());
    }


    // extracts the node name from a given row
    AZStd::string AttachmentsWindow::GetNodeNameFromTableRow(int row)
    {
        QTableWidgetItem* item = mTableWidget->item(row, 4);
        if (item == nullptr)
        {
            return AZStd::string();
        }

        return FromQtString(item->whatsThis());
    }


    // get the row id containing the widget
    int AttachmentsWindow::GetRowContainingWidget(const QWidget* widget)
    {
        // loop trough the table items and search for widget
        const uint32 numRows = mTableWidget->rowCount();
        const uint32 numCols = mTableWidget->columnCount();
        for (uint32 i = 0; i < numRows; ++i)
        {
            for (uint32 j = 0; j < numCols; ++j)
            {
                if (mTableWidget->cellWidget(i, j) == widget)
                {
                    return i;
                }
            }
        }

        return -1;
    }


    // called when the table selection gets changed
    void AttachmentsWindow::OnSelectionChanged()
    {
        UpdateInterface();
    }


    // cancel selection of escape button is pressed
    void AttachmentsWindow::OnEscapeButtonPressed()
    {
        mWaitingForAttachment = false;
        UpdateInterface();
    }


    // key press event
    void AttachmentsWindow::keyPressEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            OnRemoveButtonClicked();
            event->accept();
            return;
        }

        // base class
        QWidget::keyPressEvent(event);
    }


    // key release event
    void AttachmentsWindow::keyReleaseEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            event->accept();
            return;
        }

        // base class
        QWidget::keyReleaseEvent(event);
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/Attachments/moc_AttachmentsWindow.cpp>
