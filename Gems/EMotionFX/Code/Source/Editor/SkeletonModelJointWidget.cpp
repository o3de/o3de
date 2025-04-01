/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/Event.h>
#include <AzQtComponents/Components/ToastNotification.h>
#include <AzQtComponents/Components/ToastNotificationConfiguration.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <UI/Notifications/ToastBus.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/SkeletonModel.h>
#include <Editor/SkeletonModelJointWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QVBoxLayout>
#include <QLabel>

namespace EMotionFX
{
    //! A Horizontal Line
    //! Gives some visual seperation of elements above and below
    struct HLineLayout : public QVBoxLayout
    {
        HLineLayout(QWidget* parent = nullptr)
        {
            setContentsMargins(0, 0, 0, 5);
            auto* frame = new QFrame(parent);
            frame->setFrameShape(QFrame::HLine);
            frame->setFrameShadow(QFrame::Sunken);
            addWidget(frame);
        }
    };


    int SkeletonModelJointWidget::s_jointLabelSpacing = 17;
    int SkeletonModelJointWidget::s_jointNameSpacing = 130;

    SkeletonModelJointWidget::SkeletonModelJointWidget(QWidget* parent)
        : QWidget(parent)
        , m_jointNameLabel(nullptr)
    {
    }

    void SkeletonModelJointWidget::CreateGUI()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setMargin(0);
        mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        auto* separatorLayout = new HLineLayout;
        auto* separatorLayoutWidget = new QWidget;
        separatorLayoutWidget->setLayout(separatorLayout);
        mainLayout->addWidget(separatorLayoutWidget);

        connect(this, &SkeletonModelJointWidget::WidgetCountChanged, this, [=]() {
            separatorLayoutWidget->setVisible(WidgetCount() > 0);
        });

        // Contents Card
        m_contentCard = new AzQtComponents::Card{this};
        AzQtComponents::Card::applyContainerStyle(m_contentCard);
        m_contentCard->setTitle(GetCardTitle());
        m_contentCard->header()->setHasContextMenu(false);
        m_contentCard->header()->setUnderlineColor(GetColor());

        m_content = new QWidget{this};
        m_content->setLayout(new QVBoxLayout);
        m_content->layout()->addWidget(m_contentCard);
        m_contentCard->setContentWidget(CreateContentWidget(m_contentCard));

        mainLayout->addWidget(m_content);
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

        m_content->hide();
        InternalReinit();
        if (GetActor())
        {
            bool onlyRootSelected = selectedModelIndices.size() == 1 && SkeletonModel::IndicesContainRootNode(selectedModelIndices);
            if (!selectedModelIndices.isEmpty() && !onlyRootSelected)
            {
                InternalReinit();

                if (m_collidersWidget != nullptr && m_collidersWidget->HasVisibleColliders())
                {
                    m_content->show();
                }
            }
        }
    }

    void SkeletonModelJointWidget::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        Reinit();
    }

    void SkeletonModelJointWidget::SetFilterString(QString filterString)
    {
        if (m_collidersWidget)
        {
            m_collidersWidget->SetFilterString(filterString);
        }
        Reinit();
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
    void SkeletonModelJointWidget::ErrorNotification(QString title, QString description)
    {
        QTimer::singleShot(
            0,
            this,
            [title, description]
            {
                AzToolsFramework::ToastRequestBus::Event(
                    AZ_CRC_CE("SkeletonOutliner"),
                    &AzToolsFramework::ToastRequestBus::Events::ShowToastNotification,
                    AzQtComponents::ToastConfiguration{
                        AzQtComponents::ToastType::Error,
                        title, description});
            });
    }

} // namespace EMotionFX
