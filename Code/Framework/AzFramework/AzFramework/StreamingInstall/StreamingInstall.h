/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include "StreamingInstallRequests.h"

 // Performance and release configurations disable trace notifications
 // enable this to display standard printfs for debugging
#define DEBUG_STREAMING_INSTALL 0

namespace AzFramework
{
    namespace StreamingInstall
    {
        class StreamingInstallSystemComponent : public AZ::Component
            , public StreamingInstallRequestBus::Handler
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! Component setup
            AZ_COMPONENT(StreamingInstallSystemComponent, "{F5FC13F5-D993-48F5-AD9C-C06C1B51134C}");

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Component overrides
            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            ////////////////////////////////////////////////////////////////////////
            //! AZ::Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;

            //////////////////////////////////////////////////////////////////////////
            //! StreamingInstallRequestBus interface implementation
            void BroadcastOverallProgress() override;
            void BroadcastChunkProgress(const char* chunkId) override;
            void IsChunkInstalled(const char* chunkId) override;
            void ChangeChunkPriority(const AZStd::vector<const char*>& chunkIds) override;
            //////////////////////////////////////////////////////////////////////////

        public:
            ////////////////////////////////////////////////////////////////////////
            //! Base class for platform specific implementations
            class Implementation
            {
            public:
                AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator);

                static Implementation* Create(StreamingInstallSystemComponent& streamingInstallSystemComponent);
                Implementation(StreamingInstallSystemComponent& streamingInstallSystemComponent);

                AZ_DISABLE_COPY_MOVE(Implementation);
                virtual ~Implementation();

                virtual void BroadcastOverallProgress() = 0;
                virtual void BroadcastChunkProgress(const char* chunkId) = 0;
                virtual void ChangeChunkPriority(const AZStd::vector<const char*>& chunkIds) = 0;
                virtual void IsChunkInstalled(const char* chunkId) = 0;

                static void OnChunkDownloadComplete(const AZStd::string& chunkId);
                static void OnChunkProgressChanged(const AZStd::string& chunkId, float progress);
                static void OnPackageProgressChanged(float progress);
                static void OnQueryChunkInstalled(const AZStd::string& chunkId, bool installed);

                StreamingInstallSystemComponent& m_streamingInstallSystemComponent;
            };

        private:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! Private pointer to the platform specific implementation
            AZStd::unique_ptr<Implementation> m_pimpl;
        };
    } //namespace StreamingInstall
}
