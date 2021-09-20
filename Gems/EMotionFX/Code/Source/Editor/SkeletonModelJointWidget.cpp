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
        , m_contentsWidget(nullptr)
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

        // Contents widget
        m_contentsWidget = new QWidget();
        m_contentsWidget->setVisible(false);
        QVBoxLayout* contentsLayout = new QVBoxLayout();
        contentsLayout->setSpacing(ColliderContainerWidget::s_layoutSpacing);

        // Joint name
        QHBoxLayout* jointNameLayout = new QHBoxLayout();
        jointNameLayout->setAlignment(Qt::AlignLeft);
        jointNameLayout->setMargin(0);
        jointNameLayout->setSpacing(0);
        contentsLayout->addLayout(jointNameLayout);

        jointNameLayout->addSpacerItem(new QSpacerItem(s_jointLabelSpacing, 0, QSizePolicy::Fixed));
        QLabel* tempLabel = new QLabel("Joint name");
        tempLabel->setStyleSheet("font-weight: bold;");
        jointNameLayout->addWidget(tempLabel);

        jointNameLayout->addSpacerItem(new QSpacerItem(s_jointNameSpacing, 0, QSizePolicy::Fixed));
        m_jointNameLabel = new QLabel();
        jointNameLayout->addWidget(m_jointNameLabel);
        jointNameLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Ignored));

        contentsLayout->addWidget(CreateContentWidget(m_contentsWidget));

        m_contentsWidget->setLayout(contentsLayout);
        mainLayout->addWidget(m_contentsWidget);

        // No selection widget
        m_noSelectionWidget = CreateNoSelectionWidget(m_contentsWidget);
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
            if (!selectedModelIndices.isEmpty())
            {
                if (selectedModelIndices.size() == 1)
                {
                    m_jointNameLabel->setText(GetNode()->GetName());
                }
                else
                {
                    m_jointNameLabel->setText(QString("%1 joints selected").arg(selectedModelIndices.size()));
                }

                m_noSelectionWidget->hide();
                InternalReinit();
                m_contentsWidget->show();
            }
            else
            {
                m_contentsWidget->hide();
                InternalReinit();
                m_noSelectionWidget->show();
            }
        }
        else
        {
            m_contentsWidget->hide();
            InternalReinit();
            m_noSelectionWidget->hide();
        }
    }

    void SkeletonModelJointWidget::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
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
