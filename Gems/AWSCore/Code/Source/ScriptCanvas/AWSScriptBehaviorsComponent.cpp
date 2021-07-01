/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/AWSScriptBehaviorsComponent.h>
#include <ScriptCanvas/AWSScriptBehaviorDynamoDB.h>
#include <ScriptCanvas/AWSScriptBehaviorLambda.h>
#include <ScriptCanvas/AWSScriptBehaviorS3.h>

namespace AWSCore
{
    AZStd::vector<AZStd::unique_ptr<AWSScriptBehaviorBase>> AWSScriptBehaviorsComponent::m_behaviors;
    bool AWSScriptBehaviorsComponent::m_alreadyAddedBehaviors = false;

    void AWSScriptBehaviorsComponent::AddBehaviors()
    {
        if (!m_alreadyAddedBehaviors)
        {
            // Add new script behaviors here
            m_behaviors.push_back(AZStd::make_unique<AWSScriptBehaviorDynamoDB>());
            m_behaviors.push_back(AZStd::make_unique<AWSScriptBehaviorLambda>());
            m_behaviors.push_back(AZStd::make_unique<AWSScriptBehaviorS3>());
            m_alreadyAddedBehaviors = true;
        }
    }

    void AWSScriptBehaviorsComponent::Reflect(AZ::ReflectContext* context)
    {
        AddBehaviors();

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AWSScriptBehaviorsComponent, AZ::Component>()
                ->Version(0);

            for (auto&& behavior : m_behaviors)
            {
                behavior->ReflectSerialization(serialize);
            }

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<AWSScriptBehaviorsComponent>("AWSScriptBehaviors", "Provides ScriptCanvas functions for calling AWS")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("AWS"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;

                for (auto&& behavior : m_behaviors)
                {
                    behavior->ReflectEditParameters(editContext);
                }
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            for (auto&& behavior : m_behaviors)
            {
                behavior->ReflectBehaviors(behaviorContext);
            }
        }
    }

    void AWSScriptBehaviorsComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AWSScriptBehaviorsService"));
    }

    void AWSScriptBehaviorsComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AWSScriptBehaviorsService"));
    }

    void AWSScriptBehaviorsComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void AWSScriptBehaviorsComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AWSScriptBehaviorsComponent::Init()
    {
        for (auto&& behavior : m_behaviors)
        {
            behavior->Init();
        }
    }

    void AWSScriptBehaviorsComponent::Activate()
    {
        for (auto&& behavior : m_behaviors)
        {
            behavior->Activate();
        }
    }

    void AWSScriptBehaviorsComponent::Deactivate()
    {
        for (auto&& behavior : m_behaviors)
        {
            behavior->Deactivate();
        }

        // this forces the vector to release its capacity, clear/shrink_to_fit is not
        m_behaviors.swap(AZStd::vector<AZStd::unique_ptr<AWSScriptBehaviorBase>>());
    }
}

