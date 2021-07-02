/*
 * /*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
