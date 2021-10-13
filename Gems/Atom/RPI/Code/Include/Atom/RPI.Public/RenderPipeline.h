/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/DrawList.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

#include <Atom/RPI.Reflect/Pass/PassAsset.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/map.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace AZ
{
    namespace RHI
    {
        class ShaderResourceGroup;
    }

    namespace RPI
    {
        class Scene;
        struct TickTimeInfo;
        class ShaderResourceGroup;
        class AnyAsset;
        class WindowContext;

        enum class PipelineViewType
        {
            Unknown,
            Persistent,     // The view assigned to a pipeline view tag is persistent
            Transient       // The views assigned to a pipeline view tag are transient which only valid for a frame.
        };

        //! PipelineViews contains information about views used by the passes in the RenderPipeline.
        //! If view type is persistent, list of views is limited to just one view
        struct PipelineViews
        {
            PipelineViewTag m_viewTag;
            PipelineViewType m_type;

            PassesByDrawList m_passesByDrawList;

            //! Views associated with the m_viewTag.
            //! There should be only one view in the array if m_type is persistent.
            //! And there might be more than one views if m_type is transient.
            AZStd::vector<ViewPtr> m_views;

            //! Combined DrawListTags collected from passes which are associated with this pipeline view.
            RHI::DrawListMask m_drawListMask;
        };

        //! RenderPipeline describes how to render a scene. It has all the passes and views for rendering.
        //! A scene may have several pipelines. Each pipeline have its own render frequency. 
        //! Pipeline can be disabled and it won't be rendered if it's disabled.
        class RenderPipeline
        {
            friend class Scene;

        public:
            AZ_CLASS_ALLOCATOR(RenderPipeline, AZ::SystemAllocator, 0);

            static RenderPipelinePtr CreateRenderPipeline(const RenderPipelineDescriptor& desc);

            static RenderPipelinePtr CreateRenderPipelineFromAsset(Data::Asset<AnyAsset> pipelineAsset);

            static RenderPipelinePtr CreateRenderPipelineForWindow(const RenderPipelineDescriptor& desc, const WindowContext& windowContext);
            static RenderPipelinePtr CreateRenderPipelineForWindow(Data::Asset<AnyAsset> pipelineAsset, const WindowContext& windowContext);

            // Data type for render pipeline's views' information
            using PipelineViewMap = AZStd::map<PipelineViewTag, PipelineViews, AZNameSortAscending>;

            //! Assign a view for a PipelineViewTag used in this pipeline. 
            //! This reference of this view will be saved until it's replaced in another SetPersistentView call.
            void SetPersistentView(const PipelineViewTag& viewId, ViewPtr view);

            //! Add a view for a PipelineViewTag used in this pipeline.
            //! The view's reference will be only saved for rendering one frame and it will be cleared when the next frame starts.
            //! This function should be used after OnStartFrame is called.
            void AddTransientView(const PipelineViewTag& viewId, ViewPtr view);

            //! Set a view to the default view tag. 
            //! It's the same as SetPersistentView(GetMainViewTag(), view)
            void SetDefaultView(ViewPtr view);

            //! Get the view for the default view tag. 
            //! It's the same as GetViews(GetMainViewTag()) and using first element.
            ViewPtr GetDefaultView();

            //! Set default view from an entity which should have a ViewProvider handler.
            void SetDefaultViewFromEntity(EntityId entityId);

            //! Check if this pipeline has the specified PipelineViewTag.
            bool HasViewTag(const PipelineViewTag& viewTag) const;

            //! Get the main view tag (the tag used for the default view).
            PipelineViewTag GetMainViewTag() const;

            //! Get views that are associated with specified view tag.
            const AZStd::vector<ViewPtr>& GetViews(const PipelineViewTag& viewTag) const;

            //! Get the draw list mask that are associated with specified view tag.
            const RHI::DrawListMask& GetDrawListMask(const PipelineViewTag& viewTag) const;
            
            //! Get this render pipeline's view information
            const PipelineViewMap& GetPipelineViews() const;

            //! Check whether this pipeline needs to be rendered in next tick 
            bool NeedsRender() const;

            RenderPipelineId GetId() const;

            const Ptr<ParentPass>& GetRootPass() const;

            //! This function need to be called by Pass class when any passes are added/removed in this pipeline's pass tree.
            void SetPassModified();

            //! Notifies the pipeline it needs to recreate passes. Typical use case is for pass asset hot reloading.
            void SetPassNeedsRecreate();
            
            //! Update passes and views when any pass was modified which may affect pipeline views.
            //! This function is automatically called when frame starts. User may call it manually when they expect to get up to date view information
            //! after any pass changes.
            void OnPassModified();

            //! Check if this pipeline should be removed after a single execution.
            bool IsExecuteOnce();

            void RemoveFromScene();

            Scene* GetScene() const;

            //! return the window handle associated with this render pipeline if it's created for window
            AzFramework::NativeWindowHandle GetWindowHandle() const;

            //! Return the render settings that can be queried by passes to setup things like render target resolution
            PipelineRenderSettings& GetRenderSettings();
            const PipelineRenderSettings& GetRenderSettings() const;

            //! Undoes runtime changes made to active render settings by reverting to original settings from the descriptor
            void RevertRenderSettings();

            //! Add this RenderPipeline to the next RPI system's RenderTick and it will be rendered once.
            //! This function can be used for render a renderpipeline with desired frequence as its associated window/view
            //! is expecting.
            //! Note: the RenderPipeline will be only renderred once if this function is called multiple
            //! time between two system ticks.
            void AddToRenderTickOnce();

            //! Add this RenderPipeline to RPI system's RenderTick and it will be rendered whenever
            //! the RPI system's RenderTick is called.
            //! The RenderPipeline is rendered per RenderTick by default unless AddToRenderTickOnce() was called.
            void AddToRenderTick();

            //! Disable render for this RenderPipeline
            void RemoveFromRenderTick();

            ~RenderPipeline();
                        
            enum class RenderMode : uint8_t
            {
                RenderEveryTick, // Render at each RPI system render tick
                RenderOnce, // Render once in next RPI system render tick
                NoRender // Render disabled.
            };

            //! Get current render mode
            RenderMode GetRenderMode() const;

            //! Get draw filter tag
            RHI::DrawFilterTag GetDrawFilterTag() const;

            //! Get draw filter mask
            RHI::DrawFilterMask GetDrawFilterMask() const;

        private:
            RenderPipeline() = default;

            static void InitializeRenderPipeline(RenderPipeline* pipeline, const RenderPipelineDescriptor& desc);

            // Collect DrawListTags from passes that are using specified pipeline view
            void CollectDrawListMaskForViews(PipelineViews& views);

            // Build pipeline views from the pipeline pass tree. It's usually called when pass tree changed.
            void BuildPipelineViews();

            //////////////////////////////////////////////////
            // Functions accessed by Scene class
            
            void OnAddedToScene(Scene* scene);
            void OnRemovedFromScene(Scene* scene);

            // Called when this pipeline is about to be rendered
            void OnStartFrame(const TickTimeInfo& tick);

            // Called when the rendering of current frame is finished.
            void OnFrameEnd();

            // Find all the persistent views in this pipeline and add them and their DrawListMasks to the output map.
            // if the view already exists in map, its DrawListMask will be combined to the existing one's
            void CollectPersistentViews(AZStd::map<ViewPtr, RHI::DrawListMask>& outViewMasks) const;

            void SetDrawFilterTag(RHI::DrawFilterTag);

            // End of functions accessed by Scene class
            //////////////////////////////////////////////////


            RenderMode m_renderMode = RenderMode::RenderEveryTick;

            // The Scene this pipeline was added to.
            Scene* m_scene = nullptr;

            // Pass tree which contains all the passes in this render pipeline.
            Ptr<ParentPass> m_rootPass;
            
            PipelineViewMap m_pipelineViewsByTag;
            
            // RenderPipeline's name id, it will be used to identify the render pipeline when it's added to a Scene
            RenderPipelineId m_nameId;
            
            // Whether the pass tree was modified. It's used to trigger rebuild pipeline views when frame starts
            bool m_wasPassModified = false;

            // Whether the pipeline should recreate it's pass tree, for example in the case of pass asset hot reloading.
            bool m_needsPassRecreate = false;

            PipelineViewTag m_mainViewTag;

            // Whether the pipeline should be removed after one execution
            bool m_executeOnce = false;

            // The window handle associated with this render pipeline if it's created for a window
            AzFramework::NativeWindowHandle m_windowHandle = nullptr;

            // Render settings that can be queried by passes to setup things like render target resolution
            PipelineRenderSettings m_activeRenderSettings;

            // Original settings from RenderPipelineDescriptor, used to revert active render settings to original settings from RenderPipelineDescriptor
            PipelineRenderSettings m_originalRenderSettings;

            // A tag to filter draw items submitted by passes of this render pipeline.
            // This tag is allocated when it's added to a scene. It's set to invalid when it's removed to the scene.
            RHI::DrawFilterTag m_drawFilterTag;
            // A mask to filter draw items submitted by passes of this render pipeline.
            // This mask is created from the value of m_drawFilterTag.
            RHI::DrawFilterMask m_drawFilterMask = 0; 
        };

    } // namespace RPI
} // namespace AZ
