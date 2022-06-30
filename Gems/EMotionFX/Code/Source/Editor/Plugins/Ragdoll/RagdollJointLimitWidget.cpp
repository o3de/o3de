/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/RagdollCommands.h>
#include <EMotionFX/CommandSystem/Source/JointLimitCommands.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulatorBus.h>
#include <Editor/Plugins/Ragdoll/RagdollJointLimitWidget.h>
#include <Editor/SkeletonModel.h>
#include <Editor/ObjectEditor.h>
#include <QComboBox>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QMenu>


namespace EMotionFX
{
    int RagdollJointLimitWidget::s_leftMargin = 13;
    int RagdollJointLimitWidget::s_textColumnWidth = 142;

    void RagdollJointLimitPropertyNotify::AfterPropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        PhysicsSetupManipulatorRequestBus::Broadcast(&PhysicsSetupManipulatorRequests::OnUnderlyingPropertiesChanged);
    }

    RagdollJointLimitWidget::RagdollJointLimitWidget(const AZStd::string& copiedJointLimits, QWidget* parent)
        : AzQtComponents::Card(parent)
        , m_cardHeaderIcon(SkeletonModel::s_ragdollJointLimitIconPath)
        , m_copiedJointLimits(copiedJointLimits)
        , m_propertyNotify(AZStd::make_unique<RagdollJointLimitPropertyNotify>())
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Error("EMotionFX", serializeContext, "Can't get serialize context from component application.");

        setTitle("Joint limit");

        AzQtComponents::CardHeader* cardHeader = header();
        cardHeader->setIcon(m_cardHeaderIcon);
        connect(this, &RagdollJointLimitWidget::contextMenuRequested, this, &RagdollJointLimitWidget::OnCardContextMenu);

        QVBoxLayout* vLayout  = new QVBoxLayout();
        vLayout->setAlignment(Qt::AlignTop);
        vLayout->setMargin(0);

        QWidget* innerWidget = new QWidget(this);
        innerWidget->setLayout(vLayout);

        QGridLayout* topLayout = new QGridLayout();
        topLayout->setMargin(2);
        topLayout->setAlignment(Qt::AlignLeft);

        // Has joint limit
        QWidget* spacerWidget = new QWidget(this);
        spacerWidget->setFixedWidth(s_leftMargin);
        topLayout->addWidget(spacerWidget, 0, 0, Qt::AlignLeft);

        QLabel* hasLimitLabel = new QLabel("Has joint limit");
        hasLimitLabel->setFixedWidth(s_textColumnWidth);
        topLayout->addWidget(hasLimitLabel, 0, 1, Qt::AlignLeft);

        m_hasLimitCheckbox = new QCheckBox("", this);
        connect(m_hasLimitCheckbox, &QCheckBox::stateChanged, this, &RagdollJointLimitWidget::OnHasLimitStateChanged);
        topLayout->addWidget(m_hasLimitCheckbox, 0, 2);

        // Joint limit type
        m_typeLabel = new QLabel("Limit type");
        topLayout->addWidget(m_typeLabel, 1, 1, Qt::AlignLeft);

        m_typeComboBox = new QComboBox(innerWidget);
        m_typeComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        topLayout->addWidget(m_typeComboBox, 1, 2);

        vLayout->addLayout(topLayout);

        if (serializeContext)
        {
            //D6 joint is the only currently supported joint for ragdoll
            if (auto* jointHelpers = AZ::Interface<AzPhysics::JointHelpersInterface>::Get())
            {
                if (AZStd::optional<const AZ::TypeId> d6jointTypeId = jointHelpers->GetSupportedJointTypeId(AzPhysics::JointType::D6Joint);
                    d6jointTypeId.has_value())
                {
                    const char* jointLimitName = serializeContext->FindClassData(*d6jointTypeId)->m_editData->m_name;
                    m_typeComboBox->addItem(jointLimitName, (*d6jointTypeId).ToString<AZStd::string>().c_str());
                }
            }

            // Reflected property editor for joint limit
            m_objectEditor = new EMotionFX::ObjectEditor(serializeContext, m_propertyNotify.get(), innerWidget);
            vLayout->addWidget(m_objectEditor);
        }

        connect(m_typeComboBox, qOverload<int>(&QComboBox::activated), this, &RagdollJointLimitWidget::OnJointTypeChanged);
        connect(m_typeComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &RagdollJointLimitWidget::OnJointTypeChanged);

        m_commandCallbacks.emplace_back(AZStd::make_unique<DataChangedCallback>(this, false));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandAdjustRagdollJoint::s_commandName, m_commandCallbacks.back().get());
        CommandSystem::GetCommandManager()->RegisterCommandCallback("AdjustJointLimit", m_commandCallbacks.back().get());

        setContentWidget(innerWidget);
        setExpanded(true);
    }

    RagdollJointLimitWidget::~RagdollJointLimitWidget()
    {
        for (const auto& callback : m_commandCallbacks)
        {
            CommandSystem::GetCommandManager()->RemoveCommandCallback(callback.get(), false);
        }
    }

    bool RagdollJointLimitWidget::HasJointLimit() const
    {
        return m_hasLimitCheckbox->isChecked();
    }

    void RagdollJointLimitWidget::Update(const QModelIndex& modelIndex)
    {
        m_nodeIndex = modelIndex;
        m_objectEditor->ClearInstances(false);

        Physics::RagdollNodeConfiguration* ragdollNodeConfig = GetRagdollNodeConfig();
        if (ragdollNodeConfig)
        {
            AzPhysics::JointConfiguration* jointLimitConfig = ragdollNodeConfig->m_jointConfig.get();
            if (jointLimitConfig)
            {
                const AZ::TypeId& jointTypeId = jointLimitConfig->RTTI_GetType();
                m_objectEditor->AddInstance(jointLimitConfig, jointTypeId);

                // Only show the type combo box in case there is more than one limit type to choose from.
                if (m_typeComboBox->count() > 1)
                {
                    m_typeLabel->show();
                    m_typeComboBox->show();
                }
                else
                {
                    m_typeLabel->hide();
                    m_typeComboBox->hide();
                }

                m_objectEditor->show();
            }
            else
            {
                // No joint limit
                m_typeLabel->hide();
                m_typeComboBox->hide();
                m_objectEditor->hide();
            }

            QSignalBlocker checkboxBlocker(m_hasLimitCheckbox);
            m_hasLimitCheckbox->setChecked(jointLimitConfig != nullptr);
        }
        else
        {
            m_typeLabel->hide();
            m_typeComboBox->hide();
            m_objectEditor->hide();
        }
    }

    void RagdollJointLimitWidget::InvalidateValues()
    {
        m_objectEditor->InvalidateValues();
    }

    Physics::RagdollNodeConfiguration* RagdollJointLimitWidget::GetRagdollNodeConfig() const
    {
        Actor* actor = m_nodeIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        Node* node = m_nodeIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
        if (!actor || !node)
        {
            return nullptr;
        }

        const AZStd::shared_ptr<EMotionFX::PhysicsSetup> physicsSetup = actor->GetPhysicsSetup();
        const Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();
        return ragdollConfig.FindNodeConfigByName(node->GetNameString());
    }

    void RagdollJointLimitWidget::OnHasLimitStateChanged(int state)
    {
        if (state == Qt::Checked)
        {
            ChangeLimitType(m_typeComboBox->currentIndex());
        }
        else
        {
            ChangeLimitType(AZ::TypeId::CreateNull());
        }
    }

    void RagdollJointLimitWidget::OnCardContextMenu(const QPoint& position)
    {
        QMenu* contextMenu = new QMenu(this);
        contextMenu->setObjectName("EMFX.RagdollJointLimitWidget.ContextMenu");

        QAction* copyAction = contextMenu->addAction("Copy joint limits");
        copyAction->setObjectName("EMFX.RagdollJointLimitWidget.CopyJointLimitsAction");
        connect(copyAction, &QAction::triggered, [this]()
        {
            Physics::RagdollNodeConfiguration* ragdollNodeConfig = GetRagdollNodeConfig();
            if (!ragdollNodeConfig)
            {
                return;
            }

            AZ::Outcome<AZStd::string> serialized = CommandAdjustRagdollJoint::SerializeJointLimits(ragdollNodeConfig);
            if (!serialized.IsSuccess())
            {
                return;
            }

            emit JointLimitCopied(serialized.GetValue());
        });

        QAction* pasteAction = contextMenu->addAction("Paste joint limits");
        pasteAction->setObjectName("EMFX.RagdollJointLimitWidget.PasteJointLimitsAction");
        connect(pasteAction, &QAction::triggered, [this]()
        {
            if (m_copiedJointLimits.empty())
            {
                return;
            }
            Physics::RagdollNodeConfiguration* ragdollNodeConfig = GetRagdollNodeConfig();
            if (!ragdollNodeConfig)
            {
                return;
            }

            Actor* actor = m_nodeIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
            Node* node = m_nodeIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

            auto adjustCommand = aznew CommandAdjustRagdollJoint(actor->GetID(), node->GetName(), m_copiedJointLimits);

            AZStd::string result;
            if (!CommandSystem::GetCommandManager()->ExecuteCommand(adjustCommand, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        });
        pasteAction->setEnabled(!m_copiedJointLimits.empty());

        connect(contextMenu, &QMenu::triggered, &QObject::deleteLater);

        contextMenu->popup(position);
    }

    void RagdollJointLimitWidget::ChangeLimitType(const AZ::TypeId& type)
    {
        Physics::RagdollNodeConfiguration* ragdollNodeConfig = GetRagdollNodeConfig();
        if (ragdollNodeConfig)
        {
            if (type.IsNull())
            {
                ragdollNodeConfig->m_jointConfig = nullptr;
            }
            else
            {
                const Node* node = m_nodeIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
                const Skeleton* skeleton = m_nodeIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>()->GetSkeleton();
                ragdollNodeConfig->m_jointConfig =
                    CommandRagdollHelpers::CreateJointLimitByType(AzPhysics::JointType::D6Joint, skeleton, node);
            }

            Update();
        }
        emit JointLimitTypeChanged();
    }

    void RagdollJointLimitWidget::ChangeLimitType(int supportedTypeIndex)
    {
        const QByteArray typeString = m_typeComboBox->itemData(supportedTypeIndex).toString().toUtf8();
        const AZ::TypeId newLimitType = AZ::TypeId::CreateString(typeString.data(), typeString.count());
        ChangeLimitType(newLimitType);
    }

    void RagdollJointLimitWidget::OnJointTypeChanged(int index)
    {
        ChangeLimitType(index);
    }

    bool RagdollJointLimitWidget::DataChangedCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(commandLine);
        if ((azrtti_typeid(command) == azrtti_typeid<CommandAdjustRagdollJoint>() ||
             azrtti_typeid(command) == azrtti_typeid<CommandAdjustJointLimit>()) &&
            m_widget->m_nodeIndex.isValid())
        {
            Node* node = m_widget->m_nodeIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
            const auto typedCommand = static_cast<CommandAdjustRagdollJoint*>(command);
            if (typedCommand->GetJointName() == node->GetName())
            {
                m_widget->m_objectEditor->InvalidateValues();
            }
        }
        return true;
    }

    bool RagdollJointLimitWidget::DataChangedCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        return Execute(command, commandLine);
    }

} // namespace EMotionFX
