/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <MicrophoneBus.h>

namespace Audio
{
    ////////////////////////////////////////////////////////////////////////////
    class MicrophoneSystemComponent
        : public AZ::Component
        , protected MicrophoneRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MicrophoneSystemComponent, "{99982335-B44A-48A9-BBE5-851B4B3BB5E3}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        MicrophoneSystemComponent();
        ~MicrophoneSystemComponent() override;

    protected:

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // MicrophoneRequestBus interface implementation
        bool InitializeDevice() override;
        void ShutdownDevice() override;

        bool StartSession() override;
        void EndSession() override;

        bool IsCapturing() override;

        SAudioInputConfig GetFormatConfig() const override;
        AZStd::size_t GetData(void** outputData, AZStd::size_t numFrames, const SAudioInputConfig& targetConfig, bool shouldDeinterleave) override;
        ////////////////////////////////////////////////////////////////////////

    public:
        class Implementation : public MicrophoneRequestBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator);

            static Implementation* Create();
        };

    private:
        Implementation* m_impl = nullptr;

        bool m_initialized = false;
    };

} // namespace Audio
