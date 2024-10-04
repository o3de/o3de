/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CubeMapCapture/CubeMapCaptureComponentController.h>
#include <CubeMapCapture/CubeMapCaptureComponentConstants.h>
#include <Atom/RPI.Public/Scene.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        void CubeMapCaptureComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<CubeMapCaptureComponentConfig>()
                    ->Version(1)
                    ->Field("CaptureType", &CubeMapCaptureComponentConfig::m_captureType)
                    ->Field("SpecularQualityLevel", &CubeMapCaptureComponentConfig::m_specularQualityLevel)
                    ->Field("RelativePath", &CubeMapCaptureComponentConfig::m_relativePath)
                    ->Field("Exposure", &CubeMapCaptureComponentConfig::m_exposure);
            }
        }

        AZ::u32 CubeMapCaptureComponentConfig::OnCaptureTypeChanged()
        {
            m_relativePath.clear();
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        AZ::u32 CubeMapCaptureComponentConfig::GetSpecularQualityVisibilitySetting() const
        {
            // the specular quality level UI control is only visible when the capture type is Specular
            return m_captureType == CubeMapCaptureType::Specular ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
        }

        AZ::u32 CubeMapCaptureComponentConfig::OnSpecularQualityChanged()
        {
            m_relativePath.clear();
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        void CubeMapCaptureComponentController::Reflect(ReflectContext* context)
        {
            CubeMapCaptureComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<CubeMapCaptureComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &CubeMapCaptureComponentController::m_configuration);
            }
        }

        void CubeMapCaptureComponentController::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("TransformService"));
        }

        void CubeMapCaptureComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("CubeMapCaptureService"));
        }

        CubeMapCaptureComponentController::CubeMapCaptureComponentController(const CubeMapCaptureComponentConfig& config)
            : m_configuration(config)
        {
        }

        void CubeMapCaptureComponentController::Activate(AZ::EntityId entityId)
        {
            m_entityId = entityId;

            TransformNotificationBus::Handler::BusConnect(m_entityId);

            m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<CubeMapCaptureFeatureProcessorInterface>(entityId);
            AZ_Assert(m_featureProcessor, "CubeMapCaptureComponentController was unable to find a CubeMapCaptureFeatureProcessor on the EntityContext provided.");

            m_transformInterface = TransformBus::FindFirstHandler(entityId);
            AZ_Warning("CubeMapCaptureComponentController", m_transformInterface, "Unable to attach to a TransformBus handler.");

            // special handling is required if this component is being cloned in the editor:
            // check to see if it is already referenced by another CubeMapCapture component
            if (!m_configuration.m_relativePath.empty() && m_featureProcessor->IsCubeMapReferenced(m_configuration.m_relativePath))
            {
                // clear the cubeMapRelativePath to prevent the newly cloned CubeMapCapture
                // from using the same cubemap path as the original CubeMapCapture
                m_configuration.m_relativePath.clear();
            }
 
            // add this CubeMapCapture to the feature processor
            const AZ::Transform& transform = m_transformInterface->GetWorldTM();
            m_handle = m_featureProcessor->AddCubeMapCapture(transform);

            m_featureProcessor->SetRelativePath(m_handle, m_configuration.m_relativePath);
        }

        void CubeMapCaptureComponentController::Deactivate()
        {
            if (m_featureProcessor)
            {
                m_featureProcessor->RemoveCubeMapCapture(m_handle);
                m_handle = nullptr;
            }

            TransformNotificationBus::Handler::BusDisconnect();

            m_transformInterface = nullptr;
            m_featureProcessor = nullptr;
        }

        void CubeMapCaptureComponentController::SetConfiguration(const CubeMapCaptureComponentConfig& config)
        {
            m_configuration = config;
        }

        const CubeMapCaptureComponentConfig& CubeMapCaptureComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void CubeMapCaptureComponentController::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
        {
            if (!m_featureProcessor)
            {
                return;
            }
        
            m_featureProcessor->SetTransform(m_handle, world);
        }

        void CubeMapCaptureComponentController::SetExposure(float exposure)
        {
            if (!m_featureProcessor)
            {
                return;
            }
        
            m_featureProcessor->SetExposure(m_handle, exposure);
        }

        void CubeMapCaptureComponentController::RenderCubeMap(RenderCubeMapCallback callback, const AZStd::string& relativePath)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            m_featureProcessor->RenderCubeMap(m_handle, callback, relativePath);
        }
    } // namespace Render
} // namespace AZ
