/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Plugins/SkeletonOutliner/JointPropertyWidget.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeInfo.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/ActorInfo.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeGroupInfo.h>
#include <Editor/SkeletonModel.h>
#include <Editor/InspectorBus.h>
#include <UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <QItemSelectionModel>
#include <QBoxLayout>
#include <QLabel>

namespace EMotionFX
{
    JointPropertyWidget::JointPropertyWidget(QWidget* parent)
        :QWidget(parent)
    {
        auto* mainLayout = new QVBoxLayout;
        mainLayout->setMargin(0);
        mainLayout->setContentsMargins(0, 5, 0, 0);
        auto* propertyCard = new AzQtComponents::Card;
        AzQtComponents::Card::applySectionStyle(propertyCard);
        propertyCard->setTitle("Node Attributes");

        mainLayout->addWidget(propertyCard);

        // add the node attributes widget
        m_propertyWidget = aznew AzToolsFramework::ReflectedPropertyEditor(this);
        m_propertyWidget->setObjectName("EMFX.Joint.ReflectedPropertyEditor.PropertyWidget");

        propertyCard->setContentWidget(m_propertyWidget);

        setLayout(mainLayout);

        // Connect to the model.
        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (skeletonModel)
        {
            connect(skeletonModel, &SkeletonModel::dataChanged, this, &JointPropertyWidget::Reset);
            connect(skeletonModel, &SkeletonModel::modelReset, this, &JointPropertyWidget::Reset);
            connect(&skeletonModel->GetSelectionModel(), &QItemSelectionModel::selectionChanged,
                    this, &JointPropertyWidget::Reset);
        }

    }

    JointPropertyWidget::~JointPropertyWidget()
    {
        EMStudio::InspectorRequestBus::Broadcast(&EMStudio::InspectorRequestBus::Events::ClearIfShown, this);

        delete m_propertyWidget;
    }

    void JointPropertyWidget::Reset()
    {
        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (!skeletonModel)
        {
            return;
        }

        auto* actorInstance = skeletonModel->GetActorInstance();
        if (!actorInstance)
        {
            return;
        }

        m_propertyWidget->ClearInstances();
        m_propertyWidget->InvalidateAll();

        Node* node = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(node, &SkeletonOutlinerRequests::GetSingleSelectedNode);

        if (node && node->GetNodeIndex() != InvalidIndex)
        {
            m_nodeInfo.reset(aznew EMStudio::NodeInfo(actorInstance, node));
            m_propertyWidget->AddInstance(m_nodeInfo.get(), azrtti_typeid(m_nodeInfo.get()));
        }
        else if(actorInstance && actorInstance->GetActor())
        {
            m_actorInfo.reset(aznew EMStudio::ActorInfo(actorInstance));
            m_propertyWidget->AddInstance(m_actorInfo.get(), azrtti_typeid(m_actorInfo.get()));
        }
        else
        {
            return;
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return;
        }

        m_propertyWidget->Setup(serializeContext, nullptr, false);
        m_propertyWidget->show();
        m_propertyWidget->ExpandAll();
        m_propertyWidget->InvalidateAll();
    }
} // ns EMotionFX
