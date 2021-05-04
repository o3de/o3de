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

#include <AtomBridgeSystemComponent.h>
#include <AtomDebugDisplayViewportInterface.h>
#include <FlyCameraInputComponent.h>

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
            AZ::Interface<AzFramework::AtomActiveInterface>::Register(this);
        }

        AtomBridgeSystemComponent::~AtomBridgeSystemComponent()
        {
            AZ::Interface<AzFramework::AtomActiveInterface>::Unregister(this);
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

        static const AZ::Crc32 mainViewportEntityDebugDisplayId = AZ_CRC_CE("MainViewportEntityDebugDisplayId");

        void AtomBridgeSystemComponent::Init()
        {
#if defined(ENABLE_ATOM_DEBUG_DISPLAY) && ENABLE_ATOM_DEBUG_DISPLAY
            AZ::RPI::ViewportContextManagerNotificationsBus::Handler::BusConnect();
#endif
        }

        void AtomBridgeSystemComponent::Activate()
        {
            AzFramework::Render::RenderSystemRequestBus::Handler::BusConnect();
            AtomBridgeRequestBus::Handler::BusConnect();
            AzFramework::Components::DeprecatedComponentsRequestBus::Handler::BusConnect();
            AzFramework::GameEntityContextRequestBus::BroadcastResult(m_entityContextId, &AzFramework::GameEntityContextRequestBus::Events::GetGameEntityContextId);

            AZ::Render::Bootstrap::NotificationBus::Handler::BusConnect();
        }

        void AtomBridgeSystemComponent::Deactivate()
        { 
#if defined(ENABLE_ATOM_DEBUG_DISPLAY) && ENABLE_ATOM_DEBUG_DISPLAY
            AZ::RPI::ViewportContextManagerNotificationsBus::Handler::BusDisconnect();
#endif
            RPI::Scene* scene = RPI::RPISystemInterface::Get()->GetDefaultScene().get();
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
            AZStd::shared_ptr<AZ::RPI::WindowContext> windowContext;
            AZ::Render::Bootstrap::DefaultWindowBus::BroadcastResult(windowContext, &AZ::Render::Bootstrap::DefaultWindowInterface::GetDefaultWindowContext);

            if (!windowContext)
            {
                AZ_Warning("Atom", false, "Cannot initialize Atom because no window context is available");
                return;
            }

            AZ::RPI::RenderPipelinePtr renderPipeline = bootstrapScene->GetDefaultRenderPipeline();

            // If RenderPipeline doesn't have a default view, create a view and make it the default view.
            // These settings will be overridden by the editor or game camera.
            if (renderPipeline->GetDefaultView() == nullptr)
            {
                auto viewContextManager = AZ::Interface<RPI::ViewportContextRequestsInterface>::Get();
                m_view = AZ::RPI::View::CreateView(AZ::Name("AtomSystem Default View"), RPI::View::UsageCamera);
                viewContextManager->PushView(viewContextManager->GetDefaultViewportContextName(), m_view);
                const auto& viewport = windowContext->GetViewport();
                const float aspectRatio = viewport.m_maxX / viewport.m_maxY;

                // Note: This is projection assumes a setup for reversed depth
                AZ::Matrix4x4 viewToClipMatrix;
                AZ::MakePerspectiveFovMatrixRH(viewToClipMatrix, AZ::Constants::HalfPi, aspectRatio, 0.1f, 100.f, true);

                m_view->SetViewToClipMatrix(viewToClipMatrix);

                renderPipeline = bootstrapScene->GetDefaultRenderPipeline();
                renderPipeline->SetDefaultView(m_view);

                auto auxGeomFP = bootstrapScene->GetFeatureProcessor<RPI::AuxGeomFeatureProcessorInterface>();
                if (auxGeomFP)
                {
                    auxGeomFP->GetOrCreateDrawQueueForView(m_view.get());
                }

#if defined(ENABLE_ATOM_DEBUG_DISPLAY) && ENABLE_ATOM_DEBUG_DISPLAY
                // Make default AtomDebugDisplayViewportInterface for the scene
                AZStd::shared_ptr<AtomDebugDisplayViewportInterface> mainEntityDebugDisplay = AZStd::make_shared<AtomDebugDisplayViewportInterface>(mainViewportEntityDebugDisplayId);
                m_activeViewportsList[mainViewportEntityDebugDisplayId] = mainEntityDebugDisplay;
#endif
            }
        }

        void AtomBridgeSystemComponent::OnViewportContextAdded(AZ::RPI::ViewportContextPtr viewportContext)
        {
#if defined(ENABLE_ATOM_DEBUG_DISPLAY) && ENABLE_ATOM_DEBUG_DISPLAY
                AZStd::shared_ptr<AtomDebugDisplayViewportInterface> viewportDebugDisplay = AZStd::make_shared<AtomDebugDisplayViewportInterface>(viewportContext);
                m_activeViewportsList[viewportContext->GetId()] = viewportDebugDisplay;
#endif
        }

        void AtomBridgeSystemComponent::OnViewportContextRemoved(AzFramework::ViewportId viewportId)
        {
#if defined(ENABLE_ATOM_DEBUG_DISPLAY) && ENABLE_ATOM_DEBUG_DISPLAY
            m_activeViewportsList.erase(viewportId);
#else
            AZ_UNUSED(viewportId);
#endif
        }


    } // namespace AtomBridge
} // namespace AZ
