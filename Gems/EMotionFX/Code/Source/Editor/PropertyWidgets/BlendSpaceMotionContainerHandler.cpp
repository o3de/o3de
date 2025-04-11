/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/BlendSpaceManager.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MotionSetSelectionWindow.h>
#include <Editor/PropertyWidgets/BlendSpaceMotionContainerHandler.h>
#include <Editor/AnimGraphEditorBus.h>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendSpaceMotionWidget, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendSpaceMotionContainerWidget, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendSpaceMotionContainerHandler, EditorAllocator)

    BlendSpaceMotionWidget::BlendSpaceMotionWidget(BlendSpaceNode::BlendSpaceMotion* motion, QGridLayout* layout, int row)
        : m_motion(motion)
    {
        const AZStd::string& motionId = motion->GetMotionId();
        const bool showYFields = motion->GetDimension() == 2;

        int column = 0;

        // Motion name
        m_labelMotion = new QLabel(motionId.c_str());
        m_labelMotion->setObjectName("m_labelMotion");
        m_labelMotion->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        layout->addWidget(m_labelMotion, row, column);
        column++;

        const auto makeSpinbox = [row, &column, layout, motionId = motionId.c_str()](const QString& text, const QString& color)
        {
            auto* axisLayout = new QHBoxLayout();
            axisLayout->setAlignment(Qt::AlignRight);

            auto* axisLabel = new QLabel(text);
            axisLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            axisLabel->setStyleSheet(QString("QLabel { font-weight: bold; color : %1; }").arg(color));
            axisLayout->addWidget(axisLabel);

            auto* spinbox = new AzQtComponents::DoubleSpinBox();
            spinbox->setSingleStep(0.1);
            spinbox->setDecimals(4);
            spinbox->setRange(-FLT_MAX, FLT_MAX);
            spinbox->setProperty("motionId", motionId);
            spinbox->setKeyboardTracking(false);
            axisLayout->addWidget(spinbox);

            layout->addLayout(axisLayout, row, column);
            column++;

            return spinbox;
        };

        // Motion coordinate spinboxes.
        m_spinboxX = makeSpinbox("X", "red");

        if (showYFields)
        {
            m_spinboxY = makeSpinbox("Y", "green");
        }
        else
        {
            m_spinboxY = nullptr;
        }

        // Restore button.
        const int iconSize = 20;
        m_restoreButton = new QPushButton();
        m_restoreButton->setToolTip("Restore value to automatically computed one");
        m_restoreButton->setMinimumSize(iconSize, iconSize);
        m_restoreButton->setMaximumSize(iconSize, iconSize);
        m_restoreButton->setIcon(QIcon(":/EMotionFX/Restore.svg"));
        m_restoreButton->setProperty("motionId", motionId.c_str());
        layout->addWidget(m_restoreButton, row, column);
        column++;

        // Remove motion from blend space button.
        m_removeButton = new QPushButton();
        m_removeButton->setToolTip("Remove motion from blend space");
        m_removeButton->setMinimumSize(iconSize, iconSize);
        m_removeButton->setMaximumSize(iconSize, iconSize);
        m_removeButton->setIcon(QIcon(":/EMotionFX/Trash.svg"));
        layout->addWidget(m_removeButton, row, column);
    }


    void BlendSpaceMotionWidget::UpdateInterface(EMotionFX::BlendSpaceNode* blendSpaceNode, EMotionFX::AnimGraphInstance* animGraphInstance)
    {
        bool positionsComputed = false;
        AZ::Vector2 computedPosition = AZ::Vector2::CreateZero();
        if (blendSpaceNode && animGraphInstance)
        {
            blendSpaceNode->ComputeMotionCoordinates(m_motion->GetMotionId(), animGraphInstance, computedPosition);
            positionsComputed = true;
        }

        // Spinbox X
        m_spinboxX->blockSignals(true);

        if (m_motion->IsXCoordinateSetByUser())
        {
            m_spinboxX->setValue(m_motion->GetXCoordinate());
        }
        else
        {
            m_spinboxX->setValue(computedPosition.GetX());
        }

        m_spinboxX->blockSignals(false);
        m_spinboxX->setEnabled(m_motion->IsXCoordinateSetByUser() || positionsComputed);


        // Spinbox Y
        if (m_spinboxY)
        {
            m_spinboxY->blockSignals(true);

            if (m_motion->IsYCoordinateSetByUser())
            {
                m_spinboxY->setValue(m_motion->GetYCoordinate());
            }
            else
            {
                m_spinboxY->setValue(computedPosition.GetY());
            }

            m_spinboxY->blockSignals(false);
            m_spinboxY->setEnabled(m_motion->IsYCoordinateSetByUser() || positionsComputed);
        }

        // Enable the restore button in case the user manually set any of the.
        const bool enableRestoreButton = m_motion->IsXCoordinateSetByUser() || m_motion->IsYCoordinateSetByUser();
        m_restoreButton->setEnabled(enableRestoreButton);

        // is motion invalid?
        if (m_motion->TestFlag(EMotionFX::BlendSpaceNode::BlendSpaceMotion::TypeFlags::InvalidMotion))
        {
            m_labelMotion->setStyleSheet("#m_labelMotion { border: 1px solid red; }");
            m_labelMotion->setToolTip("Invalid motion.Select a motion set that contains this motion or add it to the current one.");
        }
        else
        {
            m_labelMotion->setStyleSheet("#m_labelMotion { border: none; }");
            m_labelMotion->setToolTip("");
        }
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    BlendSpaceMotionContainerWidget::BlendSpaceMotionContainerWidget([[maybe_unused]] BlendSpaceNode* blendSpaceNode, QWidget* parent)
        : QWidget(parent)
        , m_blendSpaceNode(nullptr)
        , m_containerWidget(nullptr)
        , m_addMotionsLabel(nullptr)
    {
        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setSpacing(0);
        mainLayout->setMargin(0);
        setLayout(mainLayout);
    }


    void BlendSpaceMotionContainerWidget::SetBlendSpaceNode(BlendSpaceNode* blendSpaceNode)
    {
        m_blendSpaceNode = blendSpaceNode;
        ReInit();
    }


    void BlendSpaceMotionContainerWidget::SetMotions(const AZStd::vector<BlendSpaceNode::BlendSpaceMotion>& motions)
    {
        m_motions = motions;
        ReInit();
    }


    const AZStd::vector<BlendSpaceNode::BlendSpaceMotion>& BlendSpaceMotionContainerWidget::GetMotions() const
    {
        return m_motions;
    }


    BlendSpaceMotionWidget* BlendSpaceMotionContainerWidget::FindWidgetByMotionId(const AZStd::string& motionId) const
    {
        for (BlendSpaceMotionWidget* container : m_motionWidgets)
        {
            const BlendSpaceNode::BlendSpaceMotion* motion = container->m_motion;
            if (motion->GetMotionId() == motionId)
            {
                return container;
            }
        }

        return nullptr;
    }


    BlendSpaceMotionWidget* BlendSpaceMotionContainerWidget::FindWidget(QObject* object)
    {
        const AZStd::string motionId = object->property("motionId").toString().toUtf8().data();

        BlendSpaceMotionWidget* widget = FindWidgetByMotionId(motionId);
        AZ_Assert(widget, "Can't find widget for motion with id '%s'.", motionId.c_str());

        return widget;
    }


    void BlendSpaceMotionContainerWidget::OnAddMotion()
    {
        EMotionFX::MotionSet* motionSet = nullptr;
        AnimGraphEditorRequestBus::BroadcastResult(motionSet, &AnimGraphEditorRequests::GetSelectedMotionSet);
        if (!motionSet)
        {
            QMessageBox::warning(this, "No Motion Set", "Cannot open motion selection window. Please make sure exactly one motion set is selected.");
            return;
        }

        // Create and show the motion picker window.
        EMStudio::MotionSetSelectionWindow motionPickWindow(this);
        motionPickWindow.GetHierarchyWidget()->SetSelectionMode(false);
        motionPickWindow.Update(motionSet);
        motionPickWindow.setModal(true);

        if (motionPickWindow.exec() == QDialog::Rejected)   // we pressed cancel or the close cross
        {
            return;
        }

        const AZStd::vector<AZStd::string> selectedMotionIds = motionPickWindow.GetHierarchyWidget()->GetSelectedMotionIds(motionSet);
        if (selectedMotionIds.empty())
        {
            return;
        }

        for (const AZStd::string& selectedMotionId : selectedMotionIds)
        {
            bool alreadyExists = false;

            for (const BlendSpaceNode::BlendSpaceMotion& blendSpaceMotion : m_motions)
            {
                if (blendSpaceMotion.GetMotionId() == selectedMotionId)
                {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists)
            {
                BlendSpaceNode::BlendSpaceMotion newMotion(selectedMotionId);
                m_motions.emplace_back(BlendSpaceNode::BlendSpaceMotion(selectedMotionId));
            }
        }

        m_blendSpaceNode->SetMotions(m_motions);
        m_motions = m_blendSpaceNode->GetMotions();

        ReInit();
        emit MotionsChanged();
    }


    void BlendSpaceMotionContainerWidget::OnRemoveMotion(const BlendSpaceNode::BlendSpaceMotion* motion)
    {
        // Iterate through the arributes back to front and delete the ones with the motion id from the delete button.
        // Note: Normally there should only be once instance as motion ids should be unique within this array.
        const AZ::s64 motionCount = m_motions.size();
        for (AZ::s64 i = motionCount - 1; i >= 0; i--)
        {
            if (&m_motions[i] == motion)
            {
                m_motions.erase(m_motions.begin() + i);
            }
        }

        ReInit();
        emit MotionsChanged();
    }


    void BlendSpaceMotionContainerWidget::OnPositionXChanged(double value)
    {
        UpdateMotionPosition(sender(), static_cast<float>(value), true, false);
    }


    void BlendSpaceMotionContainerWidget::OnPositionYChanged(double value)
    {
        UpdateMotionPosition(sender(), static_cast<float>(value), false, true);
    }


    // Get the currently active anim graph instance in case only exactly one actor instance is selected.
    EMotionFX::AnimGraphInstance* BlendSpaceMotionContainerWidget::GetSingleSelectedAnimGraphInstance() const
    {
        if (!m_blendSpaceNode)
        {
            return nullptr;
        }

        EMotionFX::ActorInstance* actorInstance = CommandSystem::GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (!actorInstance)
        {
            return nullptr;
        }

        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (animGraphInstance && animGraphInstance->GetAnimGraph() != m_blendSpaceNode->GetAnimGraph())
        {
            // The currently activated anim graph in the plugin differs from the one the current actor instance uses.
            animGraphInstance = nullptr;
        }

        return animGraphInstance;
    }


    void BlendSpaceMotionContainerWidget::UpdateMotionPosition(QObject* object, float value, bool updateX, bool updateY)
    {
        BlendSpaceMotionWidget* widget = FindWidget(object);
        if (!widget)
        {
            AZ_Error("EMotionFX", false, "Cannot update motion position. Can't find widget for QObject.");
            return;
        }

        BlendSpaceNode::BlendSpaceMotion* blendSpaceMotion = widget->m_motion;
        if (!blendSpaceMotion)
        {
            AZ_Error("EMotionFX", false, "Cannot update motion position. Blend space motion widget does not have a motion assigned to it.");
            return;
        }

        // Get the anim graph instance in case only exactly one actor instance is selected.
        EMotionFX::AnimGraphInstance* animGraphInstance = GetSingleSelectedAnimGraphInstance();
        if (animGraphInstance)
        {
            // Compute the position of the motion using the set evaluators.
            AZ::Vector2 computedPosition;
            m_blendSpaceNode->ComputeMotionCoordinates(blendSpaceMotion->GetMotionId(), animGraphInstance, computedPosition);

            const float epsilon = 1.0f / (powf(10, static_cast<float>(widget->m_spinboxX->decimals())));
            if (updateX)
            {
                if (blendSpaceMotion->IsXCoordinateSetByUser())
                {
                    // If we already manually set the motion position, just update the x coordinate.
                    blendSpaceMotion->SetXCoordinate(value);
                }
                else
                {
                    // Check if the user just clicked the interface and triggered a value change or if he actually changed the value.
                    if (!AZ::IsClose(computedPosition.GetX(), value, epsilon))
                    {
                        // Mark the position as manually set in case the user entered a new position that differs from the automatically computed one.
                        blendSpaceMotion->MarkXCoordinateSetByUser(true);
                        blendSpaceMotion->SetXCoordinate(value);
                    }
                }
            }

            if (updateY)
            {
                if (blendSpaceMotion->IsYCoordinateSetByUser())
                {
                    blendSpaceMotion->SetYCoordinate(value);
                }
                else
                {
                    if (!AZ::IsClose(computedPosition.GetY(), value, epsilon))
                    {
                        blendSpaceMotion->MarkYCoordinateSetByUser(true);
                        blendSpaceMotion->SetYCoordinate(value);
                    }
                }
            }
        }
        else
        {
            // In case there is no character, only the motion positions that are already in manual mode are enabled.
            // Thus, we can just forward the position shown in the interface to the attribute.

            if (updateX)
            {
                blendSpaceMotion->MarkXCoordinateSetByUser(true);
                blendSpaceMotion->SetXCoordinate(value);
            }
            if (updateY)
            {
                blendSpaceMotion->MarkYCoordinateSetByUser(true);
                blendSpaceMotion->SetYCoordinate(value);
            }
        }

        ReInit();
        emit MotionsChanged();
    }


    void BlendSpaceMotionContainerWidget::OnRestorePosition()
    {
        BlendSpaceMotionWidget* widget = FindWidget(sender());
        if (!widget)
        {
            AZ_Error("EMotionFX", false, "Cannot update motion position. Can't find widget for QObject.");
            return;
        }

        // Get the anim graph instance in case only exactly one actor instance is selected.
        EMotionFX::AnimGraphInstance* animGraphInstance = GetSingleSelectedAnimGraphInstance();

        // Get access to the blend space node of the anim graph to be able to calculate the blend space position.
        if (m_blendSpaceNode && animGraphInstance)
        {
            m_blendSpaceNode->RestoreMotionCoordinates(*widget->m_motion, animGraphInstance);

            ReInit();
            emit MotionsChanged();
        }
    }


    void BlendSpaceMotionContainerWidget::UpdateInterface()
    {
        // Get the anim graph instance in case only exactly one actor instance is selected.
        EMotionFX::AnimGraphInstance* animGraphInstance = GetSingleSelectedAnimGraphInstance();

        for (BlendSpaceMotionWidget* widget : m_motionWidgets)
        {
            widget->UpdateInterface(m_blendSpaceNode, animGraphInstance);
        }

        if (m_motions.empty())
        {
            m_addMotionsLabel->setText("Add motions and set coordinates.");
        }
        else
        {
            m_addMotionsLabel->setText("");
        }
    }


    void BlendSpaceMotionContainerWidget::ReInit()
    {
        if (m_containerWidget)
        {
            // Hide the old widget and request deletion.
            m_containerWidget->hide();
            m_containerWidget->deleteLater();

            m_containerWidget = nullptr;
            m_addMotionsLabel = nullptr;
            m_motionWidgets.clear();
        }

        m_containerWidget = new QWidget();
        m_containerWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

        QVBoxLayout* widgetLayout = new QVBoxLayout();
        QHBoxLayout* topRowLayout = new QHBoxLayout();

        // Add helper label left of the add button.
        m_addMotionsLabel = new QLabel();
        topRowLayout->addWidget(m_addMotionsLabel, 0, Qt::AlignLeft);

        // Add motions button.
        QPushButton* addMotionsButton = new QPushButton();
        EMStudio::EMStudioManager::MakeTransparentButton(addMotionsButton, "Images/Icons/Plus.svg", "Add motions to blend space");
        connect(addMotionsButton, &QPushButton::clicked, this, &BlendSpaceMotionContainerWidget::OnAddMotion);
        topRowLayout->addWidget(addMotionsButton, 0, Qt::AlignRight);

        widgetLayout->addLayout(topRowLayout);

        if (!m_motions.empty())
        {
            QWidget* motionsWidget = new QWidget(m_containerWidget);
            motionsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            QGridLayout* motionsLayout = new QGridLayout();
            motionsLayout->setMargin(0);

            const size_t motionCount = m_motions.size();
            for (size_t i = 0; i < motionCount; ++i)
            {
                BlendSpaceNode::BlendSpaceMotion* blendSpaceMotion = &m_motions[i];
                BlendSpaceMotionWidget* motionWidget = new BlendSpaceMotionWidget(blendSpaceMotion, motionsLayout, static_cast<int>(i));

                connect(motionWidget->m_spinboxX, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &EMotionFX::BlendSpaceMotionContainerWidget::OnPositionXChanged);
                if (motionWidget->m_spinboxY)
                {
                    connect(motionWidget->m_spinboxY, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &EMotionFX::BlendSpaceMotionContainerWidget::OnPositionYChanged);
                }
                connect(motionWidget->m_restoreButton, &QPushButton::clicked, this, &BlendSpaceMotionContainerWidget::OnRestorePosition);
                connect(motionWidget->m_removeButton, &QPushButton::clicked, [this, blendSpaceMotion]()
                {
                    OnRemoveMotion(blendSpaceMotion);
                });

                m_motionWidgets.emplace_back(motionWidget);
            }

            motionsWidget->setLayout(motionsLayout);
            widgetLayout->addWidget(motionsWidget);
        }

        m_containerWidget->setLayout(widgetLayout);
        layout()->addWidget(m_containerWidget);

        UpdateInterface();
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    BlendSpaceMotionContainerHandler::BlendSpaceMotionContainerHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::vector<BlendSpaceNode::BlendSpaceMotion>, BlendSpaceMotionContainerWidget>()
        , m_blendSpaceNode(nullptr)
    {
    }


    AZ::u32 BlendSpaceMotionContainerHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("BlendSpaceMotionContainer");
    }


    QWidget* BlendSpaceMotionContainerHandler::CreateGUI(QWidget* parent)
    {
        BlendSpaceMotionContainerWidget* picker = aznew BlendSpaceMotionContainerWidget(m_blendSpaceNode, parent);

        connect(picker, &BlendSpaceMotionContainerWidget::MotionsChanged, this, [picker]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
        });

        return picker;
    }


    void BlendSpaceMotionContainerHandler::ConsumeAttribute(BlendSpaceMotionContainerWidget* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrValue)
        {
            m_blendSpaceNode = static_cast<BlendSpaceNode*>(attrValue->GetInstance());
            GUI->SetBlendSpaceNode(m_blendSpaceNode);
        }

        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }


    void BlendSpaceMotionContainerHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, BlendSpaceMotionContainerWidget* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetMotions();
    }


    bool BlendSpaceMotionContainerHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, BlendSpaceMotionContainerWidget* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetMotions(instance);
        return true;
    }
} // namespace EMotionFX

#include <Source/Editor/PropertyWidgets/moc_BlendSpaceMotionContainerHandler.cpp>
