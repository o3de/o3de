#include <AzCore/std/parallel/thread.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>

#include <Atom/RPI.Public/Scene.h>

#include <Atom/Feature/RenderCommon.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>

#include <MeshletsAssets.h>
#include <MeshletsFeatureProcessor.h>

#pragma optimize("", off)

#ifndef SAFE_DELETE
#define SAFE_DELETE(p){if(p){delete p;p=nullptr;}}
#endif

namespace AZ
{
    namespace Meshlets
    {
        static bool printDebugInfo = false;
        static bool printDebugStats = false;
        static bool printDebugRemoval = false;
        static bool printDebugRunTimeHiding = false;
        static bool printDebugAdd = false;
        static bool debugDraw = false;
        static bool debugSpheres = false;

        MeshletsFeatureProcessor::MeshletsFeatureProcessor()
        {
            CreateResources();
        }

        void MeshletsFeatureProcessor::CreateResources()
        {
            if (!Meshlets::SharedBufferInterface::Get())
            {   // Since there can be several pipelines, allocate the shared buffer only for the
                // first one and from that moment on it will be used through its interface
                AZStd::string sharedBufferName = "MeshletsSharedBuffer";
                uint32_t bufferMaxAlignment = 16;
                uint32_t bufferSize = 256 * 1024 * 1024;
                m_sharedBuffer = AZStd::make_unique<Meshlets::SharedBuffer>(sharedBufferName, bufferSize, bufferMaxAlignment);
            }
        }

        void MeshletsFeatureProcessor::CleanResources()
        {
            m_sharedBuffer.reset();
        }

        MeshletsFeatureProcessor::~MeshletsFeatureProcessor()
        {
            // Destroy Umbra handles
            CleanResources();
        }

        void MeshletsFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<MeshletsFeatureProcessor, RPI::FeatureProcessor>()
                    ->Version(0);
            }
        }

        void MeshletsFeatureProcessor::Activate()
        {
            EnableSceneNotification();
            TickBus::Handler::BusConnect();
        }

        void MeshletsFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();
            TickBus::Handler::BusDisconnect();
        }

        int MeshletsFeatureProcessor::GetTickOrder()
        {
            return AZ::TICK_PRE_RENDER;
        }

        void MeshletsFeatureProcessor::AddMeshletsModel(MeshletsModel* meshletsModel)
        {
            meshletsModel;
        }

        bool MeshletsFeatureProcessor::RemoveMeshletsModel(MeshletsModel* meshletsModel)
        {
            meshletsModel;
            return false;
        }

        /*
        // based on         void ModelDataInstance::BuildDrawPacketList(size_t modelLodIndex)
        bool MeshletsFeatureProcessor::CreateMeshDrawPacket(AtomSceneStream::Mesh* currentMesh)
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
        
        AtomSceneStream::Mesh* MeshletsFeatureProcessor::CreateMesh(Umbra::AssetLoad& assetLoad)
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
*/

        void MeshletsFeatureProcessor::Render([[maybe_unused]] const RPI::FeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            /*
            if (!m_runtime || !*m_runtime)
                return;

            RPI::Scene* scene = GetParentScene();
            RPI::AuxGeomDrawPtr auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(scene);


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
            */
        }

        void MeshletsFeatureProcessor::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            // OnTick can be used instead of the ::Simulate since it is set to be before the render
        }

        void MeshletsFeatureProcessor::Simulate(const RPI::FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            AZ_UNUSED(packet);
        }

        void MeshletsFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
            CleanResources();
            CreateResources();
        }

        void MeshletsFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]] RPI::RenderPipelinePtr renderPipeline)
        {
            CreateResources();
        }

        void MeshletsFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
            CleanResources();
        }

    } // namespace AtomSceneStream
} // namespace AZ

#pragma optimize("", on)
