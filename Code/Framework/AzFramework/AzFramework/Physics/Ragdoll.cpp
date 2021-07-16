/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/ClassConverters.h>


namespace Physics
{
    RagdollNodeConfiguration::RagdollNodeConfiguration()
    {
        m_propertyVisibilityFlags =
            RigidBodyConfiguration::PropertyVisibility::InertiaProperties |
            RigidBodyConfiguration::PropertyVisibility::Damping |
            RigidBodyConfiguration::PropertyVisibility::SleepOptions |
            RigidBodyConfiguration::PropertyVisibility::Interpolation |
            RigidBodyConfiguration::PropertyVisibility::Gravity |
            RigidBodyConfiguration::PropertyVisibility::ContinuousCollisionDetection |
            RigidBodyConfiguration::PropertyVisibility::MaxVelocities;
    }

    void RagdollNodeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<RagdollNodeConfiguration, RigidBodyConfiguration>()
                ->Version(5, &ClassConverters::RagdollNodeConfigConverter)
                ->Field("JointConfig", &RagdollNodeConfiguration::m_jointConfig)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<RagdollNodeConfiguration>("Ragdoll node Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }
    }

    RagdollConfiguration::RagdollConfiguration()
    {
        m_startSimulationEnabled = false; //ragdolls do not start enabled.
    }

    void RagdollConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<RagdollConfiguration, AzPhysics::SimulatedBodyConfiguration>()
                ->Version(2, &ClassConverters::RagdollConfigConverter)
                ->Field("nodes", &RagdollConfiguration::m_nodes)
                ->Field("colliders", &RagdollConfiguration::m_colliders)
            ;
        }
    }

    Physics::RagdollNodeConfiguration* RagdollConfiguration::FindNodeConfigByName(const AZStd::string& nodeName) const
    {
        auto nodeIterator = AZStd::find_if(m_nodes.begin(), m_nodes.end(), [&nodeName](const Physics::RagdollNodeConfiguration& node)
            {
                return node.m_debugName == nodeName;
            });

        if (nodeIterator != m_nodes.end())
        {
            return const_cast<Physics::RagdollNodeConfiguration*>(nodeIterator);
        }

        return nullptr;
    }

    AZ::Outcome<size_t> RagdollConfiguration::FindNodeConfigIndexByName(const AZStd::string& nodeName) const
    {
        auto nodeIterator = AZStd::find_if(m_nodes.begin(), m_nodes.end(), [&nodeName](const Physics::RagdollNodeConfiguration& node)
            {
                return node.m_debugName == nodeName;
            });

        if (nodeIterator != m_nodes.end())
        {
            return AZ::Success(static_cast<size_t>(nodeIterator - m_nodes.begin()));
        }

        return AZ::Failure();
    }

    void RagdollConfiguration::RemoveNodeConfigByName(const AZStd::string& nodeName)
    {
        const AZ::Outcome<size_t> configIndex = FindNodeConfigIndexByName(nodeName);
        if (configIndex.IsSuccess())
        {
            m_nodes.erase(m_nodes.begin() + configIndex.GetValue());
        }
    }
} // namespace Physics
