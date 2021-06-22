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
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AWSGameLift
{
    class AWSGameLiftClientManager;

    //! Gem client system component. Responsible for creating the gamelift client manager.
    class AWSGameLiftClientSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(AWSGameLiftClientSystemComponent, "{d481c15c-732a-4eea-9853-4965ed1bc2be}");

        AWSGameLiftClientSystemComponent();
        virtual ~AWSGameLiftClientSystemComponent() = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        void SetGameLiftClientManager(AZStd::unique_ptr<AWSGameLiftClientManager> gameliftClientManager);

    private:
        static void ReflectCreateSessionRequest(AZ::ReflectContext* context);
        static void ReflectSearchSessionsResponse(AZ::ReflectContext* context);

        AZStd::unique_ptr<AWSGameLiftClientManager> m_gameliftClientManager;
    };

} // namespace AWSGameLift
