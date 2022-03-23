#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/Component/TickBus.h>

#include <AtomCore/Instance/Instance.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

#include <SharedBuffer.h>

#include <MultiDispatchComputePass.h>

namespace Render
{
    class MeshFeatureProcessorInterface;
}

namespace RPI
{
    class RenderPipeline;
}

namespace AZ
{
    class MeshletsRenderObject;

    namespace Meshlets
    {
        class MeshletsRenderObject;


        class MeshletsFeatureProcessor final
            : public RPI::FeatureProcessor
            , private AZ::TickBus::Handler
        {
            Name MeshletsComputePassName;

        public:
            AZ_RTTI(MeshletsFeatureProcessor, "{1D93DE27-2DC4-4E9B-90B3-DCDCB941C920}", RPI::FeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            MeshletsFeatureProcessor();
            virtual ~MeshletsFeatureProcessor();

            void Init(RPI::RenderPipeline* pipeline);

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
            void Render(const FeatureProcessor::RenderPacket& packet) override;
//            void OnRenderEnd() override;

            bool InitComputePass(const Name& passName);

            // AZ::TickBus::Handler overrides
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            int GetTickOrder() override;

            // RPI::SceneNotificationBus overrides ...
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr renderPipeline) override;
            void OnRenderPipelineRemoved(RPI::RenderPipeline* renderPipeline) override;
            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;

            void AddMeshletsRenderObject(MeshletsRenderObject* meshletsRenderObject);
            void RemoveMeshletsRenderObject(MeshletsRenderObject* meshletsRenderObject);

        protected:
            /// Implement equivalent
//            bool CreateMeshDrawPacket(Meshlets::Mesh* currentMesh);

            void CreateResources();
            void CleanResources();
            void CleanPasses();

        private:
            AZ_DISABLE_COPY_MOVE(MeshletsFeatureProcessor);

            AZStd::unique_ptr<Meshlets::SharedBuffer> m_sharedBuffer;  // used for all meshlets geometry buffers.

            Data::Instance<MultiDispatchComputePass> m_computePass;

            AZStd::list<MeshletsRenderObject*> m_meshletsRenderObjects;

            //! The render pipeline is acquired and set when a pipeline is created or changed
            //! and accordingly the passes and the feature processor are associated.
            //! Notice that scene can contain several pipelines all using the same feature
            //! processor.  On the pass side, it will acquire the scene and request the FP, 
            //! but on the FP side, it will only associate to the latest pass hence such a case
            //! might still be a problem.  If needed, it can be resolved using a map for each
            //! pass name per pipeline.
            RPI::RenderPipeline* m_renderPipeline = nullptr;
        };
    } // namespace Meshlets
} // namespace AZ
