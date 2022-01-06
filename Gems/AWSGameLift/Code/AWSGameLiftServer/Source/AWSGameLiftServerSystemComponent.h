/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace AWSGameLift
{
    struct GameLiftServerProcessDesc;
    class AWSGameLiftServerManager;

    //! Gem server system component. Responsible for managing the server process for hosting game sessions via the GameLift server manager.
    class AWSGameLiftServerSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(AWSGameLiftServerSystemComponent, "{fa2b46d6-82a9-408d-abab-62bae5ab38c9}");

        AWSGameLiftServerSystemComponent();
        virtual ~AWSGameLiftServerSystemComponent();

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

        void SetGameLiftServerManager(AZStd::unique_ptr<AWSGameLiftServerManager> gameLiftServerManager);

    private:
        AZStd::unique_ptr<AWSGameLiftServerManager> m_gameLiftServerManager;
    };

} // namespace AWSGameLift
