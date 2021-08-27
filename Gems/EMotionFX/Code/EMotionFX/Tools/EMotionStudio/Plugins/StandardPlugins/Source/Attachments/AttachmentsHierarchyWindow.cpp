/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "AttachmentsHierarchyWindow.h"
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/Attachment.h>
#include <EMotionFX/Source/ActorManager.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MCore/Source/StringConversions.h>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>


namespace EMStudio
{
    // constructor
    AttachmentsHierarchyWindow::AttachmentsHierarchyWindow(QWidget* parent)
        : QWidget(parent)
    {
        m_hierarchy = nullptr;
    }


    // destructor
    AttachmentsHierarchyWindow::~AttachmentsHierarchyWindow()
    {
    }


    void AttachmentsHierarchyWindow::Init()
    {
        QVBoxLayout* verticalLayout = new QVBoxLayout();
        verticalLayout->setMargin(0);
        setLayout(verticalLayout);

        m_hierarchy = new QTreeWidget();
        verticalLayout->addWidget(m_hierarchy);
        m_hierarchy->setColumnCount(1);
        m_hierarchy->setHeaderHidden(true);

        // set optical stuff for the tree
        m_hierarchy->setColumnWidth(0, 200);
        m_hierarchy->setColumnWidth(1, 20);
        m_hierarchy->setColumnWidth(1, 100);
        m_hierarchy->setSortingEnabled(false);
        m_hierarchy->setSelectionMode(QAbstractItemView::NoSelection);
        m_hierarchy->setMinimumWidth(150);
        m_hierarchy->setMinimumHeight(125);
        m_hierarchy->setAlternatingRowColors(true);
        m_hierarchy->setExpandsOnDoubleClick(true);
        m_hierarchy->setAnimated(true);

        // disable the move of section to have column order fixed
        m_hierarchy->header()->setSectionsMovable(false);

        verticalLayout->addWidget(m_hierarchy);

        ReInit();
    }


    void AttachmentsHierarchyWindow::ReInit()
    {
        // clear the tree
        m_hierarchy->clear();

        // get the number of actor instances and iterate through them
        const size_t numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance*   actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            EMotionFX::ActorInstance*   attachedTo      = actorInstance->GetAttachedTo();
            EMotionFX::Actor*           actor           = actorInstance->GetActor();

            // check if we are dealing with a root actor instance
            if (attachedTo == nullptr)
            {
                QTreeWidgetItem* item = new QTreeWidgetItem(m_hierarchy);
                AZStd::string actorFilename;
                AzFramework::StringFunc::Path::GetFileName(actor->GetFileNameString().c_str(), actorFilename);
                item->setText(0, QString("%1 (ID:%2)").arg(actorFilename.c_str()).arg(actorInstance->GetID()));
                item->setExpanded(true);
                m_hierarchy->addTopLevelItem(item);

                // get the number of attachments and iterate through them
                const size_t numAttachments = actorInstance->GetNumAttachments();
                for (size_t j = 0; j < numAttachments; ++j)
                {
                    EMotionFX::Attachment* attachment = actorInstance->GetAttachment(j);
                    MCORE_ASSERT(actorInstance == attachment->GetAttachToActorInstance());

                    // recursively go through all attachments
                    RecursivelyAddAttachments(item, attachment->GetAttachmentActorInstance());
                }
            }
        }
    }


    void AttachmentsHierarchyWindow::RecursivelyAddAttachments(QTreeWidgetItem* parent, EMotionFX::ActorInstance* actorInstance)
    {
        // get the actor from the actor instance
        EMotionFX::Actor* actor = actorInstance->GetActor();

        // add the current actor instance to the hierarchy
        QTreeWidgetItem* item = new QTreeWidgetItem(parent);
        AZStd::string actorFilename;
        AzFramework::StringFunc::Path::GetFileName(actor->GetFileNameString().c_str(), actorFilename);
        item->setText(0, QString("%1 (ID:%2)").arg(actorFilename.c_str()).arg(actorInstance->GetID()));
        item->setExpanded(true);
        parent->addChild(item);

        // get the number of attachments and iterate through them
        const size_t numAttachments = actorInstance->GetNumAttachments();
        for (size_t i = 0; i < numAttachments; ++i)
        {
            EMotionFX::Attachment* attachment = actorInstance->GetAttachment(i);
            MCORE_ASSERT(actorInstance == attachment->GetAttachToActorInstance());

            // recursively go through all attachments
            RecursivelyAddAttachments(item, attachment->GetAttachmentActorInstance());
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/Attachments/moc_AttachmentsHierarchyWindow.cpp>
