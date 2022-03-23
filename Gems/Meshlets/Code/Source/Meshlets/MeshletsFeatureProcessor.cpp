#include <AzCore/std/parallel/thread.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#include <Atom/Feature/RenderCommon.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>

#include <MeshletsRenderObject.h>
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
            MeshletsComputePassName = Name("MeshletsComputePass");
        }

        MeshletsFeatureProcessor::~MeshletsFeatureProcessor()
        {
            // Destroy Umbra handles
            CleanResources();
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

        void MeshletsFeatureProcessor::CleanPasses()
        {
            m_computePass = nullptr;
        }

        void MeshletsFeatureProcessor::Init([[maybe_unused]]RPI::RenderPipeline* pipeline)
        {
            InitComputePass(MeshletsComputePassName);
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

        bool MeshletsFeatureProcessor::InitComputePass(const Name& passName)
        {
            m_computePass = Data::Instance<MultiDispatchComputePass>();

            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(passName, m_renderPipeline);
            RPI::Ptr<RPI::Pass> desiredPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
            if (desiredPass)
            {
                m_computePass = static_cast<MultiDispatchComputePass*>(desiredPass.get());
            }
            else
            {
                AZ_Error("Meshlets", false,
                    "%s does not exist in this pipeline. Check your game project's .pass assets.",
                    passName.GetCStr());
                return false;
            }

            return true;
        }

        void MeshletsFeatureProcessor::AddMeshletsRenderObject(MeshletsRenderObject* meshletsRenderObject)
        {
            m_meshletsRenderObjects.push_back(meshletsRenderObject);
        }

        void MeshletsFeatureProcessor::RemoveMeshletsRenderObject(MeshletsRenderObject* meshletsRenderObject)
        {
            m_meshletsRenderObjects.remove(meshletsRenderObject);
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

        void MeshletsFeatureProcessor::Render([[maybe_unused]] const RPI::FeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            // Skip adding draw or dispatch items if there it no hair render objects
            if (m_meshletsRenderObjects.size() == 0)
            {
                return;
            }

            AZStd::list<RHI::DispatchItem*> dispatchItems;
            for (auto renderObject : m_meshletsRenderObjects)
            {
                // [Adi] For demo purposed the index is set for 0.
                // This entire control scheme will be removed to be replaced with GPU
                // driven pipeline control.
                ModelLodDataArray& modelLodArray = renderObject->GetMeshletsRenderData(0);

//                AZStd::vector<MeshRenderData>
                for (auto& renderData : modelLodArray)
                {
                    dispatchItems.push_back(renderData.m_dispatchItem->GetDispatchItem());
                }
            }
            m_computePass->AddDispatchItems(dispatchItems);
        }

        void MeshletsFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
            CleanResources();
            CreateResources();
            m_renderPipeline = renderPipeline;
            Init(m_renderPipeline);
        }

        void MeshletsFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]] RPI::RenderPipelinePtr renderPipeline)
        {
            m_renderPipeline = renderPipeline.get();
            CreateResources();
            Init(m_renderPipeline);
        }

        void MeshletsFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
            m_renderPipeline = nullptr;
            CleanResources();
        }

    } // namespace AtomSceneStream
} // namespace AZ

#pragma optimize("", on)
