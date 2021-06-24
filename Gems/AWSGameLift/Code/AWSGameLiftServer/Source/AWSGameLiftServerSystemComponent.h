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
        //! Update the serverProcessDesc with appropriate server port number and log paths.
        //! @param serverProcessDesc Desc object to update.
        void UpdateGameLiftServerProcessDesc(GameLiftServerProcessDesc& serverProcessDesc);

        AZStd::unique_ptr<AWSGameLiftServerManager> m_gameLiftServerManager;
    };

} // namespace AWSGameLift
