#include <AzCore/std/parallel/thread.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>

//#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>

#include <AzFramework/Components/CameraBus.h>

#include <Atom/Feature/RenderCommon.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>

#include <AtomSceneStreamAssets.h>
#include <AtomSceneStreamFeatureProcessor.h>

#pragma optimize("", off)

#ifndef SAFE_DELETE
#define SAFE_DELETE(p){if(p){delete p;p=nullptr;}}
#endif

namespace AZ
{
    namespace AtomSceneStream
    {
        static bool printDebugInfo = false;
        static bool printDebugStats = false;
        static bool printDebugRemoval = false;
        static bool printDebugRunTimeHiding = false;
        static bool printDebugAdd = false;
        static bool debugDraw = false;
        static bool debugSpheres = false;

        AtomSceneStreamFeatureProcessor::AtomSceneStreamFeatureProcessor()
        {
        }

        void AtomSceneStreamFeatureProcessor::CleanResource()
        {
            StopStreamingThread();

            for (auto& assetThread : m_loadingThreads)
            {   // The following will create a wait on all existing threads.  SHould we limit the amount?
                assetThread.second.m_creationThread.join();
            }
            m_loadingThreads.clear();

//            m_modelsMapByModel.clear();
            m_modelsMapByName.clear();

            m_view.destroy();
            m_scene.destroy();
            if (m_runtime)
            {
                m_runtime->destroy();
            }
            if (m_client)
            {
                m_client->destroy();
            }

            SAFE_DELETE(m_runtime);
            SAFE_DELETE(m_client);
            AZ_Warning("AtomSceneStream", false, "\n**********\nCLEANING ALL RESOURCES AND SHUTTING DOWN UMBRA\n**********");
        }

        bool AtomSceneStreamFeatureProcessor::StartUmbraClient()
        {
            const char* apiKey = "";   // in our case should be empty string as per the mail
            // Replace the following with data from a component
            const char* locator = nullptr;

            [[maybe_unused]] const char* orgLocator =
                "v=1&project=atom%40amazon.com&model=874639baa2218cf4f35019d0dca45c1f4a3435e4&version=default&endpoint=https%3A%2F%2Fhorizon-api-stag.hazel.dex.robotics.a2z.com%2F";
            // Porsche model
            [[maybe_unused]] const char* porscheLocator =
                "v=1&project=atom%40amazon.com&model=porsche-918-spyder-20211119_121403&version=-991378590&endpoint=https%3A%2F%2Fhorizon-api-stag.hazel.dex.robotics.a2z.com%2F";
            //Bentley model
            [[maybe_unused]] const char* bentleyLocator =
                "v=1&project=atom%40amazon.com&model=bentley-flying-spur-2020-20211119_124412&version=-386855994&endpoint=https%3A%2F%2Fhorizon-api-stag.hazel.dex.robotics.a2z.com%2F";

            locator = porscheLocator;

            // Create a Client
            m_client = new Umbra::Client("RuntimeSample");

            // Create Umbra runtime
            UmbraEnvironmentInfoDefaults(&m_env);
            // Texture support flags is the set of textures that are supported. Runtime attempts to deliver only textures
            // that are supported.
            m_env.textureSupportFlags = UmbraTextureSupportFlags_BC1 | UmbraTextureSupportFlags_BC3 |
                UmbraTextureSupportFlags_BC5 | UmbraTextureSupportFlags_Float;

            m_runtime = new Umbra::Runtime(**m_client, m_env);
            if (!m_runtime || !*m_runtime)
            {
                AZ_Error("AtomSceneStream", false, "Error creating Umbra run time");
                CleanResource();
                return false;
            }

            // Create a Scene by connecting it to a model
            m_scene = m_runtime->createScene(apiKey, locator);

            // Wait for connection so that camera can be initialized
            for (;;)
            {
                Umbra::ConnectionStatus s = m_scene.getConnectionStatus();
                if (s == UmbraConnectionStatus_Connected)
                    break;
                else if (s == UmbraConnectionStatus_ConnectionError)
                {
                    AZ_Error("AtomSceneStream", false, "Error connecting to Umbra Back end");
                    CleanResource();
                    return false;
                }
            }

            // Create a View
            m_view = m_runtime->createView();

            Umbra::SceneInfo info;
            m_scene.getSceneInfo(info);
            AZ_TracePrintf("AtomSceneStream", "\n============================\nScene Info:\n\tMin = (%.2f, %.2f, %.2f)\n\tMax = (%.2f, %.2f, %.2f)\n============================\n",
                info.bounds.mn.v[0], info.bounds.mn.v[1], info.bounds.mn.v[2],
                info.bounds.mx.v[0], info.bounds.mx.v[1], info.bounds.mx.v[2]
            );

            // [Adi] The following is the Umbra camera offset for correct culling - investigate why it is required.
            m_heightOffset = -0.5f * (info.bounds.mx.v[2] + info.bounds.mn.v[2]);

            AZ_Warning("AtomSceneStream", false, "\n++++++++++++++\nSUCCESSFULLY STARTED UMBRA CLIENT\n+++++++++++++++");

            return true;
        }

