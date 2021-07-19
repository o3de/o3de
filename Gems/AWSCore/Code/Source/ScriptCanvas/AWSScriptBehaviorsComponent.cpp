/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    void AWSScriptBehaviorsComponent::Reflect(AZ::ReflectContext* context)
    {
        AWSScriptBehaviorDynamoDB::Reflect(context);
        AWSScriptBehaviorLambda::Reflect(context);
        AWSScriptBehaviorS3::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AWSScriptBehaviorsComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<AWSScriptBehaviorsComponent>("AWSScriptBehaviors", "Provides ScriptCanvas functions for calling AWS")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("AWS"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
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

    void AWSScriptBehaviorsComponent::Activate()
    {
    }

    void AWSScriptBehaviorsComponent::Deactivate()
    {
    }
}

