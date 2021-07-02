/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom_Feature_Traits_Platform.h>
#if AZ_TRAIT_LUXCORE_SUPPORTED

#include "LuxCoreRenderer.h"

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzCore/std/containers/compressed_pair.h>
#include <AzFramework/API/ApplicationAPI.h>

#include <luxcore/luxcore.h>

namespace LuxCoreUI
{
    void LaunchLuxCoreUI(const AZStd::string& luxCoreExeFullPath, AZStd::string& commandLine);
}

namespace AZ
{
    namespace Render
    {
        LuxCoreRenderer::LuxCoreRenderer()
        {
            LuxCoreRequestsBus::Handler::BusConnect();
        }

        LuxCoreRenderer::~LuxCoreRenderer()
        {
            LuxCoreRequestsBus::Handler::BusDisconnect();
            ClearLuxCore();
        }

        void LuxCoreRenderer::SetCameraEntityID(AZ::EntityId id)
        {
            m_cameraEntityId = id;
        }

        void LuxCoreRenderer::AddMesh(Data::Asset<RPI::ModelAsset> modelAsset)
        {
            AZStd::string meshId = modelAsset->GetId().ToString<AZStd::string>();
            m_meshs.emplace(AZStd::piecewise_construct_t{},
                AZStd::forward_as_tuple(meshId),
                AZStd::forward_as_tuple(modelAsset));
        }

        void LuxCoreRenderer::AddMaterial(Data::Instance<RPI::Material> material)
        {
            AZStd::string materialId = material->GetId().ToString<AZStd::string>();
            m_materials.emplace(AZStd::piecewise_construct_t{},
                AZStd::forward_as_tuple(materialId),
                AZStd::forward_as_tuple(material));
        }

        void LuxCoreRenderer::AddTexture(Data::Instance<RPI::Image> image, LuxCoreTextureType type)
        {
            AZStd::string textureId = image->GetAssetId().ToString<AZStd::string>();
            m_textures.emplace(AZStd::piecewise_construct_t{},
                AZStd::forward_as_tuple(textureId),
                AZStd::forward_as_tuple(image, type));
        }

        void LuxCoreRenderer::AddObject(AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset, AZ::Data::InstanceId materialInstanceId)
        {
            m_objects.emplace_back(modelAsset->GetId().ToString<AZStd::string>(), materialInstanceId.ToString<AZStd::string>());
        }

        bool LuxCoreRenderer::CheckTextureStatus()
        {
            for (auto it = m_textures.begin(); it != m_textures.end(); ++it)
            {
                if (!it->second.IsTextureReady())
                {
                    return false;
                }
            }
            return true;
        }

        void LuxCoreRenderer::ClearLuxCore()
        {
            m_meshs.clear();
            m_materials.clear();
            m_textures.clear();
            m_objects.clear();
        }

        void LuxCoreRenderer::ClearObject()
        {
            m_objects.clear();
        }

