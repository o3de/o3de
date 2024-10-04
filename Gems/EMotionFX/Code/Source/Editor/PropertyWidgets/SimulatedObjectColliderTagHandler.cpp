/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/PhysicsSetup.h>
#include <Editor/PropertyWidgets/SimulatedObjectColliderTagHandler.h>
#include <QHBoxLayout>
#include <QSignalBlocker>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SimulatedObjectColliderTagSelector, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(SimulatedObjectColliderTagHandler, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(SimulatedJointColliderExclusionTagSelector, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(SimulatedJointColliderExclusionTagHandler, AZ::SystemAllocator)

    SimulatedObjectColliderTagSelector::SimulatedObjectColliderTagSelector(QWidget* parent)
        : TagSelector(parent)
    {
        connect(this, &SimulatedObjectColliderTagSelector::TagsChanged, this, [this]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, this);
            });
    }

    void SimulatedObjectColliderTagSelector::GetAvailableTags(QVector<QString>& outTags) const
    {
        outTags.clear();

        if (!m_simulatedObject)
        {
            AZ_Warning("EMotionFX", false, "Cannot collect available tags from simulated object. Simulated object not valid.");
            return;
        }

        const SimulatedObjectSetup* simulatedObjectSetup = m_simulatedObject->GetSimulatedObjectSetup();
        AZ_Assert(simulatedObjectSetup, "Simulated object does not belong to a valid simulated object setup.");

        const Actor* actor = simulatedObjectSetup->GetActor();
        const Physics::CharacterColliderConfiguration& colliderConfig = actor->GetPhysicsSetup()->GetSimulatedObjectColliderConfig();
        QString tag;
        for (const Physics::CharacterColliderNodeConfiguration& nodeConfig : colliderConfig.m_nodes)
        {
            for (const auto& shapeConfigPair : nodeConfig.m_shapes)
            {
                const Physics::ColliderConfiguration* colliderConfig2 = shapeConfigPair.first.get();
                tag = colliderConfig2->m_tag.c_str();

                if (!tag.isEmpty() &&
                    std::find(outTags.begin(), outTags.end(), tag) == outTags.end())
                {
                    outTags.push_back(tag);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    SimulatedObjectColliderTagHandler::SimulatedObjectColliderTagHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, SimulatedObjectColliderTagSelector>()
    {
    }

    AZ::u32 SimulatedObjectColliderTagHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("SimulatedObjectColliderTags");
    }

    QWidget* SimulatedObjectColliderTagHandler::CreateGUI(QWidget* parent)
    {
        return aznew SimulatedObjectColliderTagSelector(parent);
    }

    void SimulatedObjectColliderTagHandler::ConsumeAttribute(SimulatedObjectColliderTagSelector* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
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

    void SimulatedObjectColliderTagHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, SimulatedObjectColliderTagSelector* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetTags();
    }

    bool SimulatedObjectColliderTagHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, SimulatedObjectColliderTagSelector* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);

        if (node && node->GetParent())
        {
            AzToolsFramework::InstanceDataNode* parentNode = node->GetParent();
            m_simulatedObject = static_cast<SimulatedObject*>(parentNode->FirstInstance());
            GUI->SetSimulatedObject(m_simulatedObject);
        }
        else
        {
            GUI->SetSimulatedObject(nullptr);
        }

        GUI->SetTags(instance);
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////

    SimulatedJointColliderExclusionTagSelector::SimulatedJointColliderExclusionTagSelector(QWidget* parent)
        : TagSelector(parent)
    {
        connect(this, &SimulatedJointColliderExclusionTagSelector::TagsChanged, this, [this]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, this);
            });
    }

    void SimulatedJointColliderExclusionTagSelector::GetAvailableTags(QVector<QString>& outTags) const
    {
        outTags.clear();

        if (!m_simulatedObject)
        {
            AZ_Warning("EMotionFX", false, "Cannot collect available tags from simulated object. Simulated object not valid.");
            return;
        }

        AZ_Assert(m_simulatedObject->GetSimulatedObjectSetup(), "Simulated object does not belong to a valid simulated object setup.");
        const AZStd::vector<AZStd::string>& colliderTags = m_simulatedObject->GetColliderTags();

        QString tag;
        for (const AZStd::string& colliderTag : colliderTags)
        {
            tag = colliderTag.c_str();

            if (!tag.isEmpty() &&
                std::find(outTags.begin(), outTags.end(), tag) == outTags.end())
            {
                outTags.push_back(tag);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    SimulatedJointColliderExclusionTagHandler::SimulatedJointColliderExclusionTagHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, SimulatedJointColliderExclusionTagSelector>()
    {
    }

    AZ::u32 SimulatedJointColliderExclusionTagHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("SimulatedJointColliderExclusionTags");
    }

    QWidget* SimulatedJointColliderExclusionTagHandler::CreateGUI(QWidget* parent)
    {
        return aznew SimulatedJointColliderExclusionTagSelector(parent);
    }

    void SimulatedJointColliderExclusionTagHandler::ConsumeAttribute(SimulatedJointColliderExclusionTagSelector* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }

        if (attrib == AZ_CRC_CE("SimulatedObject"))
        {
            attrValue->Read<SimulatedObject*>(m_simulatedObject);
            GUI->SetSimulatedObject(m_simulatedObject);
        }
    }

    void SimulatedJointColliderExclusionTagHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, SimulatedJointColliderExclusionTagSelector* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetTags();
    }

    bool SimulatedJointColliderExclusionTagHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, SimulatedJointColliderExclusionTagSelector* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetTags(instance);
        return true;
    }
} // namespace EMotionFX
