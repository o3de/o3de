/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/Pass.h>

namespace AZ
{
    namespace RPI
    {
        class SwapChainPass;

        using SortedPipelineViewTags = AZStd::set<PipelineViewTag, AZNameSortAscending>;

        //! A parent pass doesn't do any rendering itself, but instead contains child passes that it delegates functionality to.
        //! A child can be a RenderPass or it can be a ParentPass itself. This creates a pass tree hierarchy that defines the
        //! the order in which passes and their logic are executed in.
        class ParentPass
            : public Pass
        {
            AZ_RPI_PASS(ParentPass);

            friend class Pass;

        public:
            using ChildPassIndex = RHI::Handle<uint32_t, class ChildPass>;

            AZ_RTTI(ParentPass, "{0801AD74-85A8-4895-A5E5-C500AEE535A6}", Pass);
            AZ_CLASS_ALLOCATOR(ParentPass, SystemAllocator, 0);

            virtual ~ParentPass();

            //! Creates a new pass without a PassTemplate
            static Ptr<ParentPass> Create(const PassDescriptor& descriptor);

            //! Creates a new parent pass reusing the same parameters used to create this pass
            //! This is used in scenarios like hot reloading where some of the templates in the pass library might have changed.
            virtual Ptr<ParentPass> Recreate() const;

            //! Recursively collects all different view tags from this pass's children 
            void GetPipelineViewTags(SortedPipelineViewTags& outTags) const override;

            //! Recursively searches children for given viewTag, and collects their DrawListTags in outDrawListMask.
            void GetViewDrawListInfo(RHI::DrawListMask& outDrawListMask, PassesByDrawList& outPassesByDrawList, const PipelineViewTag& viewTag) const override;

            //! Set render pipeline this pass belongs to recursively
            void SetRenderPipeline(RenderPipeline* pipeline) override;

            //! Return owning attachment
            Ptr<PassAttachment> GetOwnedAttachment(const Name& attachmentName) const;

            // --- Children related functions ---

            //! Adds pass to list of children
            void AddChild(const Ptr<Pass>& child);

            //! Searches for a child pass with the given name. Returns the child's index if found, null index otherwise
            ChildPassIndex FindChildPassIndex(const Name& passName) const;

            //! Find a child pass with a matching name and returns it. Return nullptr if none found.
            Ptr<Pass> FindChildPass(const Name& passName) const;
            
            template<typename PassType>
            Ptr<PassType> FindChildPass() const;

            //! Gets the list of children. Useful for validating hierarchies
            AZStd::array_view<Ptr<Pass>> GetChildren() const;

            //! Searches the tree for the first pass that uses the given DrawListTag.
            const Pass* FindPass(RHI::DrawListTag drawListTag) const;

            //! Recursively set all the Timestamp queries enabled for all its children.
            void SetTimestampQueryEnabled(bool enable) override;

            //! Recursively set all the PipelineStatistics queries enabled for all its children.
            void SetPipelineStatisticsQueryEnabled(bool enable) override;

            // --- Validation & Debug ---

            void Validate(PassValidationResults& validationResults) override;

            //! Prints the pass and all of it's children
            void DebugPrint() const override;

        protected:
            explicit ParentPass(const PassDescriptor& descriptor);

            // --- Protected Members ---

            // List of child passes. Children can themselves have children.
            // This creates a tree hierarchy for coherent traversal of all the passes.
            AZStd::vector< Ptr<Pass> > m_children;

            // Creates any child passes this pass may require.
            // For example a Bloom pass may create several downsample child passes.
            // Called when queued for creation with the PassSystem.
            void CreateChildPasses();
            virtual void CreateChildPassesInternal() { }

            // --- Pass Behaviour Overrides ---

            void ResetInternal() override;
            void BuildInternal() override;
            void OnInitializationFinishedInternal() override;
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void FrameEndInternal() override;

            // Finds the pass in m_children and removes it
            void RemoveChild(Ptr<Pass> pass);

            // Orphans all children by clearing m_children.
            void RemoveChildren();

        private:
            // RPI::Pass overrides...
            PipelineStatisticsResult GetPipelineStatisticsResultInternal() const override;

            // --- Hierarchy related functions ---

            // Called when the pass gets a new spot in the pass hierarchy
            void OnHierarchyChange() override;

            // Called when the pass is removed from it's parent list of children
            void OnOrphan() override;

            // Generates child passes from source PassTemplate
            void CreatePassesFromTemplate();
        };

        template<typename PassType>
        inline Ptr<PassType> ParentPass::FindChildPass() const
        {
            for (const Ptr<Pass>& child : m_children)
            {
                PassType* pass = azrtti_cast<PassType*>(child.get());
                if (pass)
                {
                    return pass;
                }
            }
            return {};
        }

    }   // namespace RPI
}   // namespace AZ
