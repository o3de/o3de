/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardNotification.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/SimulatedObjectCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/ObjectEditor.h>
#include <Editor/NotificationWidget.h>
#include <Editor/Plugins/SimulatedObject/SimulatedJointWidget.h>
#include <Editor/Plugins/ColliderWidgets/SimulatedObjectColliderWidget.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h>
#include <Editor/SimulatedObjectHelpers.h>
#include <Editor/SkeletonModel.h>
#include <MCore/Source/StringConversions.h>
#include <MysticQt/Source/MysticQtManager.h>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMessageBox>
#include <QModelIndex>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>


namespace EMotionFX
{
    class SimulatedObjectPropertyNotify
        : public AzToolsFramework::IPropertyEditorNotify
    {
    public:
        // this function gets called each time you are about to actually modify
        // a property (not when the editor opens)
        void BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;

        // this function gets called each time a property is actually modified
        // (not just when the editor appears), for each and every change - so
        // for example, as a slider moves.  its meant for undo state capture.
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*pNode*/) override {}

        // this funciton is called when some stateful operation begins, such as
        // dragging starts in the world editor or such in which case you don't
        // want to blow away the tree and rebuild it until editing is complete
        // since doing so is flickery and intensive.
        void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*pNode*/) override {}
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;

        // this will cause the current undo operation to complete, sealing it
        // and beginning a new one if there are further edits.
        void SealUndoStack() override {}
    private:
        MCore::CommandGroup m_commandGroup;
    };

    void SimulatedObjectPropertyNotify::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        if (!m_commandGroup.IsEmpty())
        {
            return;
        }

        const AzToolsFramework::InstanceDataNode* parent = pNode->GetParent();
        if (parent && parent->GetSerializeContext()->CanDowncast(parent->GetClassMetadata()->m_typeId, azrtti_typeid<EMotionFX::SimulatedObject>(), parent->GetClassMetadata()->m_azRtti, nullptr))
        {
            const size_t instanceCount = pNode->GetNumInstances();
            m_commandGroup.SetGroupName(AZStd::string::format("Adjust simulated object%s", instanceCount > 1 ? "s" : ""));
            for (size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
            {
                const SimulatedObject* simulatedObject = static_cast<SimulatedObject*>(parent->GetInstance(instanceIndex));
                const SimulatedObjectSetup* simulatedObjectSetup = simulatedObject->GetSimulatedObjectSetup();
                const AZ::u32 actorId = simulatedObjectSetup->GetActor()->GetID();
                const size_t objectIndex = simulatedObjectSetup->FindSimulatedObjectIndex(simulatedObject).GetValue();

                CommandAdjustSimulatedObject* command = aznew CommandAdjustSimulatedObject(actorId, objectIndex);
                m_commandGroup.AddCommand(command);

                const void* instance = pNode->GetInstance(instanceIndex);

                const AZ::SerializeContext::ClassElement* elementData = pNode->GetElementMetadata();
                if (elementData->m_nameCrc == AZ_CRC_CE("objectName"))
                {
                    command->SetOldObjectName(*static_cast<const AZStd::string*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("gravityFactor"))
                {
                    command->SetOldGravityFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("stiffnessFactor"))
                {
                    command->SetOldStiffnessFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("dampingFactor"))
                {
                    command->SetOldDampingFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("colliderTags"))
                {
                    command->SetOldColliderTags(*static_cast<const AZStd::vector<AZStd::string>*>(instance));
                }
            }
        }
        else if (parent && parent->GetSerializeContext()->CanDowncast(parent->GetClassMetadata()->m_typeId, azrtti_typeid<EMotionFX::SimulatedJoint>(), parent->GetClassMetadata()->m_azRtti, nullptr))
        {
            const size_t instanceCount = pNode->GetNumInstances();
            m_commandGroup.SetGroupName(AZStd::string::format("Adjust simulated joint%s", instanceCount > 1 ? "s" : ""));
            for (size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
            {
                const SimulatedJoint* simulatedJoint = static_cast<SimulatedJoint*>(parent->GetInstance(instanceIndex));
                const SimulatedObject* simulatedObject = simulatedJoint->GetSimulatedObject();
                const SimulatedObjectSetup* simulatedObjectSetup = simulatedObject->GetSimulatedObjectSetup();
                const AZ::u32 actorId = simulatedObjectSetup->GetActor()->GetID();
                const size_t objectIndex = simulatedObjectSetup->FindSimulatedObjectIndex(simulatedObject).GetValue();
                const size_t jointIndex = simulatedJoint->CalculateSimulatedJointIndex().GetValue();

                CommandAdjustSimulatedJoint* command = aznew CommandAdjustSimulatedJoint(actorId, objectIndex, jointIndex);
                m_commandGroup.AddCommand(command);

                const void* instance = pNode->GetInstance(instanceIndex);

                const AZ::SerializeContext::ClassElement* elementData = pNode->GetElementMetadata();
                if (elementData->m_nameCrc == AZ_CRC_CE("coneAngleLimit"))
                {
                    command->SetOldConeAngleLimit(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("mass"))
                {
                    command->SetOldMass(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("stiffness"))
                {
                    command->SetOldStiffness(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("damping"))
                {
                    command->SetOldDamping(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("gravityFactor"))
                {
                    command->SetOldGravityFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("friction"))
                {
                    command->SetOldFriction(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("pinned"))
                {
                    command->SetOldPinned(*static_cast<const bool*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("colliderExclusionTags"))
                {
                    command->SetOldColliderExclusionTags(*static_cast<const AZStd::vector<AZStd::string>*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("autoExcludeMode"))
                {
                    command->SetOldAutoExcludeMode(*static_cast<const SimulatedJoint::AutoExcludeMode*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("autoExcludeGeometric"))
                {
                    command->SetOldGeometricAutoExclusion(*static_cast<const bool*>(instance));
                }
            }
        }
    }

    void SimulatedObjectPropertyNotify::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
    {
        const AzToolsFramework::InstanceDataNode* parent = pNode->GetParent();
        if (!m_commandGroup.IsEmpty() && parent && parent->GetSerializeContext()->CanDowncast(parent->GetClassMetadata()->m_typeId, azrtti_typeid<EMotionFX::SimulatedObject>(), parent->GetClassMetadata()->m_azRtti, nullptr))
        {
            const size_t instanceCount = pNode->GetNumInstances();
            for (size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
            {
                CommandAdjustSimulatedObject* command = static_cast<CommandAdjustSimulatedObject*>(m_commandGroup.GetCommand(instanceIndex));

                const void* instance = pNode->GetInstance(instanceIndex);

                const AZ::SerializeContext::ClassElement* elementData = pNode->GetElementMetadata();
                if (elementData->m_nameCrc == AZ_CRC_CE("objectName"))
                {
                    command->SetObjectName(*static_cast<const AZStd::string*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("gravityFactor"))
                {
                    command->SetGravityFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("stiffnessFactor"))
                {
                    command->SetStiffnessFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("dampingFactor"))
                {
                    command->SetDampingFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("colliderTags"))
                {
                    AZStd::string commandString;

                    const EMotionFX::SimulatedObject* simulatedObject = static_cast<const EMotionFX::SimulatedObject*>(parent->GetInstance(instanceIndex));
                    const AZStd::vector<AZStd::string>& colliderTags = simulatedObject->GetColliderTags();

                    AZStd::vector<AZStd::string> colliderExclusionTags;
                    const AZStd::vector<SimulatedJoint*>& simulatedJoints = simulatedObject->GetSimulatedJoints();
                    for (const SimulatedJoint* simulatedJoint : simulatedJoints)
                    {
                        // Copy the current exclusion tags to a temporary buffer.
                        colliderExclusionTags = simulatedJoint->GetColliderExclusionTags();

                        // Remove all tags that are no longer part of the collider tags of the simulated object.
                        bool changed = false;
                        colliderExclusionTags.erase(AZStd::remove_if(colliderExclusionTags.begin(), colliderExclusionTags.end(),
                            [&colliderTags, &changed](const AZStd::string& tag)->bool
                            {
                                if (AZStd::find(colliderTags.begin(), colliderTags.end(), tag) == colliderTags.end())
                                {
                                    // The exclusion tag is not part of the collider tags in the simulated object.
                                    // Remove the tag from the exclusion tags.
                                    changed = true;
                                    return true;
                                }

                                return false;
                            }),
                            colliderExclusionTags.end());

                        if (changed)
                        {
                            const SimulatedObjectSetup* simulatedObjectSetup = simulatedObject->GetSimulatedObjectSetup();
                            const AZ::u32 actorId = simulatedObjectSetup->GetActor()->GetID();
                            const size_t objectIndex = simulatedObjectSetup->FindSimulatedObjectIndex(simulatedObject).GetValue();
                            const size_t jointIndex = simulatedJoint->CalculateSimulatedJointIndex().GetValue();
                            const AZStd::string colliderExclusionTagString = MCore::ConstructStringSeparatedBySemicolons(colliderExclusionTags);

                            commandString = AZStd::string::format("%s -%s %d -%s %zu -%s %zu -%s \"%s\"",
                                CommandAdjustSimulatedJoint::s_commandName,
                                CommandAdjustSimulatedJoint::s_actorIdParameterName, actorId,
                                CommandAdjustSimulatedJoint::s_objectIndexParameterName, objectIndex,
                                CommandAdjustSimulatedJoint::s_jointIndexParameterName, jointIndex,
                                CommandAdjustSimulatedJoint::s_colliderExclusionTagsParameterName, colliderExclusionTagString.c_str());
                            m_commandGroup.AddCommandString(commandString);
                        }
                    }

                    command->SetColliderTags(*static_cast<const AZStd::vector<AZStd::string>*>(instance));
                }
            }
        }
        else if (!m_commandGroup.IsEmpty() && parent && parent->GetSerializeContext()->CanDowncast(parent->GetClassMetadata()->m_typeId, azrtti_typeid<EMotionFX::SimulatedJoint>(), parent->GetClassMetadata()->m_azRtti, nullptr))
        {
            const size_t instanceCount = pNode->GetNumInstances();
            for (size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
            {
                CommandAdjustSimulatedJoint* command = static_cast<CommandAdjustSimulatedJoint*>(m_commandGroup.GetCommand(instanceIndex));

                const void* instance = pNode->GetInstance(instanceIndex);

                const AZ::SerializeContext::ClassElement* elementData = pNode->GetElementMetadata();
                if (elementData->m_nameCrc == AZ_CRC_CE("coneAngleLimit"))
                {
                    command->SetConeAngleLimit(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("mass"))
                {
                    command->SetMass(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("stiffness"))
                {
                    command->SetStiffness(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("damping"))
                {
                    command->SetDamping(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("gravityFactor"))
                {
                    command->SetGravityFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("friction"))
                {
                    command->SetFriction(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("pinned"))
                {
                    command->SetPinned(*static_cast<const bool*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("colliderExclusionTags"))
                {
                    command->SetColliderExclusionTags(*static_cast<const AZStd::vector<AZStd::string>*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("autoExcludeMode"))
                {
                    command->SetAutoExcludeMode(*static_cast<const SimulatedJoint::AutoExcludeMode*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC_CE("autoExcludeGeometric"))
                {
                    command->SetGeometricAutoExclusion(*static_cast<const bool*>(instance));
                }
            }
        }

        AZStd::string result;
        CommandSystem::GetCommandManager()->ExecuteCommandGroup(m_commandGroup, result);
        m_commandGroup.Clear();
    }

    ///////////////////////////////////////////////////////////////////////////

    const int SimulatedJointWidget::s_jointLabelSpacing = 17;
    const int SimulatedJointWidget::s_jointNameSpacing = 90;

    SimulatedJointWidget::SimulatedJointWidget(SimulatedObjectWidget* plugin, QWidget* parent)
        : QScrollArea(parent)
        , m_plugin(plugin)
        , m_contentsWidget(new QWidget(this))
        , m_removeButton(new QPushButton("Remove from simulated object", this))
        , m_backButton(new QPushButton("Back to simulated object", this))
        , m_simulatedObjectEditorCard(new AzQtComponents::Card(this))
        , m_simulatedJointEditorCard(new AzQtComponents::Card(this))
        , m_nameLeftLabel(new QLabel(this))
        , m_nameRightLabel(new QLabel(this))
        , m_propertyNotify(AZStd::make_unique<SimulatedObjectPropertyNotify>())
    {
        connect(m_removeButton, &QPushButton::clicked, this, &SimulatedJointWidget::RemoveSelectedSimulatedJoint);
        connect(m_backButton, &QPushButton::clicked, this, &SimulatedJointWidget::BackToSimulatedObject);

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Error("EMotionFX", serializeContext, "Can't get serialize context from component application.");

        // Setup the object editor.
        {
            m_simulatedObjectEditor = new ObjectEditor(serializeContext, m_propertyNotify.get());
            QWidget* objectCardContents = new QWidget(this);
            QVBoxLayout* objectCardLayout = new QVBoxLayout(objectCardContents);
            objectCardLayout->addWidget(m_simulatedObjectEditor);
            m_simulatedObjectNotification1 = new NotificationWidget(m_simulatedObjectEditorCard, "To add a joint to this simulated object, right click a joint in the outliner, choose Add to Simulated Object, and select this object.");
            m_simulatedObjectNotification2 = new NotificationWidget(m_simulatedObjectEditorCard, "There are no simulated object colliders. To add a collider, right click a joint in the outliner, choose Add Collider, and select a primitive shape. Simulated objects will collide with the primitive shape.");
            objectCardLayout->addWidget(m_simulatedObjectNotification1);
            objectCardLayout->addWidget(m_simulatedObjectNotification2);
            m_simulatedObjectNotification1->hide();
            m_simulatedObjectNotification2->hide();
            m_simulatedObjectEditorCard->setContentWidget(objectCardContents);
        }

        // Setup the joint editor.
        {
            m_simulatedJointEditor = new ObjectEditor(serializeContext, m_propertyNotify.get());
            m_simulatedJointEditor->setObjectName("EMFX.SimulatedJointWidget.SimulatedJointEditor");

            NotificationWidget* notif = new NotificationWidget(m_simulatedJointEditorCard, "To have the selected joints to collider against other primitive shape, set up 'collide with' setting in their Simulated Object.");
            notif->addFeature(m_backButton);

            QWidget* jointCardContents = new QWidget(this);
            QVBoxLayout* jointCardLayout = new QVBoxLayout(jointCardContents);

            jointCardLayout->addWidget(m_simulatedJointEditor);
            jointCardLayout->addWidget(notif);

            m_simulatedJointEditorCard->setContentWidget(jointCardContents);
        }

        m_colliderWidget = new QWidget();
        QVBoxLayout* colliderWidgetLayout = new QVBoxLayout(m_colliderWidget);

        if (ColliderHelpers::AreCollidersReflected())
        {
            SimulatedObjectColliderWidget* colliderWidget = new SimulatedObjectColliderWidget();
            colliderWidget->setObjectName("EMFX.SimulatedJointWidget.SimulatedObjectColliderWidget");
            colliderWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
            colliderWidget->CreateGUI();

            colliderWidgetLayout->addWidget(colliderWidget);
        }
        else
        {
            QLabel* noColliders = new QLabel(
                "To adjust the properties of the Simulated Object Colliders, "
                "enable the PhysX gem via the Project Manager");

            colliderWidgetLayout->addWidget(noColliders);
        }

        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (skeletonModel)
        {
            connect(&skeletonModel->GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, &SimulatedJointWidget::OnSkeletonOutlinerSelectionChanged);
        }

        // Add the name label
        QWidget* nameWidget = new QWidget();
        QBoxLayout* nameLayout = new QHBoxLayout(nameWidget);
        m_nameLeftLabel->setStyleSheet("font-weight: bold;");
        nameLayout->addWidget(m_nameLeftLabel);
        nameLayout->addWidget(m_nameRightLabel);
        nameLayout->setStretchFactor(m_nameLeftLabel, 3);
        nameLayout->setStretchFactor(m_nameRightLabel, 2);

        // Contents widget
        m_contentsWidget->setVisible(false);
        QVBoxLayout* contentsLayout = new QVBoxLayout(m_contentsWidget);
        contentsLayout->setSpacing(ColliderContainerWidget::s_layoutSpacing);
        contentsLayout->addWidget(nameWidget);
        contentsLayout->addWidget(m_removeButton);
        contentsLayout->addWidget(m_simulatedObjectEditorCard);
        contentsLayout->addWidget(m_simulatedJointEditorCard);

        QWidget* scrolledWidget = new QWidget();
        QVBoxLayout* mainLayout = new QVBoxLayout(scrolledWidget);
        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->setMargin(0);
        mainLayout->addWidget(m_contentsWidget);
        mainLayout->addWidget(m_colliderWidget);

        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        setWidget(scrolledWidget);
        setWidgetResizable(true);

        SimulatedObjectModel* model = m_plugin->GetSimulatedObjectModel();
        connect(model->GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, &SimulatedJointWidget::UpdateDetailsView);
        connect(model, &QAbstractItemModel::dataChanged, this, &SimulatedJointWidget::UpdateObjectNotification);
        connect(model, &QAbstractItemModel::dataChanged, m_simulatedObjectEditor, &ObjectEditor::InvalidateValues);
        connect(model, &QAbstractItemModel::dataChanged, m_simulatedJointEditor, &ObjectEditor::InvalidateValues);
    }

    SimulatedJointWidget::~SimulatedJointWidget() = default;

    void SimulatedJointWidget::UpdateDetailsView(const QItemSelection& selected, const QItemSelection& deselected)
    {
        AZ_UNUSED(selected);
        AZ_UNUSED(deselected);

        const SimulatedObjectModel* model = m_plugin->GetSimulatedObjectModel();
        const QItemSelectionModel* selectionModel = model->GetSelectionModel();
        const QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

        if (selectedIndexes.empty())
        {
            m_contentsWidget->setVisible(false);
            m_simulatedObjectEditor->ClearInstances(true);
            m_simulatedJointEditor->ClearInstances(true);
            return;
        }

        m_simulatedObjectEditor->ClearInstances(false);
        m_simulatedJointEditor->ClearInstances(false);

        QString jointName;
        QString objectName;
        size_t numSelectedObjects = 0;
        size_t numSelectedJoints = 0;

        AZStd::unordered_map<AZ::Uuid, AZStd::vector<void*>> typeIdToAggregateInstance;
        for (const QModelIndex& modelIndex : selectedIndexes)
        {
            if (modelIndex.column() != 0)
            {
                continue;
            }
            void* object = modelIndex.data(SimulatedObjectModel::ROLE_JOINT_PTR).value<SimulatedJoint*>();
            AZ::Uuid typeId = azrtti_typeid<SimulatedJoint>();
            ObjectEditor* objectEditor = m_simulatedJointEditor;

            if (!object)
            {
                object = modelIndex.data(SimulatedObjectModel::ROLE_OBJECT_PTR).value<SimulatedObject*>();
                typeId = azrtti_typeid<SimulatedObject>();
                objectEditor = m_simulatedObjectEditor;
                if (objectName.isNull())
                {
                    objectName = modelIndex.data().toString();
                }
            }
            else if (jointName.isNull())
            {
                jointName = modelIndex.data().toString();
                objectName = modelIndex.data(SimulatedObjectModel::ROLE_OBJECT_NAME).value<QString>();
            }

            if (object)
            {
                auto foundAggregate = typeIdToAggregateInstance.find(typeId);
                if (foundAggregate != typeIdToAggregateInstance.end())
                {
                    objectEditor->AddInstance(object, typeId, foundAggregate->second[0]);
                    foundAggregate->second.emplace_back(object);
                }
                else
                {
                    objectEditor->AddInstance(object, typeId);
                    typeIdToAggregateInstance.emplace(typeId, AZStd::vector<void*> {object});
                }
            }
        }

        auto selectedSimulatedObjects = typeIdToAggregateInstance.find(azrtti_typeid<SimulatedObject>());
        if (selectedSimulatedObjects != typeIdToAggregateInstance.end())
        {
            m_simulatedObjectEditorCard->show();
            numSelectedObjects = selectedSimulatedObjects->second.size();
            if (numSelectedObjects == 1)
            {
                m_simulatedObjectEditorCard->setTitle("Simulated Object Settings");
            }
            else
            {
                m_simulatedObjectEditorCard->setTitle(QString("%1 Simulated Object%2").arg(numSelectedObjects).arg(numSelectedObjects > 1 ? "s" : ""));
            }
        }
        else
        {
            m_simulatedObjectEditorCard->hide();
        }

        auto selectedSimulatedJoints = typeIdToAggregateInstance.find(azrtti_typeid<SimulatedJoint>());
        if (selectedSimulatedJoints != typeIdToAggregateInstance.end())
        {
            m_simulatedJointEditorCard->show();
            numSelectedJoints = selectedSimulatedJoints->second.size();
            if (numSelectedJoints == 1)
            {
                m_simulatedJointEditorCard->setTitle("Simulated Joint Settings");
            }
            else
            {
                m_simulatedJointEditorCard->setTitle(QString("%1 Simulated Joint%2").arg(numSelectedJoints).arg(numSelectedJoints > 1 ? "s" : ""));
            }

            // We only want to show the button when only joints are selected.
            if (numSelectedObjects == 0)
            {
                m_backButton->show();
                m_backButton->setText(QString("Back to '%1'").arg(objectName));

                m_removeButton->show();
                m_removeButton->setText(QString("Remove from '%1'").arg(objectName));
            }
            else
            {
                m_backButton->hide();
                m_removeButton->hide();
            }
        }
        else
        {
            m_simulatedJointEditorCard->hide();
            m_backButton->hide();
            m_removeButton->hide();
        }

        // Update the name label
        {
            QString objectPlural = numSelectedObjects == 1 ? "" : "s";
            QString jointPlural = numSelectedJoints == 1 ? "" : "s";
            if (numSelectedObjects > 0 && numSelectedJoints > 0)
            {
                m_nameLeftLabel->setText("Multiple selected");
                m_nameRightLabel->setText(QString("%1 object%2, %3 joint%4 selected").arg(numSelectedObjects).arg(objectPlural).arg(numSelectedJoints).arg(jointPlural));
            }
            else if (numSelectedObjects > 0)
            {
                m_nameLeftLabel->setText("Object name");
                if (numSelectedObjects == 1)
                {
                    m_nameRightLabel->setText(objectName);
                }
                else
                {
                    m_nameRightLabel->setText(QString("%1 object%2 selected").arg(numSelectedObjects).arg(objectPlural));
                }
            }
            else if (numSelectedJoints > 0)
            {
                m_nameLeftLabel->setText("Joint name");
                if (numSelectedJoints == 1)
                {
                    m_nameRightLabel->setText(jointName);
                }
                else
                {
                    m_nameRightLabel->setText(QString("%1 joint%2 selected").arg(numSelectedJoints).arg(jointPlural));
                }
            }
        }

        UpdateObjectNotification();

        m_contentsWidget->setVisible(!selectedIndexes.empty());

        // Hide the collider widget as the joint in the Simulated Object widget
        // was the last thing selected
        m_colliderWidget->hide();
    }

    void SimulatedJointWidget::UpdateObjectNotification()
    {
        if (m_simulatedObjectEditorCard->isHidden())
        {
            return;
        }
        m_simulatedObjectNotification1->hide();
        m_simulatedObjectNotification2->hide();

        const SimulatedObjectModel* model = m_plugin->GetSimulatedObjectModel();
        const QModelIndexList selectedIndexes = model->GetSelectionModel()->selectedIndexes();

        if (selectedIndexes.size() != 1)
        {
            return;
        }

        // Add notification when a single object is selected.
        SimulatedObject* object = selectedIndexes[0].data(SimulatedObjectModel::ROLE_OBJECT_PTR).value<SimulatedObject*>();
        if (!object)
        {
            return;
        }

        if (object->GetNumSimulatedJoints() == 0)
        {
            m_simulatedObjectNotification1->show();
        }
        if (object->GetColliderTags().empty())
        {
            m_simulatedObjectNotification2->show();
        }
    }

    void SimulatedJointWidget::OnSkeletonOutlinerSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        AZ_UNUSED(deselected);

        if (!selected.isEmpty())
        {
            // Show the collider widget as the joint in the skeleton outliner is the last selected.
            m_contentsWidget->hide();
            m_colliderWidget->show();
        }
    }

    void SimulatedJointWidget::RemoveSelectedSimulatedJoint() const
    {
        const SimulatedObjectModel* model = m_plugin->GetSimulatedObjectModel();
        SimulatedObjectHelpers::RemoveSimulatedJoints(model->GetSelectionModel()->selectedRows(0), false);
    }

    void SimulatedJointWidget::BackToSimulatedObject()
    {
        SimulatedObjectModel* model = m_plugin->GetSimulatedObjectModel();
        QItemSelectionModel* selectionModel = model->GetSelectionModel();
        const QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

        if (selectedIndexes.empty())
        {
            return;
        }

        // Note: If multiple joints are selected and they are from different objects, select the first.
        const size_t objectIndex = selectedIndexes[0].data(SimulatedObjectModel::ROLE_OBJECT_INDEX).value<quint64>();
        const QModelIndex modelIndex = model->GetModelIndexByObjectIndex(objectIndex);
        selectionModel->select(modelIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
} // namespace EMotionFX
