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
#ifndef AZFRAMEWORK_NET_INTERESTMANAGER_COMPONENT_H
#define AZFRAMEWORK_NET_INTERESTMANAGER_COMPONENT_H


#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzFramework/Network/NetBindingSystemBus.h>

#include <GridMate/Session/Session.h>

namespace GridMate
{
    class InterestManager;
    class GridSession;
    class BitmaskInterestHandler;
    class ProximityInterestHandler;
}

namespace AzFramework
{
    class InterestManagerSystemRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~InterestManagerSystemRequests() {}

        // Returns interest manager instance
        virtual GridMate::InterestManager* GetInterestManager() = 0;

        // Returns interest manager instance
        virtual GridMate::BitmaskInterestHandler* GetBitmaskInterest() = 0;

        // Returns interest manager instance
        virtual GridMate::ProximityInterestHandler* GetProximityInterest() = 0;
    };

    // Interface Bus
    using InterestManagerRequestsBus = AZ::EBus<InterestManagerSystemRequests>;

    class InterestManagerEvents
        : public AZ::EBusTraits
    {
    public:
        virtual ~InterestManagerEvents() {}

        // Called when interest manager is initialized and ready to use
        virtual void OnInterestManagerActivate(GridMate::InterestManager* im) { (void)im; }

        // Called when interest manager is deactivated
        virtual void OnInterestManagerDeactivate(GridMate::InterestManager* im) { (void)im; }
    };

    // Interface Bus
    using InterestManagerEventsBus = AZ::EBus<InterestManagerEvents>;

    /**
     * Interest manager component.
     * When component is activated replicas will go through interest filtering before being sent to other peers
    */
    class InterestManagerComponent
        : public AZ::Component
        , public AZ::SystemTickBus::Handler
        , public InterestManagerRequestsBus::Handler
        , public NetBindingSystemEventsBus::Handler
    {
    public:
        AZ_COMPONENT(InterestManagerComponent, "{55371FA7-2942-4A3C-A3EA-27FF2C7DB6C5}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        InterestManagerComponent();

        void Activate() override;
        void Deactivate() override;

    protected:

        // AZ::SystemTickBus::Listener interface implementation
        void OnSystemTick() override;

        // InterestManagerSystemRequests implementation
        GridMate::InterestManager* GetInterestManager() override;
        GridMate::BitmaskInterestHandler* GetBitmaskInterest() override;
        GridMate::ProximityInterestHandler* GetProximityInterest() override;

        // SessionEventBus
        void OnNetworkSessionActivated(GridMate::GridSession* session) override;
        void OnNetworkSessionDeactivated(GridMate::GridSession* session) override;

        void InitInterestManager();
        void ShutdownInterestManager();

        // Interest handlers
        AZStd::unique_ptr<GridMate::InterestManager> m_im;
        AZStd::unique_ptr<GridMate::BitmaskInterestHandler> m_bitmaskHandler;
        AZStd::unique_ptr<GridMate::ProximityInterestHandler> m_proximityHandler;

        GridMate::GridSession* m_session; ///< currently bound session

    private:
        InterestManagerComponent(const InterestManagerComponent&) = delete; //Cannot use default due to unique_ptr.
    };
} // namesapce AzFramework

#endif // AZFRAMEWORK_NET_INTERESTMANAGER_COMPONENT_H
