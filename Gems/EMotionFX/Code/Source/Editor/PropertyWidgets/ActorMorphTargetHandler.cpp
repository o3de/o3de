/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/PropertyWidgets/ActorMorphTargetHandler.h>
#include <Editor/ActorEditorBus.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphTarget.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MorphTargetSelectionWindow.h>
#include <QHBoxLayout>
#include <QMessageBox>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(ActorMorphTargetPicker, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ActorSingleMorphTargetHandler, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ActorMultiMorphTargetHandler, EditorAllocator)

    ActorMorphTargetPicker::ActorMorphTargetPicker(bool multiSelection, QWidget* parent)
        : QWidget(parent)
        , m_multiSelection(multiSelection)
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);

        m_pickButton = new QPushButton(this);
        connect(m_pickButton, &QPushButton::clicked, this, &ActorMorphTargetPicker::OnPickClicked);
        hLayout->addWidget(m_pickButton);

        m_resetButton = new QPushButton(this);
        EMStudio::EMStudioManager::MakeTransparentButton(m_resetButton, "Images/Icons/Clear.svg", "Reset selection");
        connect(m_resetButton, &QPushButton::clicked, this, &ActorMorphTargetPicker::OnResetClicked);
        hLayout->addWidget(m_resetButton);

        setLayout(hLayout);
    }


    void ActorMorphTargetPicker::UpdateInterface()
    {
        const size_t morphTargetCount = m_morphTargetNames.size();
        if (morphTargetCount == 0)
        {
            m_pickButton->setText("Select morph targets");
            m_resetButton->setVisible(false);
        }
        else if (morphTargetCount == 1)
        {
            m_pickButton->setText(m_morphTargetNames[0].c_str());
            m_resetButton->setVisible(true);
        }
        else
        {
            const AZStd::string text = AZStd::string::format("%zu morph targets", morphTargetCount);
            m_pickButton->setText(text.c_str());
            m_resetButton->setVisible(true);
        }
    }


    void ActorMorphTargetPicker::SetMorphTargetNames(const AZStd::vector<AZStd::string>& morphTargetNames)
    {
        m_morphTargetNames = morphTargetNames;
        UpdateInterface();
    }


    const AZStd::vector<AZStd::string>& ActorMorphTargetPicker::GetMorphTargetNames() const
    {
        return m_morphTargetNames;
    }


    void ActorMorphTargetPicker::OnPickClicked()
    {
        // Get access to the currently selected actor.
        EMotionFX::ActorInstance* actorInstance = nullptr;
        ActorEditorRequestBus::BroadcastResult(actorInstance, &ActorEditorRequests::GetSelectedActorInstance);
        if (!actorInstance)
        {
            QMessageBox::warning(this, "No Actor Instance", "Cannot open node selection window. No valid actor instance selected.");
            return;
        }
        EMotionFX::Actor* actor = actorInstance->GetActor();

        EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(0);
        if (!morphSetup)
        {
            QMessageBox::warning(this, "No Morph Targets", "The actor has no morph targets.");
            return;
        }

        // Select the previously selected morph targets.
        AZStd::vector<uint32> selection;
        if (!m_morphTargetNames.empty())
        {
            for (const AZStd::string& morphTargetName : m_morphTargetNames)
            {
                const EMotionFX::MorphTarget* morphTarget = morphSetup->FindMorphTargetByName(morphTargetName.c_str());
                if (morphTarget)
                {
                    selection.emplace_back(morphTarget->GetID());
                }
            }
        }

        // Create and show the morph target picker window.
        EMStudio::MorphTargetSelectionWindow selectionWindow(this, m_multiSelection);
        selectionWindow.Update(morphSetup, selection);
        selectionWindow.setModal(true);

        if (selectionWindow.exec() == QDialog::Accepted)
        {
            const size_t selectedMorphTargetCount = selectionWindow.GetMorphTargetIDs().size();

            m_morphTargetNames.clear();
            m_morphTargetNames.reserve(selectedMorphTargetCount);

            for (size_t i = 0; i < selectedMorphTargetCount; ++i)
            {
                const uint32 morphTargetId = selectionWindow.GetMorphTargetIDs()[i];
                const EMotionFX::MorphTarget* morphTarget = morphSetup->FindMorphTargetByID(morphTargetId);
                if (morphTarget)
                {
                    m_morphTargetNames.emplace_back(morphTarget->GetNameString());
                }
            }
            
            UpdateInterface();
            emit SelectionChanged();
        }
    }


    void ActorMorphTargetPicker::OnResetClicked()
    {
        if (m_morphTargetNames.empty())
        {
            return;
        }

        SetMorphTargetNames(AZStd::vector<AZStd::string>());
        emit SelectionChanged();
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    ActorSingleMorphTargetHandler::ActorSingleMorphTargetHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, ActorMorphTargetPicker>()
        , m_multiSelection(false)
    {
    }


    AZ::u32 ActorSingleMorphTargetHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("ActorMorphTargetName");
    }


    QWidget* ActorSingleMorphTargetHandler::CreateGUI(QWidget* parent)
    {
        ActorMorphTargetPicker* picker = aznew ActorMorphTargetPicker(m_multiSelection, parent);

        connect(picker, &ActorMorphTargetPicker::SelectionChanged, this, [picker]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
        });

        return picker;
    }


    void ActorSingleMorphTargetHandler::ConsumeAttribute(ActorMorphTargetPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }


    void ActorSingleMorphTargetHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, ActorMorphTargetPicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetMorphTargetNames();
    }


    bool ActorSingleMorphTargetHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, ActorMorphTargetPicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetMorphTargetNames(instance);
        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    ActorMultiMorphTargetHandler::ActorMultiMorphTargetHandler()
        : ActorSingleMorphTargetHandler()
    {
        m_multiSelection = true;
    }


    AZ::u32 ActorMultiMorphTargetHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("ActorMorphTargetNames");
    }
} // namespace EMotionFX

#include <Source/Editor/PropertyWidgets/moc_ActorMorphTargetHandler.cpp>
