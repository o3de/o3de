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

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/vector.h>

namespace AWSCore
{
    class AWSScriptBehaviorBase;

    //! Bootstraps and provides AWS ScriptCanvas behaviors
    class AWSScriptBehaviorsComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(AWSScriptBehaviorsComponent, "{9F37F23F-4229-4A1F-BAA6-4A4AB8422B47}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        static bool AddedBehaviours()
        {
            return m_alreadyAddedBehaviors;
        }

    protected:

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        static void AddBehaviors(); // Add any behaviors you derived from AWSScriptBehaviorBase to the implementation of this function

        static AZStd::vector<AZStd::unique_ptr<AWSScriptBehaviorBase>> m_behaviors;
        static bool m_alreadyAddedBehaviors;
    };
} // namespace AWSCore
