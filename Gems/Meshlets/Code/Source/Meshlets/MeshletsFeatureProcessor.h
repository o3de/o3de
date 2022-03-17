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
#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>

namespace Render
{
    class MeshFeatureProcessorInterface;
}

namespace AZ
{
    class MeshletsModel;

    namespace Meshlets
    {
        class MeshletsAsset;

        using ModelsMapByModel = AZStd::unordered_map<MeshletsModel*, Render::MeshFeatureProcessorInterface::MeshHandle>;
        using ModelsMapByName = AZStd::unordered_map<AZStd::string, MeshletsModel*>;

        class MeshletsFeatureProcessor final
            : public RPI::FeatureProcessor
            , private AZ::TickBus::Handler
        {
        public:
            AZ_RTTI(MeshletsFeatureProcessor, "{1D93DE27-2DC4-4E9B-90B3-DCDCB941C920}", RPI::FeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            MeshletsFeatureProcessor();
            virtual ~MeshletsFeatureProcessor();

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
            void Render(const FeatureProcessor::RenderPacket& packet) override;
//            void OnRenderEnd() override;

            // AZ::TickBus::Handler overrides
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            int GetTickOrder() override;

            // RPI::SceneNotificationBus overrides ...
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr renderPipeline) override;
            void OnRenderPipelineRemoved(RPI::RenderPipeline* renderPipeline) override;
//            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;

            void AddMeshletsModel(MeshletsModel* meshletsModel);
            bool RemoveMeshletsModel(MeshletsModel* meshletsModel);

        protected:

            /// Implement equivalent
//            bool CreateMeshDrawPacket(Meshlets::Mesh* currentMesh);

            void CleanResource();


        private:
            AZ_DISABLE_COPY_MOVE(MeshletsFeatureProcessor);

            Render::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;

            uint32_t m_memoryUsage = 0;
        };
    } // namespace Meshlets
} // namespace AZ
