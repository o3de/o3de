/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            PropertyVisibility::InertiaProperties |
            PropertyVisibility::Damping |
            PropertyVisibility::SleepOptions |
            PropertyVisibility::Interpolation |
            PropertyVisibility::Gravity |
            PropertyVisibility::ContinuousCollisionDetection |
            PropertyVisibility::MaxVelocities;
    }

    void RagdollNodeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<RagdollNodeConfiguration, RigidBodyConfiguration>()
                ->Version(4, &ClassConverters::RagdollNodeConfigConverter)
                ->Field("JointLimit", &RagdollNodeConfiguration::m_jointLimit)
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

    void RagdollConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<RagdollConfiguration, WorldBodyConfiguration>()
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