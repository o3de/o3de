/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/ColliderContainerWidget.h>
#include <Editor/SkeletonModel.h>
#include <Editor/SkeletonModelJointWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include "AzQtComponents/Components/Widgets/CardHeader.h"
#include "EMotionFX/Source/ActorInstance.h"
#include <QLabel>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QVBoxLayout>


namespace EMotionFX
{
    int SkeletonModelJointWidget::s_jointLabelSpacing = 17;
    int SkeletonModelJointWidget::s_jointNameSpacing = 130;

    SkeletonModelJointWidget::SkeletonModelJointWidget(QWidget* parent)
        : QWidget(parent)
        , m_jointNameLabel(nullptr)
        , m_noSelectionWidget(nullptr)
    {
    }

    void SkeletonModelJointWidget::CreateGUI()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->setMargin(0);
        mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        mainLayout->setSizeConstraint(QLayout::SetMinimumSize);

        // Contents Card
        m_contentCard = new AzQtComponents::Card{this};
        AzQtComponents::Card::applyContainerStyle(m_contentCard);
        m_contentCard->setTitle(GetCardTitle());
        m_contentCard->header()->setHasContextMenu(false);
        m_contentCard->header()->setUnderlineColor(GetColor());
        //m_contentCard->setVisible(false);

        auto* content = new QWidget{m_contentCard};
        QVBoxLayout* contentsLayout = new QVBoxLayout();
        // contentsLayout->setSpacing(ColliderContainerWidget::s_layoutSpacing);
        m_contentCard->setLayout(contentsLayout);

        content->setLayout(contentsLayout);
        contentsLayout->addWidget(CreateContentWidget(m_contentCard));
        m_contentCard->setContentWidget(content);
        mainLayout->addWidget(m_contentCard);

        // No selection widget
        m_noSelectionWidget = CreateNoSelectionWidget(m_contentCard);
        mainLayout->addWidget(m_noSelectionWidget);

        setLayout(mainLayout);

        Reinit();

        // Connect to the model.
        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (skeletonModel)
        {
            connect(skeletonModel, &SkeletonModel::dataChanged, this, &SkeletonModelJointWidget::OnDataChanged);
            connect(skeletonModel, &SkeletonModel::modelReset, this, &SkeletonModelJointWidget::OnModelReset);
            connect(&skeletonModel->GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, &SkeletonModelJointWidget::OnSelectionChanged);
        }
    }

    void SkeletonModelJointWidget::Reinit()
    {
        const QModelIndexList& selectedModelIndices = GetSelectedModelIndices();

        if (!EMStudio::GetManager()->GetIgnoreVisibility() && !isVisible())
        {
            return;
        }

        if (GetActor())
        {
            bool onlyRootSelected = selectedModelIndices.size() == 1 && SkeletonModel::IndicesContainRootNode(selectedModelIndices);
            if (!selectedModelIndices.isEmpty() && !onlyRootSelected)
            {
                m_noSelectionWidget->hide();
                InternalReinit();
                m_contentCard->show();
            }
            else
            {
                m_contentCard->hide();
                InternalReinit();
                m_noSelectionWidget->show();
            }
        }
        else
        {
            m_contentCard->hide();
            InternalReinit();
            m_noSelectionWidget->hide();
        }
    }

    void SkeletonModelJointWidget::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        Reinit();
    }

    void SkeletonModelJointWidget::SetFilterString(AZStd::string str)
    {
        if (m_collidersWidget)
        {
            m_collidersWidget->SetFilterString(str);
        }
    }

    void SkeletonModelJointWidget::OnSelectionChanged([[maybe_unused]] const QItemSelection& selected, [[maybe_unused]] const QItemSelection& deselected)
    {
        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (skeletonModel)
        {
            const QModelIndexList selectedRows = skeletonModel->GetSelectionModel().selectedRows();
        }
        Reinit();
    }

    void SkeletonModelJointWidget::OnDataChanged([[maybe_unused]] const QModelIndex& topLeft, [[maybe_unused]] const QModelIndex& bottomRight, [[maybe_unused]] const QVector<int>& roles)
    {
        Reinit();
    }

    void SkeletonModelJointWidget::OnModelReset()
    {
        Reinit();
    }

    Actor* SkeletonModelJointWidget::GetActor() const
    {
        Actor* actor = nullptr;
        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (skeletonModel)
        {
            actor = skeletonModel->GetActor();
        }
        return actor;
    }

    ActorInstance* SkeletonModelJointWidget::GetActorInstance()
    {
        ActorInstance* selectedActorInstance = nullptr;
        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (skeletonModel)
        {
            selectedActorInstance = skeletonModel->GetActorInstance();
        }

        return selectedActorInstance;
    }

    Node* SkeletonModelJointWidget::GetNode() const
    {
        Node* node = nullptr;
        const QModelIndexList& selectedModelIndices = GetSelectedModelIndices();
        if (!selectedModelIndices.empty())
        {
            node = selectedModelIndices[0].data(SkeletonModel::ROLE_POINTER).value<Node*>();
        }
        return node;
    }

    QModelIndexList SkeletonModelJointWidget::GetSelectedModelIndices() const
    {
        QModelIndexList selectedModelIndices;
        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (skeletonModel)
        {
            selectedModelIndices = skeletonModel->GetSelectionModel().selectedRows();
        }

        return selectedModelIndices;
    }
} // namespace EMotionFX
