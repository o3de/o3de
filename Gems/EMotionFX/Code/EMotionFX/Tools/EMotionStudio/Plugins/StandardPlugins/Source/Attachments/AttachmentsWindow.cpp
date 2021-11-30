/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "AttachmentsWindow.h"
#include "AzCore/std/limits.h"
#include "MCore/Source/Config.h"
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
        m_tableWidget            = nullptr;
        m_actorInstance          = nullptr;
        m_nodeSelectionWindow    = nullptr;
        m_waitingForAttachment   = false;
        m_isDeformableAttachment = deformable;
        m_escapeShortcut         = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    }


    // destructor
    AttachmentsWindow::~AttachmentsWindow()
    {
    }


    // init the geometry lod window
    void AttachmentsWindow::Init()
    {
        m_tempString.reserve(16384);

        setObjectName("StackFrameOnlyBG");
        setAcceptDrops(true);

        // create the lod information table
        m_tableWidget = new QTableWidget();

        // set the alternating row colors
        m_tableWidget->setAlternatingRowColors(true);

        // set the table to row single selection
        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

        // make the table items read only
        m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // set the minimum size and the resizing policy
        m_tableWidget->setMinimumHeight(125);
        m_tableWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        // automatically adjust the size of the last entry to make it always fitting the table widget size
        QHeaderView* horizontalHeader = m_tableWidget->horizontalHeader();
        horizontalHeader->setStretchLastSection(true);

        // disable the corner button between the row and column selection thingies
        m_tableWidget->setCornerButtonEnabled(false);

        // enable the custom context menu for the motion table
        m_tableWidget->setContextMenuPolicy(Qt::DefaultContextMenu);

        // set the column count
        m_tableWidget->setColumnCount(6);

        // set header items for the table
        m_tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem("Vis"));
        m_tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem("ID"));
        m_tableWidget->setHorizontalHeaderItem(2, new QTableWidgetItem("Name"));
        m_tableWidget->setHorizontalHeaderItem(3, new QTableWidgetItem("IsSkin"));
        m_tableWidget->setHorizontalHeaderItem(4, new QTableWidgetItem("Node"));
        m_tableWidget->setHorizontalHeaderItem(5, new QTableWidgetItem("Nodes"));

        // set the horizontal header alignement
        horizontalHeader->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignLeft);

        // set the vertical header not visible
        QHeaderView* verticalHeader = m_tableWidget->verticalHeader();
        verticalHeader->setVisible(false);

        // set the vis fast updates and IsSkin columns fixed
        horizontalHeader->setSectionResizeMode(0, QHeaderView::Fixed);
        horizontalHeader->setSectionResizeMode(3, QHeaderView::Fixed);
        horizontalHeader->setSectionResizeMode(5, QHeaderView::Fixed);

        // set the width of the other columns
        m_tableWidget->setColumnWidth(0, 25);
        m_tableWidget->setColumnWidth(1, 25);
        m_tableWidget->setColumnWidth(2, 100);
        m_tableWidget->setColumnWidth(3, 44);
        m_tableWidget->setColumnWidth(4, 100);
        m_tableWidget->setColumnWidth(5, 32);

        // create buttons for the attachments dialog
        m_openAttachmentButton           = new QToolButton();
        m_openDeformableAttachmentButton = new QToolButton();
        m_removeButton                   = new QToolButton();
        m_clearButton                    = new QToolButton();
        m_cancelSelectionButton          = new QToolButton();

        EMStudioManager::MakeTransparentButton(m_openAttachmentButton,              "Images/Icons/Open.svg",   "Open actor from file and add it as regular attachment");
        EMStudioManager::MakeTransparentButton(m_openDeformableAttachmentButton,    "Images/Icons/Open.svg",   "Open actor from file and add it as skin attachment");
        EMStudioManager::MakeTransparentButton(m_removeButton,                      "Images/Icons/Minus.svg",  "Remove selected attachments");
        EMStudioManager::MakeTransparentButton(m_clearButton,                       "Images/Icons/Clear.svg",  "Remove all attachments");
        EMStudioManager::MakeTransparentButton(m_cancelSelectionButton,             "Images/Icons/Remove.svg", "Cancel attachment selection");

        // create the buttons layout
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setAlignment(Qt::AlignLeft);
        buttonLayout->addWidget(m_openAttachmentButton);
        buttonLayout->addWidget(m_openDeformableAttachmentButton);
        buttonLayout->addWidget(m_removeButton);
        buttonLayout->addWidget(m_clearButton);

        // create the buttons layout for selection mode
        QHBoxLayout* buttonLayoutSelectionMode = new QHBoxLayout();
        buttonLayoutSelectionMode->setSpacing(0);
        buttonLayoutSelectionMode->setAlignment(Qt::AlignLeft);
        buttonLayoutSelectionMode->addWidget(m_cancelSelectionButton);

        // create info widgets
        m_waitingForAttachmentWidget = new QWidget();
        m_noSelectionWidget          = new QWidget();
        m_waitingForAttachmentLayout = new QVBoxLayout();
        m_noSelectionLayout          = new QVBoxLayout();
        QLabel* waitingForAttachmentLabel = new QLabel("Please select an actor instance.");
        QLabel* noSelectionLabel = new QLabel("No attachments to show.");

        m_waitingForAttachmentLayout->addLayout(buttonLayoutSelectionMode);
        m_waitingForAttachmentLayout->addWidget(waitingForAttachmentLabel);
        m_waitingForAttachmentLayout->setAlignment(waitingForAttachmentLabel, Qt::AlignCenter);
        m_waitingForAttachmentWidget->setLayout(m_waitingForAttachmentLayout);
        m_waitingForAttachmentWidget->setHidden(true);

        m_noSelectionLayout->addWidget(noSelectionLabel);
        m_noSelectionLayout->setAlignment(noSelectionLabel, Qt::AlignCenter);
        m_noSelectionWidget->setLayout(m_noSelectionLayout);
        m_noSelectionWidget->setHidden(true);

        // create the layouts
        m_attachmentsWidget  = new QWidget();
        m_attachmentsLayout  = new QVBoxLayout();
        m_mainLayout         = new QVBoxLayout();
        m_mainLayout->setMargin(0);
        m_mainLayout->setSpacing(2);
        m_attachmentsLayout->setMargin(0);
        m_attachmentsLayout->setSpacing(2);

        // fill the attachments layout
        m_attachmentsLayout->addLayout(buttonLayout);
        m_attachmentsLayout->addWidget(m_tableWidget);
        m_attachmentsWidget->setLayout(m_attachmentsLayout);
        m_attachmentsWidget->setObjectName("StackFrameOnlyBG");

        // settings for the selection mode widgets
        m_waitingForAttachmentWidget->setObjectName("StackFrameOnlyBG");
        m_waitingForAttachmentWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        m_waitingForAttachmentLayout->setSpacing(0);
        m_waitingForAttachmentLayout->setMargin(0);
        m_noSelectionWidget->setObjectName("StackFrameOnlyBG");
        m_noSelectionWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        // fill the main layout
        m_mainLayout->addWidget(m_attachmentsWidget);
        m_mainLayout->addWidget(m_waitingForAttachmentWidget);
        m_mainLayout->addWidget(m_noSelectionWidget);
        setLayout(m_mainLayout);

        // create the node selection window
        m_nodeSelectionWindow = new NodeSelectionWindow(this, true);

        // connect the controls to the slots
        connect(m_tableWidget,                          &QTableWidget::itemSelectionChanged, this, &AttachmentsWindow::OnSelectionChanged);
        connect(m_openAttachmentButton,                 &QToolButton::clicked,              this, &AttachmentsWindow::OnOpenAttachmentButtonClicked);
        connect(m_openDeformableAttachmentButton,       &QToolButton::clicked,              this, &AttachmentsWindow::OnOpenDeformableAttachmentButtonClicked);
        connect(m_removeButton,                         &QToolButton::clicked,              this, &AttachmentsWindow::OnRemoveButtonClicked);
        connect(m_clearButton,                          &QToolButton::clicked,              this, &AttachmentsWindow::OnClearButtonClicked);
        connect(m_nodeSelectionWindow->GetNodeHierarchyWidget(),                        &NodeHierarchyWidget::OnSelectionDone, this, &AttachmentsWindow::OnAttachmentNodesSelected);
        connect(m_nodeSelectionWindow,                                                  &NodeSelectionWindow::rejected,             this, &AttachmentsWindow::OnCancelAttachmentNodeSelection);
        connect(m_nodeSelectionWindow->GetNodeHierarchyWidget()->GetTreeWidget(),       &QTreeWidget::itemSelectionChanged, this, &AttachmentsWindow::OnNodeChanged);
        connect(m_escapeShortcut, &QShortcut::activated, this, &AttachmentsWindow::OnEscapeButtonPressed);
        connect(m_cancelSelectionButton, &QToolButton::clicked, this, &AttachmentsWindow::OnEscapeButtonPressed);

        // reinit the window
        ReInit();
    }


    // reinit the geometry lod window
    void AttachmentsWindow::ReInit()
    {
        // get the selected actor instance
        const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        m_actorInstance = selection.GetSingleActorInstance();

        // disable controls if no actor instance is selected
        if (m_actorInstance == nullptr)
        {
            // set the row count
            m_tableWidget->setRowCount(0);

            // update the interface
            UpdateInterface();

            // stop here
            return;
        }

        // the number of existing attachments
        const int numAttachments = aznumeric_caster(m_actorInstance->GetNumAttachments());

        // set table size and add header items
        m_tableWidget->setRowCount(numAttachments);

        // loop trough all attachments and add them to the table
        for (int i = 0; i < numAttachments; ++i)
        {
            EMotionFX::Attachment* attachment = m_actorInstance->GetAttachment(i);
            if (attachment == nullptr)
            {
                continue;
            }

            EMotionFX::ActorInstance*   attachmentInstance  = attachment->GetAttachmentActorInstance();
            EMotionFX::Actor*           attachmentActor     = attachmentInstance->GetActor();
            EMotionFX::Actor*           attachedToActor     = m_actorInstance->GetActor();
            EMotionFX::Node*            attachedToNode      =
                !attachment->GetIsInfluencedByMultipleJoints()
                    ? attachedToNode = attachedToActor->GetSkeleton()->GetNode(
                         static_cast<EMotionFX::AttachmentNode*>(attachment)->GetAttachToNodeIndex())
                    : nullptr;

            // create table items
            m_tempString = AZStd::string::format("%i", attachmentInstance->GetID());
            QTableWidgetItem* tableItemID           = new QTableWidgetItem(m_tempString.c_str());
            AzFramework::StringFunc::Path::GetFileName(attachmentActor->GetFileNameString().c_str(), m_tempString);
            QTableWidgetItem* tableItemName         = new QTableWidgetItem(m_tempString.c_str());
            m_tempString = attachment->GetIsInfluencedByMultipleJoints() ? "Yes" : "No";
            QTableWidgetItem* tableItemDeformable   = new QTableWidgetItem(m_tempString.c_str());
            m_tempString = AZStd::string::format("%zu", attachmentInstance->GetNumNodes());
            QTableWidgetItem* tableItemNumNodes     = new QTableWidgetItem(m_tempString.c_str());
            QTableWidgetItem* tableItemNodeName     = new QTableWidgetItem("");
            // set node name if exists
            if (attachedToNode)
            {
                tableItemNodeName->setWhatsThis(attachedToNode->GetName());

                auto nodeSelectionButton = new AzQtComponents::BrowseEdit();
                nodeSelectionButton->setPlaceholderText(attachedToNode->GetName());
                nodeSelectionButton->setStyleSheet("text-align: left;");
                m_tableWidget->setCellWidget(i, 4, nodeSelectionButton);
                connect(nodeSelectionButton, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &AttachmentsWindow::OnSelectNodeButtonClicked);
            }

            // create the checkboxes
            QCheckBox* isVisibleCheckBox = new QCheckBox();
            isVisibleCheckBox->setStyleSheet("background: transparent; padding-left: 6px;");
            isVisibleCheckBox->setProperty("attachmentInstanceID", attachmentInstance->GetID());
            isVisibleCheckBox->setChecked(true);

            // add table items to the current row
            m_tableWidget->setCellWidget(i, 0, isVisibleCheckBox);
            m_tableWidget->setItem(i, 1, tableItemID);
            m_tableWidget->setItem(i, 2, tableItemName);
            m_tableWidget->setItem(i, 3, tableItemDeformable);
            m_tableWidget->setItem(i, 4, tableItemNodeName);
            m_tableWidget->setItem(i, 5, tableItemNumNodes);

            // connect the controls to the functions
            connect(isVisibleCheckBox,         &QCheckBox::stateChanged, this, &AttachmentsWindow::OnVisibilityChanged);

            // set the row height
            m_tableWidget->setRowHeight(i, 21);
        }

        // update the interface
        UpdateInterface();
    }


    // update the enabled state of the remove/clear button depending on the table entries
    void AttachmentsWindow::OnUpdateButtonsEnabled()
    {
        m_removeButton->setEnabled(m_tableWidget->selectedItems().size() != 0);
        m_clearButton->setEnabled(m_tableWidget->rowCount() != 0);
    }


    // updates the whole interface
    void AttachmentsWindow::UpdateInterface()
    {
        // enable/disable widgets, based on the selection state
        m_attachmentsWidget->setHidden(m_waitingForAttachment);
        m_waitingForAttachmentWidget->setHidden((m_waitingForAttachment == false));

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
            m_dropFileNames.clear();
            m_dropFileNames.reserve(urls.count());

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
                    m_dropFileNames.push_back(filename);
                }
            }

            // get the number of dropped sound files
            if (m_dropFileNames.empty())
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
        if (m_actorInstance == nullptr)
        {
            return;
        }

        // get name of the first node
        EMotionFX::Actor* actor = m_actorInstance->GetActor();
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

            const size_t actorIndex = EMotionFX::GetActorManager().FindActorIndexByFileName(filename.c_str());

            // create instance for the attachment
            if (actorIndex == InvalidIndex)
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
            if (m_isDeformableAttachment == false)
            {
                commandGroup.AddCommandString(AZStd::string::format("AddAttachment -attachmentID %%LASTRESULT%% -attachToID %i -attachToNode \"%s\"", m_actorInstance->GetID(), nodeName).c_str());
            }
            else
            {
                commandGroup.AddCommandString(AZStd::string::format("AddDeformableAttachment -attachmentID %%LASTRESULT%% -attachToID %i", m_actorInstance->GetID()).c_str());
            }
        }

        // select the old actorinstance
        commandGroup.AddCommandString("Unselect -actorInstanceID SELECT_ALL -actorID SELECT_ALL");
        commandGroup.AddCommandString(AZStd::string::format("Select -actorinstanceID %i", m_actorInstance->GetID()).c_str());

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
        for (const QTableWidgetItem* item : items)
        {
            if (item == nullptr || item->column() != 1)
            {
                continue;
            }

            // the attachment id
            const int id                    = GetIDFromTableRow(item->row());
            const AZStd::string nodeName    = GetNodeNameFromTableRow(item->row());

            group.AddCommandString(AZStd::string::format("RemoveAttachment -attachmentID %d -attachToID %i -attachToNode \"%s\"", id, m_actorInstance->GetID(), nodeName.c_str()).c_str());
        }

        // execute the group command
        GetCommandManager()->ExecuteCommandGroup(group, outResult);

        // Reinit the table
        ReInit();
    }


    // called if an actor has been dropped for normal attachments
    void AttachmentsWindow::OnDroppedAttachmentsActors()
    {
        m_isDeformableAttachment = false;

        // add attachments to the selected actorinstance
        AddAttachments(m_dropFileNames);

        // clear the attachments array
        m_dropFileNames.clear();
    }


    // called if an actor has been dropped for deformable attachments
    void AttachmentsWindow::OnDroppedDeformableActors()
    {
        m_isDeformableAttachment = true;

        // add attachments to the selected actorinstance
        AddAttachments(m_dropFileNames);

        // clear the attachments array
        m_dropFileNames.clear();
    }


    // connects two selected actor instances
    void AttachmentsWindow::OnAttachmentSelected()
    {
        if (m_waitingForAttachment == false)
        {
            return;
        }

        // get the selected actor instance
        const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        EMotionFX::ActorInstance* attachmentInstance = selection.GetSingleActorInstance();

        if (m_actorInstance == nullptr || attachmentInstance == nullptr)
        {
            return;
        }

        // get name of the first node
        EMotionFX::Actor* actor = m_actorInstance->GetActor();
        if (actor == nullptr)
        {
            return;
        }

        assert(actor->GetNumNodes() > 0);
        const char* nodeName = actor->GetSkeleton()->GetNode(0)->GetName();

        // remove the attachment in case it is already attached
        m_actorInstance->RemoveAttachment(attachmentInstance);

        // execute command for the attachment
        AZStd::string outResult;
        MCore::CommandGroup commandGroup("Add Attachment");

        // add the attachment
        if (m_isDeformableAttachment == false)
        {
            commandGroup.AddCommandString(AZStd::string::format("AddAttachment -attachToID %i -attachmentID %i -attachToNode \"%s\"", m_actorInstance->GetID(), attachmentInstance->GetID(), nodeName).c_str());
        }
        else
        {
            commandGroup.AddCommandString(AZStd::string::format("AddDeformableAttachment -attachToID %i -attachmentID %i", m_actorInstance->GetID(), attachmentInstance->GetID()).c_str());
        }

        // clear selection and select the actor instance the attachment is attached to
        commandGroup.AddCommandString(AZStd::string::format("ClearSelection"));
        commandGroup.AddCommandString(AZStd::string::format("Select -actorInstanceID %i", m_actorInstance->GetID()));

        // reset the state for selection
        m_waitingForAttachment = false;

        // execute the command group
        EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult);

        // reinit the dialog as we added another attachment
        ReInit();
    }


    void AttachmentsWindow::OnNodeChanged()
    {
        NodeHierarchyWidget* hierarchyWidget = m_nodeSelectionWindow->GetNodeHierarchyWidget();
        AZStd::vector<SelectionItem>& selectedItems = hierarchyWidget->GetSelectedItems();
        if (selectedItems.size() != 1)
        {
            return;
        }

        const uint32                actorInstanceID = selectedItems[0].m_actorInstanceId;
        const char*                 nodeName        = selectedItems[0].GetNodeName();
        EMotionFX::ActorInstance*   actorInstance   = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            return;
        }

        assert(actorInstance == m_actorInstance);

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
        m_actorInstance->RemoveAttachment(attachment);

        // create and add the new attachment
        EMotionFX::AttachmentNode* newAttachment = EMotionFX::AttachmentNode::Create(m_actorInstance, node->GetNodeIndex(), attachment);
        m_actorInstance->AddAttachment(newAttachment);
    }


    EMotionFX::ActorInstance* AttachmentsWindow::GetSelectedAttachment()
    {
        // get the attachment id
        const QList<QTableWidgetItem*> selectedTableItems = m_tableWidget->selectedItems();
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

        m_actorInstance->RemoveAttachment(attachment);

        EMotionFX::Actor* actor = m_actorInstance->GetActor();
        EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(m_nodeBeforeSelectionWindow.c_str());

        if (node)
        {
            EMotionFX::AttachmentNode* newAttachment = EMotionFX::AttachmentNode::Create(m_actorInstance, node->GetNodeIndex(), attachment);
            m_actorInstance->AddAttachment(newAttachment);
        }
    }

    // open a new attachment
    void AttachmentsWindow::OnOpenAttachmentButtonClicked()
    {
        // set to normal attachment
        m_isDeformableAttachment = false;

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
        m_isDeformableAttachment = true;

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
        int lowestSelectedRow = AZStd::numeric_limits<int>::max();
        const QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();
        for (const QTableWidgetItem* selectedItem : selectedItems)
        {
            if (selectedItem->row() < lowestSelectedRow)
            {
                lowestSelectedRow = selectedItem->row();
            }
        }

        RemoveTableItems(selectedItems);

        if (lowestSelectedRow > (m_tableWidget->rowCount() - 1))
        {
            m_tableWidget->selectRow(lowestSelectedRow - 1);
        }
        else
        {
            m_tableWidget->selectRow(lowestSelectedRow);
        }
    }


    // remove all attachments
    void AttachmentsWindow::OnClearButtonClicked()
    {
        m_tableWidget->selectAll();
        RemoveTableItems(m_tableWidget->selectedItems());
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
            m_tableWidget->selectRow(row);
        }

        m_nodeBeforeSelectionWindow = GetSelectedNodeName();

        // show the node selection window
        m_nodeSelectionWindow->Update(m_actorInstance->GetID());
        m_nodeSelectionWindow->show();
    }


    // called when the node selection is done
    void AttachmentsWindow::OnAttachmentNodesSelected(AZStd::vector<SelectionItem> selection)
    {
        // check if selection is valid
        if (selection.size() != 1)
        {
            MCore::LogDebug("No valid attachment selected.");
            return;
        }

        const uint32 actorInstanceID    = selection[0].m_actorInstanceId;
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
        commandGroup.AddCommandString(AZStd::string::format("RemoveAttachment -attachmentID %i -attachToID %i -attachToNode \"%s\"", attachment->GetID(), m_actorInstance->GetID(), oldNodeName.c_str()).c_str());
        commandGroup.AddCommandString(AZStd::string::format("AddAttachment -attachToID %i -attachmentID %i -attachToNode \"%s\"", m_actorInstance->GetID(), attachment->GetID(), nodeName).c_str());

        // execute the command group
        GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult);
    }


    // get the selected node name
    AZStd::string AttachmentsWindow::GetSelectedNodeName()
    {
        const QList<QTableWidgetItem*> items = m_tableWidget->selectedItems();
        const size_t numItems = items.length();
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
        QTableWidgetItem* item = m_tableWidget->item(row, 1);
        if (item == nullptr)
        {
            return MCore::InvalidIndexT<int>;
        }

        AZStd::string id;
        FromQtString(item->text(), &id);

        return AzFramework::StringFunc::ToInt(id.c_str());
    }


    // extracts the node name from a given row
    AZStd::string AttachmentsWindow::GetNodeNameFromTableRow(int row)
    {
        QTableWidgetItem* item = m_tableWidget->item(row, 4);
        if (item == nullptr)
        {
            return {};
        }

        return FromQtString(item->whatsThis());
    }


    // get the row id containing the widget
    int AttachmentsWindow::GetRowContainingWidget(const QWidget* widget)
    {
        // loop trough the table items and search for widget
        const int numRows = m_tableWidget->rowCount();
        const int numCols = m_tableWidget->columnCount();
        for (int i = 0; i < numRows; ++i)
        {
            for (int j = 0; j < numCols; ++j)
            {
                if (m_tableWidget->cellWidget(i, j) == widget)
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
        m_waitingForAttachment = false;
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