        AtomSceneStreamFeatureProcessor::~AtomSceneStreamFeatureProcessor()
        {
            // Destroy Umbra handles
            CleanResource();
        }

        void AtomSceneStreamFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<AtomSceneStreamFeatureProcessor, RPI::FeatureProcessor>()
                    ->Version(0);
            }
        }

        void AtomSceneStreamFeatureProcessor::Activate()
        {
            EnableSceneNotification();
            TickBus::Handler::BusConnect();
        }

        void AtomSceneStreamFeatureProcessor::Deactivate()
        {
            m_readyForStreaming = false;
            DisableSceneNotification();
            TickBus::Handler::BusDisconnect();
        }

        int AtomSceneStreamFeatureProcessor::GetTickOrder()
        {
            return AZ::TICK_PRE_RENDER;
        }

        void AtomSceneStreamFeatureProcessor::UpdateUmbraViewCamera()
        {
            //----------------- Camera Transform Matrix ------------------
            AZ::Transform activeCameraTransform;
            Camera::ActiveCameraRequestBus::BroadcastResult(activeCameraTransform,
                &Camera::ActiveCameraRequestBus::Events::GetActiveCameraTransform);
            const AZ::Matrix4x4 cameraMatrix = AZ::Matrix4x4::CreateFromTransform(activeCameraTransform);

            //--------------- Camera Projection Matrix -------------------
            Camera::Configuration config;
            Camera::ActiveCameraRequestBus::BroadcastResult(config,
                &Camera::ActiveCameraRequestBus::Events::GetActiveCameraConfiguration);

//            Camera::CameraRequestBus::EventResult(viewWidth, m_cameraEntityId, &Camera::CameraRequestBus::Events::GetFrustumWidth);

            float nearDist = AZStd::min(config.m_nearClipDistance, 0.002f);
            float farDist = config.m_farClipDistance;
            // [Adi] - the following aspect is a hack since the aspect is bogus per the code in GetActiveCameraConfiguration
            static float aspectMult = 1.2f;
            static float fovMult = 1.2f;
            static float nearMult = 1.0f;
            float aspectRatio = aspectMult * config.m_frustumWidth / config.m_frustumHeight; 
            Matrix4x4 viewToClipMatrix;
            MakePerspectiveFovMatrixRH(viewToClipMatrix, config.m_fovRadians * fovMult, aspectRatio, nearMult * nearDist, farDist);

            //---------- Setting the ViewInfo for the streaming data query -----------
            // Camera position
            static Vector3 positionAdd = Vector3(0, 0, m_heightOffset);    // [Adi] - this is strange, difference of ~60 with Umbra?!
            Vector3 cameraPos = cameraMatrix.GetTranslation() + positionAdd;

            UmbraFloat3 umbraCameraPos;
            umbraCameraPos.v[0] = cameraPos.GetX();
            umbraCameraPos.v[1] = cameraPos.GetY();
            umbraCameraPos.v[2] = cameraPos.GetZ();

            AZ::Matrix4x4 cameraMatrixAtomToUmbra;

            cameraMatrixAtomToUmbra = cameraMatrix;
            // Zero the translation
            cameraMatrixAtomToUmbra.SetTranslation(Vector3::CreateZero());

            // Inverting since the Umbra matrix math is inverse to ours
            cameraMatrixAtomToUmbra.InvertFast();
            cameraMatrixAtomToUmbra = Matrix4x4::CreateRotationX(AZ::DegToRad(-90.0f)) * cameraMatrixAtomToUmbra;

            // Inverse and transform the position to match Umbra camera position treatment
            cameraPos = -cameraPos;
            Vector3 umbraCameraTranslation = cameraMatrixAtomToUmbra * cameraPos;
            cameraMatrixAtomToUmbra.SetTranslation(umbraCameraTranslation);

            // Camera View-Projection
            AZ::Matrix4x4 worldToClip = viewToClipMatrix * cameraMatrixAtomToUmbra;
            UmbraFloat4_4 umbraWorldToClip;

            worldToClip.StoreToColumnMajorFloat16((float*)&umbraWorldToClip.v[0]);    // Atom and Umbra have transposed matrix usage (column vs' row major)

            // The data 
            Umbra::ViewInfo viewInfo;
            viewInfo.cameraWorldToClip = &umbraWorldToClip;
            viewInfo.cameraPosition = &umbraCameraPos;
            viewInfo.cameraDepthRange = UmbraDepthRange_MinusOneToOne;
            viewInfo.cameraMatrixFormat = UmbraMatrixFormat_ColumnMajor;
            viewInfo.quality = m_quality;
            m_view.setCamera(viewInfo);
        }

        void AtomSceneStreamFeatureProcessor::DebugDraw(
            RPI::AuxGeomDrawPtr auxGeom, AtomSceneStream::Mesh* currentMesh,
            Vector3& offset, const Color& debugColor)
        {
            Vector3 center;
            float radius;

            const Aabb curAABB = currentMesh->GetAABB();
            curAABB.GetAsSphere(center, radius);

            if (debugSpheres)
            {
                auxGeom->DrawSphere(center + offset, radius,
                    debugColor, RPI::AuxGeomDraw::DrawStyle::Line, RPI::AuxGeomDraw::DepthTest::Off,
                    RPI::AuxGeomDraw::DepthWrite::Off, RPI::AuxGeomDraw::FaceCullMode::None, -1);
            }
            else
            {
                auxGeom->DrawAabb(curAABB,
                    debugColor, RPI::AuxGeomDraw::DrawStyle::Line, RPI::AuxGeomDraw::DepthTest::Off,
                    RPI::AuxGeomDraw::DepthWrite::Off, RPI::AuxGeomDraw::FaceCullMode::None, -1);
            }
        }

        void AtomSceneStreamFeatureProcessor::DebugDrawMeshes(
            RPI::AuxGeomDrawPtr auxGeom, AtomSceneStream::Mesh* currentMesh, const Color& debugColor)
        {
            RPI::AuxGeomDraw::AuxGeomDynamicIndexedDrawArguments drawArgs;
            drawArgs.m_verts = (Vector3*)currentMesh->GetVertexDesc().ptr;
            drawArgs.m_vertCount = currentMesh->GetVertexCount();
//            drawArgs.m_indices = (uint32_t*) currentMesh->GetIndexDesc().ptr;
//            drawArgs.m_indexCount = currentMesh->GetIndexCount();
            drawArgs.m_colors = &debugColor;
            drawArgs.m_colorCount = 1;
//            drawArgs.m_opacityType = RPI::AuxGeomDraw::OpacityType::Opaque;
//            auxGeom->DrawTriangles(drawArgs);
            drawArgs.m_size = 15;
            auxGeom->DrawPoints(drawArgs);
        }

        // based on         void ModelDataInstance::BuildDrawPacketList(size_t modelLodIndex)
        bool AtomSceneStreamFeatureProcessor::CreateMeshDrawPacket(AtomSceneStream::Mesh* currentMesh)
        {
            Data::Instance<RPI::Model> atomModel = currentMesh->GetAtomModel();
            RPI::ModelLod& modelLod = *atomModel->GetLods()[0];

            {
                currentMesh->GetMaterial()->PrepareMaterial();  // [Adi] - can it be that this including the compile should be placed after the srg object generation?
                Data::Instance<RPI::Material> material = currentMesh->GetAtomMaterial();

                const auto& materialAsset = material->GetAsset();
                auto& objectSrgLayout = materialAsset->GetObjectSrgLayout();
                if (!objectSrgLayout)
                {
                    AZ_Error("AtomSceneStream", false, "Error --  [%s] per-object ShaderResourceGroup NOT found.",
                        currentMesh->GetName().c_str());
                    return false;
                }

                auto& shaderAsset = materialAsset->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();
                Data::Instance<RPI::ShaderResourceGroup> meshObjectSrg = RPI::ShaderResourceGroup::Create(shaderAsset, objectSrgLayout->GetName());
                if (!meshObjectSrg)
                {
                    AZ_Error("AtomSceneStream", false, "Error -- [%s] Failed to create a new shader resource group.",
                        currentMesh->GetName().c_str());
                    return false;
                }

//                currentMesh->GetMaterial()->PrepareMaterial();  // [Adi] - can it be that this including the compile should be placed after the srg object generation?

                // setup the mesh draw packet
                RPI::MeshDrawPacket drawPacket(modelLod, 0, material, meshObjectSrg);

//                currentMesh->GetMaterial()->PrepareMaterial();  // [Adi] - can it be that this including the compile should be placed after the srg object generation?

                // set the shader option to select forward pass IBL specular if necessary
                if (!drawPacket.SetShaderOption(AZ::Name("o_meshUseForwardPassIBLSpecular"), AZ::RPI::ShaderOptionValue{true}))
                {
                    AZ_Warning("AtomSceneStream", false, "[%s] Failed to set o_meshUseForwardPassIBLSpecular on mesh draw packet",
                        currentMesh->GetName().c_str());
                }

                // stencil bits
                uint8_t stencilRef = Render::StencilRefs::UseIBLSpecularPass | Render::StencilRefs::UseDiffuseGIPass;

                drawPacket.SetStencilRef(stencilRef);
                drawPacket.SetSortKey(0);

                RPI::Scene* scene = GetParentScene();
                if (!drawPacket.Update(*scene, false) && printDebugAdd)
                {   // [Adi] The question here is if to fail or to return as is, and try to update the draw packet
                    // at a later stage.
                    AZ_Warning("AtomSceneStream", false, "Warning -- [%s] Failed to update draw packet during creation - update will be postponed",
                        currentMesh->GetName().c_str());
                }

                currentMesh->SetMeshDrawPacket(drawPacket);
            }

            return true;
        }

        void AtomSceneStreamFeatureProcessor::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            // OnTick can be used instead of the ::Simulate since it is set to be before the render
        }

        void AtomSceneStreamFeatureProcessor::Simulate(const RPI::FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            if (m_readyForStreaming && !m_isConnectedAndStreaming)
            {
                m_isConnectedAndStreaming = StartUmbraClient();
            }
            else
            {
                if (!m_runtime || !*m_runtime)
                    return;

                {
                    const float streamingTimeSlot = 0.025f;
                    HandleAssetsStreaming(streamingTimeSlot);
                }

                UpdateUmbraViewCamera();

                m_runtime->update();

                // Try to remove one free thread per frame.
                AZStd::lock_guard<AZStd::mutex> lock(m_assetCreationMutex);
                for (auto& meshStruct : m_loadingThreads)
                {
                    if (meshStruct.second.m_finished && meshStruct.second.m_creationThread.joinable())
                    {
                        AZ_Warning("AtomSceneStream", false, "Loading thread [%s] is now being deleted after completion", meshStruct.second.m_name.c_str());
                        meshStruct.second.m_creationThread.join();
                        m_loadingThreads.erase(meshStruct.first);
                        break;
                    }
                }
            }

            AZ_UNUSED(packet);
        }

        void AtomSceneStreamFeatureProcessor::Render([[maybe_unused]] const RPI::FeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            if (!m_runtime || !*m_runtime)
                return;

            RPI::Scene* scene = GetParentScene();
            RPI::AuxGeomDrawPtr auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(scene);

            m_modelsRenderedThisFrame = 0;
            m_modelsRequiredThisFrame = 0;
            for (;;)
            {
                Umbra::Renderable batch[128];
                int num = m_view.nextRenderables(batch, sizeof(batch) / sizeof(batch[0]));
                if (!num)
                    break;

                for (int i = 0; i < num; i++)
                {
                    AtomSceneStream::Mesh* currentMesh = (AtomSceneStream::Mesh*)batch[i].mesh;

                    const float* matrixValues = (const float*)&batch[i].transform.v[0].v[0];
                    Matrix3x4 modelMatrix = Matrix3x4::CreateFromColumnMajorFloat16(matrixValues);
                    Transform modelTransform = Transform::CreateFromMatrix3x4(modelMatrix);
                    Vector3 offset = modelMatrix.GetTranslation();

                    ++m_modelsRequiredThisFrame;

//                    if (m_modelsMapByModel.find(currentMesh) == m_modelsMapByModel.end())
                    auto iter = m_modelsMapByName.find(currentMesh->GetName());
                    if (iter == m_modelsMapByName.end())
                    {
                        offset += Vector3(0.01f, 0.01f, 0.01f);
                        AZ_Warning("AtomSceneStream", false, "Model [%s] is not in the registered map yet - Render will be skipped", currentMesh->GetName().c_str());
                        DebugDraw(auxGeom, currentMesh, offset, Colors::Yellow);
                        continue;
                    }

                    if (currentMesh != iter->second)
                    {
                        AZ_Warning("AtomSceneStream", false, "Different model [%s] ptr between the map and Umbra", currentMesh->GetName().c_str());
                    }

                    if (debugDraw)
                    {
                        DebugDraw(auxGeom, currentMesh, offset, Colors::Green);
//                        DebugDrawMeshes(auxGeom, currentMesh, Colors::Green);
                    }

                    // The following two lines are not required unless one wants to change some of
                    // the material's flags and inspect the render (normals direction, etc..)
                    AtomSceneStream::Material* currentMaterial = currentMesh->GetMaterial();
                    currentMaterial->PrepareMaterial();

                    // The material was not compiled yet - this is required for good data
                    if (!currentMesh->Compile(scene))
                    {
                        AZ_Warning("AtomSceneStream", false, "Warning -- Model [%s] with Material [%s] was not compiled",// - skipping render",
                            currentMesh->GetMaterial()->GetName().c_str(), currentMesh->GetName().c_str());
                        DebugDraw(auxGeom, currentMesh, offset, Colors::Red);
 //                       continue;
                    }

                    // The draw packet failed to acquire the RHI Draw Packet - this will require update
                    RPI::MeshDrawPacket& drawPacket = currentMesh->GetMeshDrawPacket();
                    if (!drawPacket.GetRHIDrawPacket() && !drawPacket.Update(*scene, false))
                    {
                        AZ_Error("AtomSceneStream", false, "Warning -- Failed to create draw packet during Render - render will be skipped");
                        continue;
                    }

                    // And add it to the views
                    for (auto& view : packet.m_views)
                    {
//                        currentMesh->GetMeshDrawPacket()->Update(*scene, false);
                        view->AddDrawPacket(currentMesh->GetMeshDrawPacket().GetRHIDrawPacket());
                    }

                    ++m_modelsRenderedThisFrame;
                }
            }

            if (printDebugInfo && printDebugStats)// && modelsRemoved)
            {
                AZ_Warning("AtomSceneStream", false, "\n-------------------\nModels Stats - Total[%d] - Rendered[%d]\n-------------------\n",
                    m_modelsRequiredThisFrame, m_modelsRenderedThisFrame);
            }
        }

        bool AtomSceneStreamFeatureProcessor::CreateMeshFromAssetLoad(AZStd::string threadName, Umbra::AssetLoad& assetLoad)
        {
            AtomSceneStream::Mesh* mesh = CreateMesh(assetLoad);
            if (!mesh)
            {
                assetLoad.finish(UmbraAssetLoadResult_Failure);
                AZ_Warning("AtomSceneStream", false, "Mesh thread FAILED in creation");
            }
            else
            {
                assetLoad.prepare((Umbra::UserPointer)mesh);
                assetLoad.finish(UmbraAssetLoadResult_Success);
                AZ_Warning("AtomSceneStream", false, "Mesh thread finished creation");
            }

            AZStd::lock_guard<AZStd::mutex> lock(m_assetCreationMutex);
            m_loadingThreads[threadName].m_finished = true;
            
            return mesh ? true : false;
        }

        AtomSceneStream::Mesh* AtomSceneStreamFeatureProcessor::CreateMesh(Umbra::AssetLoad& assetLoad)
        {
            bool creationSuccess = true;
            Umbra::MeshInfo info = assetLoad.getMeshInfo();
            AtomSceneStream::Material* material = (AtomSceneStream::Material*)info.material;
            if (!material || !material->GetAtomMaterial())
            {
                AZ_Warning("AtomSceneStream", false, "Warning -- Mesh creation postponed until material is ready");
                return nullptr;
            }

            AtomSceneStream::Mesh* meshPtr = new AtomSceneStream::Mesh(assetLoad);
            if (!meshPtr->IsReady())
            {
                AZ_Error("AtomSceneStream", false, "Error -- Mesh %s FAILED creation - deleting now", meshPtr->GetName().c_str());
                creationSuccess = false;
            }

            meshPtr->GetAtomMaterial()->Compile();  //  during the creation the material was set in the mesh
            if (creationSuccess && !CreateMeshDrawPacket(meshPtr))
            {
                AZ_Warning("AtomSceneStream", false, "Error -- DrawPacket for Model [%s] was not created - deleting now", meshPtr->GetName().c_str());
                creationSuccess = false;
            }

            if (!creationSuccess)
            {
                delete meshPtr;
                return nullptr;
            }

            // Register the entry to mark that the model exists in the current frame.
            m_memoryUsage += meshPtr->GetMemoryUsage();
            m_modelsMapByName[meshPtr->GetName()] = meshPtr;

            return meshPtr;
        }

        //----------------------------------------------------------------------
        // AssetLoad creates a single asset from streamed data.
        bool AtomSceneStreamFeatureProcessor::LoadStreamedAsset()
        {
            Umbra::AssetLoad assetLoad;

            {
                AZStd::lock_guard<AZStd::mutex> lock(m_assetCreationMutex);
//                assetLoad = m_multiThreadedAssetCreation ? AZStd::move(m_nextAsset) : m_runtime->getNextAssetLoad();
                assetLoad = m_multiThreadedAssetCreation ? Umbra::AssetLoad(m_nextUmbraAsset) : m_runtime->getNextAssetLoad();

                if (!assetLoad)
                {
                    AZ_Warning("AtomSceneStream", !m_multiThreadedAssetCreation, "LoadStreamedAsset - NO AssetLoad");
                    return false;
                }

                AZ_Warning("AtomSceneStream", !m_multiThreadedAssetCreation, "LoadStreamedAsset - STARTED AssetLoad");
            }

            // If memory usage is too high, the job is finished with OutOfMemory. This tells Umbra
            // to stop loading the current batch and by reducing the quality, the loading can continue.
            if (m_memoryUsage > GPU_MEMORY_LIMIT)
            {
                m_quality *= 0.875f;
                assetLoad.finish(UmbraAssetLoadResult_OutOfMemory);
                return false;
            }

            void* ptr = nullptr;
            switch (assetLoad.getType())
            {
            case UmbraAssetType_Texture:
                ptr = new AtomSceneStream::Texture(assetLoad);
                m_memoryUsage += ((AtomSceneStream::Texture*)ptr)->GetMemoryUsage();
                break;

            case UmbraAssetType_Material:
                if (AtomSceneStream::Material::MaterialCanBeCreated(assetLoad))
                {
                    ptr = new AtomSceneStream::Material(assetLoad);
                }
                else
                {   // The asset load failed due to missing dependencies.
                    // Return failure and break the streaming until the required assets are
                    // properly created and their pointer are set in the assets using them.
                    assetLoad.finish(UmbraAssetLoadResult_Failure);
                    return false;  
                }
                break;

            case UmbraAssetType_Mesh:
            {
                if (!AtomSceneStream::Mesh::MeshCanBeCreated(assetLoad))
                {   // The asset load failed due to missing dependencies.
                    // Return failure and break the streaming until the required assets are
                    // properly created and their pointer are set in the assets using them.
                    assetLoad.finish(UmbraAssetLoadResult_Failure);
                    return false;
                }

                ptr = (void*)CreateMesh(assetLoad);
                if (!ptr)
                {
                    assetLoad.finish(UmbraAssetLoadResult_Failure);
                }
                else
                {
                    assetLoad.prepare((Umbra::UserPointer)ptr);
                    assetLoad.finish(UmbraAssetLoadResult_Success);
                }

                // While the mesh creation failed, following assets can still be created, therefore return true
                // to continue the streaming loop
                return true;
            }

            default: break;
            }

            assetLoad.prepare((Umbra::UserPointer)ptr);
            assetLoad.finish(UmbraAssetLoadResult_Success);

            AZ_Warning("AtomSceneStream", !m_multiThreadedAssetCreation, "LoadStreamedAsset - SUCCESS");
            return true;
        }

        //----------------------------------------------------------------------
        // AssetUnloadJob tells that an asset can be freed from the GPU. This function frees a single asset
        bool AtomSceneStreamFeatureProcessor::UnloadStreamedAsset()
        {
            if (!m_runtime)
                return false;

            Umbra::AssetUnload assetUnload = m_runtime->getNextAssetUnload();
            if (!assetUnload)
                return false;

            switch (assetUnload.getType())
            {
            case UmbraAssetType_Material:
            {
                delete (AtomSceneStream::Material*)assetUnload.getUserPointer();
                break;
            }

            case UmbraAssetType_Texture:
            {
                AtomSceneStream::Texture* texture = (AtomSceneStream::Texture*)assetUnload.getUserPointer();
                m_memoryUsage -= texture->GetMemoryUsage();
                delete texture;
                break;
            }

            case UmbraAssetType_Mesh:
            {
                AtomSceneStream::Mesh* meshForRemoval = (AtomSceneStream::Mesh*)assetUnload.getUserPointer();
                if (m_meshFeatureProcessor && meshForRemoval)
                {
                    m_memoryUsage -= meshForRemoval->GetMemoryUsage();
                    auto iter = m_modelsMapByName.find(meshForRemoval->GetName());
                    if (iter != m_modelsMapByName.end())
                    {
                        if (printDebugInfo && printDebugRemoval)
                        {
                            AZStd::string errorMessage = "--- Mesh (Streamer) Removal [" + meshForRemoval->GetName() + "]";
                            AZ_Warning("AtomSceneStream", false, errorMessage.c_str());
                        }
                        m_modelsMapByName.erase(iter);
                    }
                    delete meshForRemoval;    // [Adi] - to do: requires postponed deletion (3 frames delay)
                }
                break;
            }
            default: break;
            }

            assetUnload.finish();
            return true;
        }

        bool AtomSceneStreamFeatureProcessor::StartAssetCreationThread()
        {
            // Hold a lock on the map until finished creation of the thread
            AZStd::lock_guard<AZStd::mutex> lock(m_assetCreationMutex);

//            UmbraAssetLoad* nextAssetLoad = m_runtime->getNextAssetLoad();
//            Umbra::AssetLoad nextAssetLoad = m_runtime->getNextAssetLoad();
//            m_nextAsset = m_runtime->getNextAssetLoad();

            m_nextUmbraAsset = UmbraRuntimeNextAssetLoad(*m_runtime);
//            if (!nextAssetLoad)
//            if (!m_nextAsset)
            if (!m_nextUmbraAsset)
            {
                return false;
            }

            static uint32_t assetNumber = 0;
            AZStd::thread_desc threadDesc;
            AZStd::string threadName = "AssetCreation_" + AZStd::to_string(assetNumber++);
            threadDesc.m_name = threadName.c_str();

            MeshCreationStruct& meshCreatinoStruct = m_loadingThreads[threadName];
            meshCreatinoStruct.m_name = threadName;
            meshCreatinoStruct.m_finished = false;
            // The following two lines can both exist as we only need the data and not the process and this point.
//            meshCreatinoStruct.assetLoad = AZStd::move(nextAssetLoad);
//            m_nextAsset = AZStd::move(nextAssetLoad);
            meshCreatinoStruct.m_creationThread = AZStd::thread(threadDesc, [this]() { LoadStreamedAsset(); });

            AZ_Warning("AtomSceneStream", false, "Thread [%s] was created", m_loadingThreads[threadName].m_name.c_str());

            return true;
        }

        void AtomSceneStreamFeatureProcessor::StopStreamingThread()
        {
            if (m_isStreaming)
            {
                m_streamerThread.join();
            }
            m_isStreaming = false;
        }

        // Perform asset streaming work until time budget is reached
        void AtomSceneStreamFeatureProcessor::HandleAssetsStreaming(float seconds)
        {
            if (!m_runtime)
                return;

            auto startTime = AZStd::chrono::system_clock::now();
            int workTimeMilliseconds = int(seconds * 1000.0f + 0.5f);
            auto endTime = startTime + AZStd::chrono::milliseconds(workTimeMilliseconds);

            // First unload old assets to make room for new ones
            while (AZStd::chrono::system_clock::now() < endTime)
                if (!UnloadStreamedAsset())
                    break;

            while (AZStd::chrono::system_clock::now() < endTime)
            {
                if (m_multiThreadedAssetCreation)
                {
                    if (!StartAssetCreationThread())
                        break;
                }
                else
                {
                    if (!LoadStreamedAsset())
                        break;
                }
            }
        }

        void AtomSceneStreamFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]]RPI::RenderPipelinePtr renderPipeline)
        {
            CleanResource();
            m_isConnectedAndStreaming = false;
            m_readyForStreaming = true;
        }

        void AtomSceneStreamFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]]RPI::RenderPipeline* renderPipeline)
        {
            CleanResource();
            m_isConnectedAndStreaming = false;
            m_readyForStreaming = true;
        }

    } // namespace AtomSceneStream
} // namespace AZ

#pragma optimize("", on)
