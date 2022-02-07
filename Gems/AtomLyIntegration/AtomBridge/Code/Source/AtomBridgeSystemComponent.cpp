/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomBridgeSystemComponent.h>
#include <AtomDebugDisplayViewportInterface.h>
#include <FlyCameraInputComponent.h>
#include <PerViewportDynamicDrawManager.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Interface/Interface.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Entity/GameEntityContextBus.h>

#include <AzCore/Math/MatrixUtils.h>

#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>

#include <Atom/Bootstrap/DefaultWindowBus.h>

namespace AZ
{
    namespace AtomBridge
    {
        void AtomBridgeSystemComponent::Reflect(ReflectContext* context)
        {
            if (SerializeContext* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<AtomBridgeSystemComponent, Component>()
                    ->Version(0)
                ;

                if (EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<AtomBridgeSystemComponent>("AtomBridge", "[Description of functionality provided by this System Component]")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                    ;
                }
            }
        }

        AtomBridgeSystemComponent::AtomBridgeSystemComponent()
        {
        }

        AtomBridgeSystemComponent::~AtomBridgeSystemComponent()
        {
        }

        void AtomBridgeSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("AtomBridgeService", 0xdb816a99));
        }

        void AtomBridgeSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("AtomBridgeService", 0xdb816a99));
        }

        void AtomBridgeSystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ::RHI::Factory::GetComponentService());
            required.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
            required.push_back(AZ_CRC("RPISystem", 0xf2add773));
            required.push_back(AZ_CRC("BootstrapSystemComponent", 0xb8f32711));
        }

        void AtomBridgeSystemComponent::GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void AtomBridgeSystemComponent::Init()
        {
            AZ::RPI::ViewportContextManagerNotificationsBus::Handler::BusConnect();
        }

        void AtomBridgeSystemComponent::Activate()
        {
            AzFramework::Render::RenderSystemRequestBus::Handler::BusConnect();
            AtomBridgeRequestBus::Handler::BusConnect();
            AzFramework::Components::DeprecatedComponentsRequestBus::Handler::BusConnect();
            AzFramework::GameEntityContextRequestBus::BroadcastResult(m_entityContextId, &AzFramework::GameEntityContextRequestBus::Events::GetGameEntityContextId);

            AZ::Render::Bootstrap::NotificationBus::Handler::BusConnect();
            m_dynamicDrawManager = AZStd::make_unique<PerViewportDynamicDrawManager>();
        }

        void AtomBridgeSystemComponent::Deactivate()
        { 
            m_dynamicDrawManager.reset();
            AZ::RPI::ViewportContextManagerNotificationsBus::Handler::BusDisconnect();
            RPI::Scene* scene = AZ::RPI::Scene::GetSceneForEntityContextId(m_entityContextId);
            // Check if scene is emptry since scene might be released already when running AtomSampleViewer 
            if (scene)
            {
                auto auxGeomFP = scene->GetFeatureProcessor<RPI::AuxGeomFeatureProcessorInterface>();
                if (auxGeomFP)
                {
                    auxGeomFP->ReleaseDrawQueueForView(m_view.get());
                }
            }
            // Don't want to leave this until our destructor because the AZ::Data::InstanceDatabase may not be valid at that point
            m_view = nullptr;

            AZ::Render::Bootstrap::NotificationBus::Handler::BusDisconnect();

            AzFramework::Components::DeprecatedComponentsRequestBus::Handler::BusDisconnect();
            AtomBridgeRequestBus::Handler::BusDisconnect();
            AzFramework::Render::RenderSystemRequestBus::Handler::BusDisconnect();
        }

        AZStd::string AtomBridgeSystemComponent::GetRendererName() const
        {
            return "Other";
        }

        void AtomBridgeSystemComponent::EnumerateDeprecatedComponents(AzFramework::Components::DeprecatedComponentsList& list) const
        {
            static const AZ::Uuid legacyRenderComponentUuids[] = {
                AZ::Uuid("{FC315B86-3280-4D03-B4F0-5553D7D08432}"), // EditorMeshComponent
            };

            const AZStd::string deprecatedString = "  (DEPRECATED By Atom)";
            for (const AZ::Uuid& componentUuid : legacyRenderComponentUuids)
            {
                auto deprecatedEntry = list.find(componentUuid);
                if (deprecatedEntry == list.end())
                {
                    list[componentUuid] = AzFramework::Components::DeprecatedInfo{ true, deprecatedString };
                }
                else
                {
                    deprecatedEntry->second.m_hideComponent = true;
                    deprecatedEntry->second.m_deprecationString += deprecatedString;
                }
            }
        }

        void AtomBridgeSystemComponent::OnBootstrapSceneReady(AZ::RPI::Scene* bootstrapScene)
        {
            // Make default AtomDebugDisplayViewportInterface
            AZStd::shared_ptr<AtomDebugDisplayViewportInterface> mainEntityDebugDisplay =
                AZStd::make_shared<AtomDebugDisplayViewportInterface>(AzFramework::g_defaultSceneEntityDebugDisplayId, bootstrapScene);
            m_activeViewportsList[AzFramework::g_defaultSceneEntityDebugDisplayId] = mainEntityDebugDisplay;
        }

        void AtomBridgeSystemComponent::OnViewportContextAdded(AZ::RPI::ViewportContextPtr viewportContext)
        {
                AZStd::shared_ptr<AtomDebugDisplayViewportInterface> viewportDebugDisplay = AZStd::make_shared<AtomDebugDisplayViewportInterface>(viewportContext);
                m_activeViewportsList[viewportContext->GetId()] = viewportDebugDisplay;
        }

        void AtomBridgeSystemComponent::OnViewportContextRemoved(AzFramework::ViewportId viewportId)
        {
            AZ_Assert(viewportId != AzFramework::g_defaultSceneEntityDebugDisplayId, "Error trying to remove the default scene draw instance");
            m_activeViewportsList.erase(viewportId);
        }


    } // namespace AtomBridge
} // namespace AZ
