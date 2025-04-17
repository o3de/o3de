/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/DrawList.h>
#include <Atom/RHI/DrawFilterTagRegistry.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/PipelinePassChanges.h>
#include <Atom/RPI.Public/Pass/PassTree.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

#include <Atom/RPI.Reflect/Pass/PassAsset.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>
#include <Atom/RPI.Public/ViewProviderBus.h>
#include <Atom/RPI.Public/WindowContext.h>

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
        class ShaderResourceGroup;
        class AnyAsset;

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

        //! Points to a pass binding for global access through the pipeline using a name for reference
        struct PipelineGlobalBinding
        {
            // The name used to reference this binding in a global manner.
            Name m_globalName;

            // The referenced binding
            PassAttachmentBinding* m_binding;

            // The pass that owns the binding. Used to remove the global binding from the list when the pass is orphaned
            Pass* m_pass;
        };

        //! RenderPipeline describes how to render a scene. It has all the passes and views for rendering.
        //! A scene may have several pipelines. Each pipeline have its own render frequency. 
        //! Pipeline can be disabled and it won't be rendered if it's disabled.
        class ATOM_RPI_PUBLIC_API RenderPipeline
        {
            friend class Pass;
            friend class PassSystem;
            friend class Scene;

        public:
            AZ_CLASS_ALLOCATOR(RenderPipeline, AZ::SystemAllocator);

            static RenderPipelinePtr CreateRenderPipeline(const RenderPipelineDescriptor& desc);

            static RenderPipelinePtr CreateRenderPipelineFromAsset(Data::Asset<AnyAsset> pipelineAsset);

            static RenderPipelinePtr CreateRenderPipelineForWindow(const RenderPipelineDescriptor& desc, const WindowContext& windowContext,
                                                                   const ViewType viewType = ViewType::Default);
            static RenderPipelinePtr CreateRenderPipelineForWindow(Data::Asset<AnyAsset> pipelineAsset, const WindowContext& windowContext);

            // Anti-aliasing
            bool SetActiveAAMethod(AZStd::string aaMethodName);
            static AntiAliasingMode GetAAMethodByName(AZStd::string aaMethodName);
            static AZStd::string GetAAMethodNameByIndex(AntiAliasingMode aaMethodIndex);            
            AntiAliasingMode GetActiveAAMethod();

            //! Create a render pipeline which renders to the specified attachment image
            //! The render pipeline's root pass is created from the pass template specified from RenderPipelineDescriptor::m_rootPassTemplate
            //! The input AttachmentImageAsset is used to connect to first output attachment of the root pass template
            //! Note: the AttachmentImageAsset doesn't need to be loaded
            static RenderPipelinePtr CreateRenderPipelineForImage(const RenderPipelineDescriptor& desc, Data::Asset<AttachmentImageAsset> imageAsset);

            // Data type for render pipeline's views' information
            using PipelineViewMap = AZStd::unordered_map<PipelineViewTag, PipelineViews>;
            using ViewToViewTagMap = AZStd::map<const View*, PipelineViewTag>;

            // Removes a registered view from the pipeline, either transient or persistent
            // This is only needed if you want to re-register a view with another viewtag
            void UnregisterView(ViewPtr view);

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

            //! Set a stereoscopic view to the default view tag.
            //! It's the same as SetPersistentView(GetMainViewTag(), view)
            void SetDefaultStereoscopicViewFromEntity(EntityId entityId, RPI::ViewType viewType);

            //! Get the view for the default view tag. 
            //! It's the same as GetViews(GetMainViewTag()) and using first element.
            ViewPtr GetDefaultView();

            //! Get the frist view for the view tag.
            //! It's the same as GetViews("tag") and using first element.
            ViewPtr GetFirstView(const PipelineViewTag& viewTag);

            //! Set default view from an entity which should have a ViewProvider handler.
            void SetDefaultViewFromEntity(EntityId entityId);

            //! Check if this pipeline has the specified PipelineViewTag.
            bool HasViewTag(const PipelineViewTag& viewTag) const;

            //! Get the main view tag (the tag used for the default view).
            const PipelineViewTag& GetMainViewTag() const;

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

            //! Returns the flags indicating the pipeline pass changes that occured this past frame
            u32 GetPipelinePassChanges() const { return m_pipelinePassChanges; }

            //! Processes passes in the pipeline that are queued for build, initialization or removal
            void ProcessQueuedPassChanges();

            //! This function signals the render pipeline that modifications have been made to the pipeline passes
            void MarkPipelinePassChanges(u32 passChangeFlags);
            
            //! Update passes and views that are affected by any modifed passes. Called at the start of each frame.
            void UpdatePasses();

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
            //! This function can be used for render a render pipeline with desired frequency as its associated window/view
            //! is expecting.
            //! Note: the RenderPipeline will be only rendered once if this function is called multiple
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

            //! Get draw filter mask
            RHI::DrawFilterMask GetDrawFilterMask() const;

            //! Get the RenderPipelineDescriptor which was used to create this RenderPipeline
            const RenderPipelineDescriptor& GetDescriptor() const;

            // Helper functions to modify the passes of this render pipeline
            //! Find a reference pass's location and add the new pass before the reference pass
            //! After the new pass was inserted, the new pass and the reference pass are siblings
            bool AddPassBefore(Ptr<Pass> newPass, const AZ::Name& referencePassName);
            
            //! Find a reference pass's location and add the new pass after the reference pass
            //! After the new pass was inserted, the new pass and the reference pass are siblings
            bool AddPassAfter(Ptr<Pass> newPass, const AZ::Name& referencePassName);

            //! Find the first pass with matching name in the render pipeline
            //! Note: to find all the passes with matching name in this render pipeline,
            //! use RPI::PassSystemInterface::Get()->ForEachPass() function instead.
            Ptr<Pass> FindFirstPass(const AZ::Name& passName);

            //! Return the view type associated with this pipeline.
            ViewType GetViewType() const;

            //! Update viewport and scissor based on pass tree's output
            void UpdateViewportScissor();

            //! Return true if the pipeline allows merging of passes as subpasses.
            bool SubpassMergingSupported() const;

        private:
            RenderPipeline() = default;

            // Adds a pass connection to the list of pipeline connections so it can be reference in a global way
            // Should be called during the pass build phase
            void AddPipelineGlobalConnection(const Name& globalName, PassAttachmentBinding* binding, Pass* pass);

            // Removes all pipeline global connections associated with a specific pass
            void RemovePipelineGlobalConnectionsFromPass(Pass* passOnwer);

            // Retrieves a previously added pipeline global connection via name
            const PipelineGlobalBinding* GetPipelineGlobalConnection(const Name& globalName) const;

            // Checks if the view is already registered with a different viewTag
            bool CanRegisterView(const PipelineViewTag& allowedViewTag, const View* view) const;

            // Removes a registered view from the pipeline
            void RemoveTransientView(const PipelineViewTag viewId, ViewPtr view);
            void ResetPersistentView(const PipelineViewTag viewId, ViewPtr view);

            // Clears the lists of global attachments and binding that passes use to reference attachments in a global manner
            // This is called from the pipeline root pass during the pass reset phase
            void ClearGlobalBindings();

            static void InitializeRenderPipeline(RenderPipeline* pipeline, const RenderPipelineDescriptor& desc);

            // Collect DrawListTags from passes that are using specified pipeline view
            void CollectDrawListMaskForViews(PipelineViews& views);

            // Build pipeline views from the pipeline pass tree. It's usually called when pass tree changed.
            void BuildPipelineViews();

            // Called by Pass System at the start of rendering the frame
            void PassSystemFrameBegin(Pass::FramePrepareParams params);

            // Called by Pass System at the end of rendering the frame
            void PassSystemFrameEnd();

            //////////////////////////////////////////////////
            // Functions accessed by Scene class
            
            void OnAddedToScene(Scene* scene);
            void OnRemovedFromScene(Scene* scene);

            // Called when this pipeline is about to be rendered
            void OnStartFrame(float time);

            // Called when the rendering of current frame is finished.
            void OnFrameEnd();

            // Find all the persistent views in this pipeline and add them and their DrawListMasks to the output map.
            // if the view already exists in map, its DrawListMask will be combined to the existing one's
            void CollectPersistentViews(AZStd::map<ViewPtr, RHI::DrawListMask>& outViewMasks) const;

            void SetDrawFilterTags(RHI::DrawFilterTagRegistry* tagRegistry);
            void ReleaseDrawFilterTags(RHI::DrawFilterTagRegistry* tagRegistry);

            // AA method
            static bool SetAAMethod(RenderPipeline* pipeline, AZStd::string aaMethodName);
            static bool SetAAMethod(RenderPipeline* pipeline, AntiAliasingMode aaMethod);
            static bool EnablePass(RenderPipeline* pipeline, Name& passName, bool enable);

            // End of functions accessed by Scene class
            //////////////////////////////////////////////////


            RenderMode m_renderMode = RenderMode::RenderEveryTick;

            // The Scene this pipeline was added to.
            Scene* m_scene = nullptr;

            // Holds the passes belonging to the pipeline
            PassTree m_passTree;

            // Attachment bindings/connections that can be referenced from any pass in the pipeline in a global manner
            AZStd::vector<PipelineGlobalBinding> m_pipelineGlobalConnections;

            PipelineViewMap m_pipelineViewsByTag;
            ViewToViewTagMap m_persistentViewsByViewTag;
            ViewToViewTagMap m_transientViewsByViewTag;

            // RenderPipeline's name id, it will be used to identify the render pipeline when it's added to a Scene
            RenderPipelineId m_nameId;

            // The name of a material pipeline (.materialpipeline file) that this RenderPipeline is associated with.
            Name m_materialPipelineTagName;

            // Whether the pipeline should recreate it's pass tree, for example in the case of pass asset hot reloading.
            bool m_needsPassRecreate = false;

            // Set of flags to track what changes have been made to the pipeline's passes
            u32 m_pipelinePassChanges = PipelinePassChanges::NoPassChanges;

            PipelineViewTag m_mainViewTag;

            // Was the pipeline modified by Scene's feature processor
            bool m_wasModifiedByScene = false;

            // The window handle associated with this render pipeline if it's created for a window
            AzFramework::NativeWindowHandle m_windowHandle = nullptr;

            // Render settings that can be queried by passes to setup things like render target resolution
            PipelineRenderSettings m_activeRenderSettings;

            // Tags to filter draw items submitted by passes of this render pipeline.
            // These tags are allocated when the pipeline is added to a scene. They are set to invalid when removed from the scene.
            RHI::DrawFilterTag m_drawFilterTagForPipelineInstanceName;
            RHI::DrawFilterTag m_drawFilterTagForMaterialPipeline;

            // A mask to filter draw items submitted by passes of this render pipeline.
            // This mask is created from the above DrawFilterTag(s).
            RHI::DrawFilterMask m_drawFilterMask = 0;

            // The descriptor used to created this render pipeline
            RenderPipelineDescriptor m_descriptor;

            AntiAliasingMode m_activeAAMethod = AntiAliasingMode::MSAA;

            // View type associated with the Render Pipeline.
            ViewType m_viewType = ViewType::Default;

            // viewport and scissor for frame update
            RHI::Viewport m_viewport;
            RHI::Scissor m_scissor;

            // Supports merging of passes as subpasses.
            bool m_allowSubpassMerging = false;
        };

    } // namespace RPI
} // namespace AZ