        void LuxCoreRenderer::RenderInLuxCore()
        {
            luxcore::Init();
            luxcore::Scene *luxCoreScene = luxcore::Scene::Create();

            const char* folderName = "luxcoredata";
            if (!AZ::IO::FileIOBase::GetInstance()->Exists(folderName))
            {
                AZ::IO::FileIOBase::GetInstance()->CreatePath(folderName);
            }

            char resolvedPath[1024];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(folderName, resolvedPath, 1024);

            // Camera transform
            if (!m_cameraEntityId.IsValid())
            {
                AZ_Assert(false, "Please set camera entity id");
                return;
            }

            AZ::Transform cameraTransform = AZ::Transform::CreateIdentity();
            AZ::Vector4 cameraUp, cameraFwd, cameraOrig, cameraTarget;
            AZ::TransformBus::EventResult(cameraTransform, m_cameraEntityId, &AZ::TransformBus::Events::GetWorldTM);

            const AZ::Matrix4x4 rotationMatrix = AZ::Matrix4x4::CreateFromTransform(cameraTransform);

            cameraFwd = rotationMatrix.GetColumn(1);
            cameraUp = rotationMatrix.GetColumn(2);
            cameraOrig = rotationMatrix.GetColumn(3);
            cameraTarget = cameraOrig + cameraFwd;

            // Camera parameter
            float nearClip, farClip, fieldOfView;
            Camera::CameraRequestBus::EventResult(fieldOfView, m_cameraEntityId, &Camera::CameraRequestBus::Events::GetFovDegrees);
            Camera::CameraRequestBus::EventResult(nearClip, m_cameraEntityId, &Camera::CameraRequestBus::Events::GetNearClipDistance);
            Camera::CameraRequestBus::EventResult(farClip, m_cameraEntityId, &Camera::CameraRequestBus::Events::GetFarClipDistance);

            // Set Camera
            luxCoreScene->Parse(
                luxrays::Property("scene.camera.lookat.orig")((float)cameraOrig.GetX(), (float)cameraOrig.GetY(), (float)cameraOrig.GetZ()) <<
                luxrays::Property("scene.camera.lookat.target")((float)cameraTarget.GetX(), (float)cameraTarget.GetY(), (float)cameraTarget.GetZ()) <<
                luxrays::Property("scene.camera.up")((float)cameraUp.GetX(), (float)cameraUp.GetY(), (float)cameraUp.GetZ()) <<
                luxrays::Property("scene.camera.fieldofview")(fieldOfView) <<
                luxrays::Property("scene.camera.cliphither")(nearClip) <<
                luxrays::Property("scene.camera.clipyon")(farClip) <<
                luxrays::Property("scene.camera.type")("perspective"));

            // Set Texture
            try {
                for (auto it = m_textures.begin(); it != m_textures.end(); ++it)
                {
                    if (it->second.GetRawDataPointer() != nullptr)
                    {
                        if (it->second.IsIBLTexture())
                        {
                            luxCoreScene->DefineImageMap<float>(std::string(it->first.data()), static_cast<float*>(it->second.GetRawDataPointer()), 1.f, it->second.GetTextureChannels(), it->second.GetTextureWidth(), it->second.GetTextureHeight());
                        }
                        else
                        {
                            luxCoreScene->DefineImageMap<uint8_t>(std::string(it->first.data()), static_cast<uint8_t*>(it->second.GetRawDataPointer()), 1.f, it->second.GetTextureChannels(), it->second.GetTextureWidth(), it->second.GetTextureHeight());
                        }
                        
                        luxCoreScene->Parse(

                            it->second.GetLuxCoreTextureProperties()
                        );
                    }
                    else
                    {
                        AZ_Assert(false, "texture data is nullptr!!!");
                        return;
                    }
                }
            }
            catch (const std::runtime_error& e)
            {
                (void)e;
                AZ_Assert(false, "%s", e.what());
                return;
            }
            catch (const std::exception& e)
            {
                (void)e;
                AZ_Assert(false, "%s", e.what());
                return;
            }


            // Set Material
            try {
                for (auto it = m_materials.begin(); it != m_materials.end(); ++it)
                {
                    luxCoreScene->Parse(
                        it->second.GetLuxCoreMaterialProperties()
                    );
                }
            }
            catch (const std::exception& e)
            {
                (void)e;
                AZ_Assert(false, "%s", e.what());
                return;
            }

            // Set Model
            try {
                for (auto it = m_meshs.begin(); it != m_meshs.end(); ++it)
                {
                    luxCoreScene->DefineMesh(std::string(it->second.GetMeshId().ToString<AZStd::string>().data()),
                        it->second.GetVertexCount(),
                        it->second.GetTriangleCount(),
                        const_cast<float*>(it->second.GetPositionData()),
                        const_cast<unsigned int*>(it->second.GetIndexData()),
                        const_cast<float*>(it->second.GetNormalData()),
                        const_cast<float*>(it->second.GetUVData()),
                        NULL,
                        NULL);
                }
            }
            catch (const std::exception& e)
            {
                (void)e;
                AZ_Assert(false, "%s", e.what());
                return;
            }

            // Objects
            try {
                for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
                {
                    luxCoreScene->Parse(
                        it->GetLuxCoreObjectProperties()
                    );
                }
            }
            catch (const std::exception& e)
            {
                (void)e;
                AZ_Assert(false, "%s", e.what());
                return;
            }

            // RenderConfig
            luxcore::RenderConfig *config = luxcore::RenderConfig::Create(
                luxrays::Property("path.pathdepth.total")(7) <<
                luxrays::Property("path.pathdepth.diffuse")(5) <<
                luxrays::Property("path.pathdepth.glossy")(5) <<
                luxrays::Property("path.pathdepth.specular")(6) <<
                luxrays::Property("path.hybridbackforward.enable")(0) <<
                luxrays::Property("path.hybridbackforward.partition")(0) <<
                luxrays::Property("path.hybridbackforward.glossinessthreshold ")(0.05) <<
                luxrays::Property("path.forceblackbackground.enable")(0) <<
                luxrays::Property("film.noiseestimation.warmup")(8) <<
                luxrays::Property("film.noiseestimation.step")(32) <<
                luxrays::Property("film.width")(1920) <<
                luxrays::Property("film.height")(1080) <<
                luxrays::Property("film.filter.type")("BLACKMANHARRIS") <<
                luxrays::Property("film.filter.width")(1.5) <<
                luxrays::Property("film.imagepipelines.0.0.type")("NOP") <<
                luxrays::Property("film.imagepipelines.0.1.type")("GAMMA_CORRECTION") <<
                luxrays::Property("film.imagepipelines.0.1.value")(2.2f) <<
                luxrays::Property("film.imagepipelines.0.radiancescales.0.enabled")(1) <<
                luxrays::Property("film.imagepipelines.0.radiancescales.0.globalscale")(1) <<
                luxrays::Property("film.imagepipelines.0.radiancescales.0.rgbscale")(1, 1, 1) <<
                luxrays::Property("film.outputs.0.type")("RGB_IMAGEPIPELINE") <<
                luxrays::Property("film.outputs.0.index")(0) <<
                luxrays::Property("film.outputs.0.filename")("RGB_IMAGEPIPELINE_0.png") <<
                luxrays::Property("sampler.type")("SOBOL") <<
                luxrays::Property("renderengine.type")("PATHCPU") <<
                luxrays::Property("renderengine.seed")(1) <<
                luxrays::Property("lightstrategy.type")("LOG_POWER") <<
                luxrays::Property("scene.epsilon.min")(9.9999997473787516e-06f) <<
                luxrays::Property("scene.epsilon.max")(0.10000000149011612f) <<
                luxrays::Property("scene.epsilon.max")(0.10000000149011612f) <<
                luxrays::Property("batch.haltthreshold")(0.01953125f) <<
                luxrays::Property("batch.haltthreshold.warmup")(64) <<
                luxrays::Property("batch.haltthreshold.step")(64) <<
                luxrays::Property("batch.haltthreshold.filter.enable")(1) <<
                luxrays::Property("batch.haltthreshold.stoprendering.enable")(1) <<
                luxrays::Property("batch.haltspp")(0) <<
                luxrays::Property("batch.halttime")(0) <<
                luxrays::Property("filesaver.renderengine.type")("PATHCPU") <<
                luxrays::Property("filesaver.format")("TXT"),
                luxCoreScene);


            // Export
            try {
                config->Export(resolvedPath);
            }
            catch (const std::runtime_error& e)
            {
                (void)e;
                AZ_Assert(false, "%s", e.what());
                return;
            }

            // Run luxcoreui.exe
            AZStd::string luxCoreExeFullPath;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(luxCoreExeFullPath, &AzFramework::ApplicationRequests::GetAppRoot);
            luxCoreExeFullPath = luxCoreExeFullPath + AZ_TRAIT_LUXCORE_EXEPATH;
            AzFramework::StringFunc::Path::Normalize(luxCoreExeFullPath);

            AZStd::string commandLine = "-o " + AZStd::string(resolvedPath) + "/render.cfg";

            LuxCoreUI::LaunchLuxCoreUI(luxCoreExeFullPath, commandLine);
        }
    }
}
#endif
