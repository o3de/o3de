/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/StreamingInstall/StreamingInstallNotifications.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include "StreamingInstall.h"

namespace AzFramework
{
    namespace StreamingInstall
    {
        ////////////////////////////////////////////////////////////////////////////////////////
        //Streaming install chunk notification behavior handling and reflection for script context
        class StreamingInstallChunkNotificationBusBehaviorHandler
            : public StreamingInstallChunkNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(StreamingInstallChunkNotificationBusBehaviorHandler, "{88242BCF-9AD8-4560-B8CB-CE4680B2F5E6}", AZ::SystemAllocator
                , OnChunkDownloadComplete
                , OnChunkProgressChanged
                , OnQueryChunkInstalled
            );

            void OnChunkDownloadComplete(const AZStd::string& chunkId) override
            {
                Call(FN_OnChunkDownloadComplete, chunkId);
            }
            void OnChunkProgressChanged(const AZStd::string& chunkId, float progress) override
            {
                Call(FN_OnChunkProgressChanged, chunkId, progress);
            }
            void OnQueryChunkInstalled(const AZStd::string& chunkId, bool installed) override
            {
                Call(FN_OnQueryChunkInstalled, chunkId, installed);
            }
        };

        ////////////////////////////////////////////////////////////////////////////////////////
        //Streaming install package notification behavior handling and reflection for script context
        class StreamingInstallPackageNotificationBusBehaviorHandler
            : public StreamingInstallPackageNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(StreamingInstallPackageNotificationBusBehaviorHandler, "{DDF74916-E67C-4CE2-8C5B-84C832C0230C}", AZ::SystemAllocator
                , OnPackageProgressChanged
            );

            void OnPackageProgressChanged(float progress) override
            {
                Call(FN_OnPackageProgressChanged, progress);
            }
        };

        ////////////////////////////////////////////////////////////////////////////////////////
        void StreamingInstallSystemComponent::Init()
        {
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        void StreamingInstallSystemComponent::Activate()
        {
#if DEBUG_STREAMING_INSTALL
            AZ_Printf("StreamingInstall", "Component activated.\n");
#endif
            m_pimpl.reset(Implementation::Create(*this));
            StreamingInstallRequestBus::Handler::BusConnect();
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        void StreamingInstallSystemComponent::Deactivate()
        {
#if DEBUG_STREAMING_INSTALL
            AZ_Printf("StreamingInstall", "Component deactivated.\n");
#endif
            StreamingInstallRequestBus::Handler::BusDisconnect();
            m_pimpl.reset();
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        void StreamingInstallSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<StreamingInstallSystemComponent, AZ::Component>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<StreamingInstallSystemComponent>("StreamingInstall", "Support for streaming install")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<StreamingInstallChunkNotificationBus>("StreamingInstallChunkNotificationBus")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Handler<StreamingInstallChunkNotificationBusBehaviorHandler>();

                behaviorContext->EBus<StreamingInstallPackageNotificationBus>("StreamingInstallPackageNotificationBus")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Handler<StreamingInstallPackageNotificationBusBehaviorHandler>();

                behaviorContext->EBus<StreamingInstallRequestBus>("StreamingInstallRequestBus")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Category, "Streaming Install")
                    ->Event("BroadcastOverallProgress", &StreamingInstallRequestBus::Events::BroadcastOverallProgress)
                    ->Event("BroadcastChunkProgress", &StreamingInstallRequestBus::Events::BroadcastChunkProgress)
                    ->Event("ChangeChunkPriority", &StreamingInstallRequestBus::Events::ChangeChunkPriority)
                    ->Event("IsChunkInstalled", &StreamingInstallRequestBus::Events::IsChunkInstalled);
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        void StreamingInstallSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("StreamingInstallService", 0xb3cf7bef));
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        void StreamingInstallSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("StreamingInstallService", 0xb3cf7bef));
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        void StreamingInstallSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            AZ_UNUSED(required);
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        void StreamingInstallSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        //StreamingInstallRequestBus definitions
        void StreamingInstallSystemComponent::BroadcastOverallProgress()
        {
            if (m_pimpl)
            {
                m_pimpl->BroadcastOverallProgress();
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        void StreamingInstallSystemComponent::BroadcastChunkProgress(const char* chunkId)
        {
            if (m_pimpl)
            {
                m_pimpl->BroadcastChunkProgress(chunkId);
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        void StreamingInstallSystemComponent::IsChunkInstalled(const char* chunkId)
        {
            if (m_pimpl)
            {
                m_pimpl->IsChunkInstalled(chunkId);
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        void StreamingInstallSystemComponent::ChangeChunkPriority(const AZStd::vector<const char*>& chunkIds)
        {
            if (m_pimpl)
            {
                m_pimpl->ChangeChunkPriority(chunkIds);
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        //Implementation definitions
        StreamingInstall::StreamingInstallSystemComponent::Implementation::Implementation(StreamingInstallSystemComponent& streamingInstallSystemComponent)
            : m_streamingInstallSystemComponent(streamingInstallSystemComponent)
        {
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        StreamingInstall::StreamingInstallSystemComponent::Implementation::~Implementation()
        {
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        void StreamingInstall::StreamingInstallSystemComponent::Implementation::OnChunkDownloadComplete(const AZStd::string& chunkId)
        {
#if DEBUG_STREAMING_INSTALL
            AZ_Printf("StreamingInstall", "Completed Installing Chunk %s.\n", chunkId.c_str());
#endif
            AZ::TickBus::QueueFunction([chunkId]()
            {
                StreamingInstallChunkNotificationBus::Event(chunkId, &StreamingInstallChunkNotificationBus::Events::OnChunkDownloadComplete, chunkId);
            });
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        void StreamingInstall::StreamingInstallSystemComponent::Implementation::OnChunkProgressChanged(const AZStd::string& chunkId, float progress)
        {
#if DEBUG_STREAMING_INSTALL
            AZ_Printf("StreamingInstall", "Chunk %s Install Progress: %f\n", chunkId.c_str(), progress);
#endif
            AZ::TickBus::QueueFunction([chunkId, progress]()
            {
                StreamingInstallChunkNotificationBus::Event(chunkId, &StreamingInstallChunkNotificationBus::Events::OnChunkProgressChanged, chunkId, progress);
            });
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        void StreamingInstall::StreamingInstallSystemComponent::Implementation::OnPackageProgressChanged(float progress)
        {
#if DEBUG_STREAMING_INSTALL
            AZ_Printf("StreamingInstall", "Package Install Progress: %f\n", progress);
#endif
            AZ::TickBus::QueueFunction([progress]()
            {
                StreamingInstallPackageNotificationBus::Broadcast(&StreamingInstallPackageNotifications::OnPackageProgressChanged, progress);
            });
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        void StreamingInstall::StreamingInstallSystemComponent::Implementation::OnQueryChunkInstalled(const AZStd::string& chunkId, bool installed)
        {
#if DEBUG_STREAMING_INSTALL
            AZ_Printf("StreamingInstall", "Query Chunk %s Installed: %i.\n", chunkId.c_str(), installed);
#endif
            AZ::TickBus::QueueFunction([chunkId, installed]()
            {
                StreamingInstallChunkNotificationBus::Event(chunkId, &StreamingInstallChunkNotificationBus::Events::OnQueryChunkInstalled, chunkId, installed);
            });
        }
    }
}
