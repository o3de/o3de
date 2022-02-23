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

#include <umbra/Client.hpp>
#include <umbra/Runtime.hpp>

namespace Umbra
{
    class AssetLoad;
}

namespace Render
{
    class MeshFeatureProcessorInterface;
}

namespace AZ
{
    namespace AtomSceneStream
    {
        class Mesh;

        struct MeshCreationStruct
        {
            AZStd::string m_name;
            AZStd::thread m_creationThread;
//            UmbraAssetLoad* assetLoad = nullptr;
            Umbra::AssetLoad assetLoad;
            bool m_finished = false;
        };

        using ModelsMapByModel = AZStd::unordered_map<AtomSceneStream::Mesh*, Render::MeshFeatureProcessorInterface::MeshHandle>;
        using ModelsMapByName = AZStd::unordered_map<AZStd::string, AtomSceneStream::Mesh*>;
        using ThreadMapByName = AZStd::unordered_map<AZStd::string, MeshCreationStruct>;
//        using ModelsMapByName = AZStd::unordered_map<AZStd::string, uint32_t>;

        const uint32_t GPU_MEMORY_LIMIT = 768 * 1024 * 1024; // 1 GiB

        class AtomSceneStreamFeatureProcessor final
            : public RPI::FeatureProcessor
            , private AZ::TickBus::Handler
        {
        public:
            AZ_RTTI(AtomSceneStreamFeatureProcessor, "{04AF8DF3-CF8B-478B-B52A-050B7161844D}", RPI::FeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            AtomSceneStreamFeatureProcessor();
            virtual ~AtomSceneStreamFeatureProcessor();

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

        protected:
            bool CreateMeshDrawPacket(AtomSceneStream::Mesh* currentMesh);
            AtomSceneStream::Mesh* CreateMesh(Umbra::AssetLoad& assetLoad);
            bool CreateMeshFromAssetLoad(AZStd::string threadName, Umbra::AssetLoad& assetLoad);
            void CleanResource();
            void DebugDraw(RPI::AuxGeomDrawPtr auxGeom, AtomSceneStream::Mesh* currentMesh, Vector3& offset, const Color& debugColor);
            void DebugDrawMeshes(RPI::AuxGeomDrawPtr auxGeom, AtomSceneStream::Mesh* currentMesh, const Color& debugColor);
            void UpdateUmbraViewCamera();
            bool StartUmbraClient();
            bool LoadStreamedAsset();
            bool UnloadStreamedAsset();
            void HandleAssetsStreaming(float seconds);

            bool ParallelLoadStreamedAsset();
            bool ParallelUnloadStreamedAsset();

//            bool StartMeshCreationThread(Umbra::AssetLoad& assetLoad);
            bool StartAssetCreationThread();
            bool StartStreamingThread();
            void StopStreamingThread();
//            void HandleAssetsStreamingInThread();


        private:
            AZ_DISABLE_COPY_MOVE(AtomSceneStreamFeatureProcessor);

            Render::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;
            ModelsMapByName m_modelsMapByName;
//            ModelsMapByModel m_modelsMapByModel;

            float m_quality = 0.5f;     // adjusted based on memory consumption
            // The Umbra scene is centered around X and Y but starting at 0 for Z. For this reason
            // it seems that Umbra camera requires this offset to properly do the culling?!
            // [Adi] - investigate with Umbra team
            float m_heightOffset = 0;   
            uint32_t m_memoryUsage = 0;

            Umbra::EnvironmentInfo m_env;
            Umbra::Client* m_client = nullptr;
            Umbra::Runtime* m_runtime = nullptr;
            Umbra::Scene m_scene;
            Umbra::View m_view;

            // Thread management
            ThreadMapByName m_loadingThreads;
            AZStd::mutex m_assetCreationMutex;
//            Umbra::AssetLoad m_nextAsset;
            UmbraAssetLoad* m_nextUmbraAsset = nullptr;

            // The following was designed for creating a streaming thread running in parallel
            // to the render thread.  This approach seems to fail since Umbra is highly dependent
            // on status of streamed assets to maintain proper assets management.
            AZStd::thread m_streamerThread;
            AZStd::thread_desc m_streamerThreadDesc;
            AZStd::atomic_bool m_isStreaming{ false };

            bool m_readyForStreaming = false;
            bool m_isConnectedAndStreaming = false;

            // Current multi-threading approach is to open a thread on each asset creation. The
            // problem here is that assets almost always depend on others (material depends on
            // textures, and each mesh depends on its material).  The current direction is to
            // test the pointer of the dependencies, and only if these are filled, continue with
            // the asset creation, otherwise cancel the current streaming loop and try again in
            // the next frame.
            // Max gains:
            // 1. the load time now depends on the longest asset load time (largest texture for
            //      material), hence for material is can speed up up to 3 times.
            // 2. For mesh, we now depend on material load, but several meshes can be loaded
            //      in parallel and their materials and textures can be loaded in parallel.
            bool m_multiThreadedAssetCreation = false;

            uint32_t m_modelsCreatedThisFrame;
            uint32_t m_modelsRenderedThisFrame;
            uint32_t m_modelsRequiredThisFrame;
        };
    } // namespace AtomSceneStream
} // namespace AZ
