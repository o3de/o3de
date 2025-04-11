/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <Presence/PresenceRequestBus.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Component/TickBus.h>

namespace Presence
{
    class PresenceSystemComponent
        : public AZ::Component
        , protected PresenceRequestBus::Handler
    {

    #define DEBUG_PRESENCE 1

    public:
        ////////////////////////////////////////////////////////////////////////////////////////
        //! Component setup
        AZ_COMPONENT(PresenceSystemComponent, "{1B04E968-2729-4CA0-8841-21E50FE9133C}");

        ////////////////////////////////////////////////////////////////////////////////////////
        //! Component overrides
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    protected:
        ////////////////////////////////////////////////////////////////////////
        //! AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////
        //! PresenceRequestBus interface implementation
        void SetPresence(const SetPresenceParams& params) override;
        void QueryPresence(const QueryPresenceParams& params) override;

    public:
        ////////////////////////////////////////////////////////////////////////
        //! Base class for platform specific implementations
        class Implementation
        {
        public: 
            AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator);

            static Implementation* Create(PresenceSystemComponent& presenceSystemComponent);
            Implementation(PresenceSystemComponent& presenceSystemComponent);

            AZ_DISABLE_COPY_MOVE(Implementation);

            virtual ~Implementation();

            virtual void SetPresence(const SetPresenceParams& params) = 0;
            virtual void QueryPresence(const QueryPresenceParams& params) = 0;

            static void OnPresenceSetComplete(const SetPresenceParams& params);
            static void OnPresenceQueriedComplete(const QueryPresenceParams& params, const PresenceDetails& details);

            PresenceSystemComponent& m_presenceSystemComponent;
        };

        private:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! Private pointer to the platform specific implementation
            AZStd::unique_ptr<Implementation> m_pimpl;
    };
}
