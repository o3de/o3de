/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#ifdef AZ_PROFILE_TELEMETRY

#include <AzCore/std/parallel/threadbus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/string/string.h>

#include <RADTelemetry/ProfileTelemetryBus.h>

namespace RADTelemetry
{
    class ProfileTelemetryComponent
        : public AZ::Component
        , private AZStd::ThreadEventBus::Handler
        , private AZ::SystemTickBus::Handler
        , private AZ::Debug::ProfilerRequestBus::Handler
        , private ProfileTelemetryRequestBus::Handler
    {
    public:
        AZ_COMPONENT(ProfileTelemetryComponent, "{51118122-7214-4918-BFF3-237E25FF4918}");

        ProfileTelemetryComponent();
        ~ProfileTelemetryComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

    private:
        ProfileTelemetryComponent(const ProfileTelemetryComponent&) = delete;
        //////////////////////////////////////////////////////////////////////////
        // Thread event bus
        void OnThreadEnter(const AZStd::thread_id& id, const AZStd::thread_desc* desc) override;
        void OnThreadExit(const AZStd::thread_id& id) override;

        //////////////////////////////////////////////////////////////////////////
        // SystemTickBus
        void OnSystemTick() override;

        //////////////////////////////////////////////////////////////////////////
        // ProfilerRequstBus
        bool IsActive() override;
        void FrameAdvance(AZ::Debug::ProfileFrameAdvanceType type) override;

        //////////////////////////////////////////////////////////////////////////
        // ProfileTelemetryRequestBus
        void ToggleEnabled() override;
        void SetAddress(const char *address, AZ::u16 port) override;
        void SetCaptureMask(AZ::Debug::ProfileCategoryPrimitiveType mask) override;
        void SetFrameAdvanceType(AZ::Debug::ProfileFrameAdvanceType type) override;

        AZ::Debug::ProfileCategoryPrimitiveType GetCaptureMask() override;
        AZ::Debug::ProfileCategoryPrimitiveType GetDefaultCaptureMask() override;
        tm_api* GetApiInstance() override;

        //////////////////////////////////////////////////////////////////////////
        // Component descriptor
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        //////////////////////////////////////////////////////////////////////////
        // Private helpers
        void Enable();
        void Disable();
        void Initialize();
        bool IsInitialized() const;
        static AZ::Debug::ProfileCategoryPrimitiveType GetDefaultCaptureMaskInternal();

        //////////////////////////////////////////////////////////////////////////
        // Data members
        struct ThreadNameEntry
        {
            AZStd::thread_id id;
            AZStd::string name;
        };
        AZStd::vector<ThreadNameEntry> m_threadNames;
        using LockType = AZStd::mutex;
        using ScopedLock = AZStd::lock_guard<LockType>;
        LockType m_threadNameLock;
        AZStd::atomic_uint m_profiledThreadCount = { 0 };

        const char* m_address = "127.0.0.1";
        char* m_buffer = nullptr;
        AZ::Debug::ProfileCategoryPrimitiveType m_captureMask = GetDefaultCaptureMaskInternal();
        AZ::Debug::ProfileFrameAdvanceType m_frameAdvanceType = AZ::Debug::ProfileFrameAdvanceType::Game;
        AZ::u16 m_port = 4719;
        bool m_running = false;
        bool m_initialized = false;
    };
}

#endif
