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
            AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator, 0);

            static Implementation* Create();
        };

    private:
        Implementation* m_impl = nullptr;

        bool m_initialized = false;
    };

} // namespace Audio
