/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/GpuQuery/GpuQuerySystemInterface.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Pass/PassAttachment.h>
#include <Atom/RPI.Public/Pass/PassDefines.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#include <Atom/RPI.Reflect/Pass/PassAsset.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>

#include <Atom/RHI/DrawList.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>

#include <AtomCore/std/containers/array_view.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#define AZ_RPI_PASS(PASS_NAME)                                                      \
    friend class PassFactory;                                                       \
    friend class PassLibrary;                                                       \
    friend class PassSystem;                                                        \
    friend class PassFactory;                                                       \
    friend class ParentPass;                                                        \
    friend class RenderPipeline;                                                    \
    friend class UnitTest::PassTests;                                               \

namespace UnitTest
{
    class PassTests;
}

namespace AZ
{
    namespace RHI
    {
        class FrameGraphBuilder;
        class FrameGraphAttachmentInterface;
    }

    namespace RPI
    {
        class Pass;
        class PassTemplate;
        struct PassRequest;
        struct PassValidationResults;
        class AttachmentReadback;

        using SortedPipelineViewTags = AZStd::set<PipelineViewTag, AZNameSortAscending>;
        using PassesByDrawList = AZStd::map<RHI::DrawListTag, const Pass*>;

        const uint32_t PassAttachmentBindingCountMax = 32;
        const uint32_t PassInputBindingCountMax = 16;
        const uint32_t PassInputOutputBindingCountMax = PassInputBindingCountMax;
        const uint32_t PassOutputBindingCountMax = PassInputBindingCountMax;
                
        enum class PassAttachmentReadbackOption : uint8_t
        {
            Input = 0,
            Output
        };

        //! Atom's base pass class (every pass class in Atom must derive from this class).
        //! 
        //! Passes are organized into a tree hierarchy with the derived ParentPass class.
        //! The root of the entire hierarchy is owned by the PassSystem.
        //! 
        //! When authoring a new pass class, inherit from Pass and override any of the virtual functions
        //! ending with 'Internal' to define the behavior of your passes. These virtual are recursively
        //! called in preorder traversal throughout the pass tree. Only FrameBegin and FrameEnd are
        //! guaranteed to be called per frame. The other override-able functions are called as needed 
        //! when scheduled with the PassSystem. See QueueForBuildAndInitialization, QueueForRemoval and QueueForInitialization.
        //! 
        //! Passes are created by the PassFactory. They can be created using either Pass Name,
        //! a PassTemplate, or a PassRequest. To register your pass class with the PassFactory,
        //! you'll need to write a static create method (see ParentPass and RenderPass for examples)
        //! and register this create method with the PassFactory.
        class Pass
            : public AZStd::intrusive_base
        {
            AZ_RPI_PASS(Pass);

        public:
            using ChildPassIndex = RHI::Handle<uint32_t, class ChildPass>;

            // Input parameters for Prepare
            struct FramePrepareParams
            {
                FramePrepareParams(RHI::FrameGraphBuilder* frameGraphBuilder = nullptr)
                    : m_frameGraphBuilder(frameGraphBuilder)
                {}

                RHI::FrameGraphBuilder* m_frameGraphBuilder = nullptr;

                // This should include SRGs that are higher level than
                // the Pass, like per-frame and per-scene SRGs.
                const ShaderResourceGroupList* m_shaderResourceGroupsToBind = nullptr;

                RHI::Scissor m_scissorState;
                RHI::Viewport m_viewportState;
            };

            AZ_RTTI(Pass, "{EA34FF66-631D-433B-B449-71F5647E7BB5}", AZStd::intrusive_base);
            AZ_CLASS_ALLOCATOR(Pass, SystemAllocator, 0);

            virtual ~Pass();

            // --- Simple getters/setters ---

            //! Returns the name of the pass (example: Bloom)
            const Name& GetName() const;

            //! Return the path name of the pass (example: Root.SwapChain.Bloom)
            const Name& GetPathName() const;

            //! Returns the depth of this pass in the tree hierarchy (Root depth is 0)
            uint32_t GetTreeDepth() const;

            //! Returns the number of input attachment bindings
            uint32_t GetInputCount() const;

            //! Returns the number of input/output attachment bindings
            uint32_t GetInputOutputCount() const;

            //! Returns the number of output attachment bindings
            uint32_t GetOutputCount() const;

            //! Returns the pass template which was used for create this pass.
            //! It may return nullptr if the pass wasn't create from a template
            const PassTemplate* GetPassTemplate() const;

            //! Enable/disable this pass
            //! If the pass is disabled, it (and any children if it's a ParentPass) won't be rendered.  
            void SetEnabled(bool enabled);
            virtual bool IsEnabled() const;

            bool HasDrawListTag() const;
            bool HasPipelineViewTag() const;

            //! Return the set of attachment bindings
            PassAttachmentBindingListView GetAttachmentBindings() const;

            //! Casts the pass to a parent pass if valid, else returns nullptr
            ParentPass* AsParent();
            const ParentPass* AsParent() const;

            // --- Utility functions ---
            
            //! Queues the pass to have Build() and Initialize() called by the PassSystem on frame update 
            void QueueForBuildAndInitialization();

            //! Queues the pass to have RemoveFromParent() called by the PassSystem on frame update
            void QueueForRemoval();

            //! Queues the pass to have Initialize() called by the PassSystem on frame update 
            void QueueForInitialization();

            //! Adds an attachment binding to the list of this Pass' attachment bindings
            void AddAttachmentBinding(PassAttachmentBinding attachmentBinding);

            // Returns a reference to the N-th input binding, where N is the index passed to the function
            PassAttachmentBinding& GetInputBinding(uint32_t index);

            // Returns a reference to the N-th input/output binding, where N is the index passed to the function
            PassAttachmentBinding& GetInputOutputBinding(uint32_t index);

            // Returns a reference to the N-th output binding, where N is the index passed to the function
            PassAttachmentBinding& GetOutputBinding(uint32_t index);

            //! Attach an external buffer resource as attachment to specified slot
            //! The buffer will be added as a pass attachment then attach to the pass slot
            //! Note: the pass attachment and binding will be removed after the general Build call.
            //!       you can add this call in pass' BuildInternal so it will be added whenever attachments get rebuilt
            void AttachBufferToSlot(AZStd::string_view slot, Data::Instance<Buffer> buffer);
            void AttachBufferToSlot(const Name& slot, Data::Instance<Buffer> buffer);
            void AttachImageToSlot(const Name& slot, Data::Instance<AttachmentImage> image);

            // --- Virtual functions which may need to be override by derived classes ---

            //! Collect all different view tags from this pass 
            virtual void GetPipelineViewTags(SortedPipelineViewTags& outTags) const;

            //! Adds this pass' DrawListTags to the outDrawListMask.
            virtual void GetViewDrawListInfo(RHI::DrawListMask& outDrawListMask, PassesByDrawList& outPassesByDrawList, const PipelineViewTag& viewTag) const;

            //! Check if the pass has a DrawListTag. Pass' DrawListTag can be used to filter draw items.
            virtual RHI::DrawListTag GetDrawListTag() const;

            //! Function used by views to sort draw lists. Can be overridden so passes can provide custom sort functionality.
            virtual void SortDrawList(RHI::DrawList& drawList) const;

            //! Check if the pass is associated to a view. If pass has a pipeline view tag, the rpi view assigned to this view tag will have pass's draw list tag.
            virtual const PipelineViewTag& GetPipelineViewTag() const;

            //! Set render pipeline this pass belongs to
            virtual void SetRenderPipeline(RenderPipeline* pipeline);
            RenderPipeline* GetRenderPipeline() const;

            Scene* GetScene() const;

            //! Validates entire tree hierarchy (ensures passes have valid state and attachments).
            //! Functionality compiled out if AZ_RPI_ENABLE_PASS_VALIDATION is not defined.
            virtual void Validate(PassValidationResults& validationResults);

            // --- Debug and validation print functions ---

            //! Prints the pass
            virtual void DebugPrint() const;

            //! Return the latest Timestamp result of this pass
            TimestampResult GetLatestTimestampResult() const;

            //! Return the latest PipelineStatistic result of this pass
            PipelineStatisticsResult GetLatestPipelineStatisticsResult() const;

            //! Enables/Disables Timestamp queries for this pass
            virtual void SetTimestampQueryEnabled(bool enable);

            //! Enables/Disables PipelineStatistics queries for this pass
            virtual void SetPipelineStatisticsQueryEnabled(bool enable);

            //! Readback an attachment attached to the specified slot name
            //! @param readback The AttachmentReadback object which is used for readback. Its callback function will be called when readback is finished.
            //! @param slotName The attachment bind to the slot with this slotName is to be readback
            //! @param option The option is used for choosing input or output state when readback an InputOutput attachment.
            //!               It's ignored if the attachment isn't an InputOutput attachment.
            //! Return true if the readback request was successful. User may expect the AttachmentReadback's callback function would be called. 
            bool ReadbackAttachment(AZStd::shared_ptr<AttachmentReadback> readback, const Name& slotName, PassAttachmentReadbackOption option = PassAttachmentReadbackOption::Output);

            //! Returns whether the Timestamp queries is enabled/disabled for this pass
            bool IsTimestampQueryEnabled() const;

            //! Returns whether the PipelineStatistics queries is enabled/disabled for this pass
            bool IsPipelineStatisticsQueryEnabled() const;

            //! Helper function to print spaces to indent the pass
            void PrintIndent(AZStd::string& stringOutput, uint32_t indent) const;

            //! Prints the name of the pass
            void PrintPassName(AZStd::string& stringOutput, uint32_t indent = 0) const;

            //! Prints the attachment binding at the given index
            void DebugPrintBinding(AZStd::string& stringOutput, const PassAttachmentBinding& binding) const;

            //! Prints the attachment binding at the given index and its connection
            void DebugPrintBindingAndConnection(AZStd::string& stringOutput, uint8_t bindingIndex) const;

            //! Prints the pass name and all the errors accumulated during build and setup
            void PrintErrors() const;

            //! Prints the pass name and all the warnings accumulated during build and setup 
            void PrintWarnings() const;

            //! Helper function to print an array of messages (like errors or warnings) for a pass
            void PrintMessages(const AZStd::vector<AZStd::string>& messages) const;

            //! Prints the pass and all the list of inputs and input/outputs that are missing an attachment
            void PrintBindingsWithoutAttachments(uint32_t slotTypeMask) const;

            //! Returns pointer to the parent pass
            ParentPass* GetParent() const;

            PassState GetPassState() const;

            // Update all bindings on this pass that are connected to bindings on other passes
            void UpdateConnectedBindings();

            // Update input and input/output bindings on this pass that are connected to bindings on other passes
            void UpdateConnectedInputBindings();

            // Update output bindings on this pass that are connected to bindings on other passes
            void UpdateConnectedOutputBindings();

        protected:
            explicit Pass(const PassDescriptor& descriptor);

            // Creates a pass descriptor for creating a duplicate pass. Used for hot reloading.
            PassDescriptor GetPassDescriptor() const;

            // Imports owned imported attachments into the FrameGraph
            // Called in pass's frame prepare function
            void ImportAttachments(RHI::FrameGraphAttachmentInterface attachmentDatabase);

            // --- Find functions ---

            // Searches for an adjacent pass with the given Name. An adjacent pass is either:
            // a parent pass, a child pass, a sibling pass or the pass itself (this).
            // Special names: "This" will return this, and "Parent" will return the parent pass.
            // Search order: 1.This -> 2.Parent -> 3.Siblings -> 4.Children
            Ptr<Pass> FindAdjacentPass(const Name& passName);

            // Searches this pass's attachment bindings for one with the provided Name (nullptr if none found)
            PassAttachmentBinding* FindAttachmentBinding(const Name& slotName);

            // Searches this pass's attachment bindings for one with the provided Name (nullptr if none found)
            const PassAttachmentBinding* FindAttachmentBinding(const Name& slotName) const;

            // Searches the attachments owned by this pass using the provided Name (nullptr if none found)
            Ptr<PassAttachment> FindOwnedAttachment(const Name& attachmentName) const;

            // Find an attachment with a matching name from either inputs, outputs or inputOutputs and returns it.
            // Returns nullptr if no attachment found.
            Ptr<PassAttachment> FindAttachment(const Name& slotName) const;

            // Searches adjacent passes for an attachment binding matching the PassAttachmentRef. An adjacent pass is either:
            // a parent pass, a child pass, a sibling pass or the pass itself (this).
            const PassAttachmentBinding* FindAdjacentBinding(const PassAttachmentRef& attachmentRef);

            // --- Template related setup ---

            // Process a PassConnection to connect two PassAttachmentBindings
            void ProcessConnection(const PassConnection& connection, uint32_t slotTypeMask = 0xFFFFFFFF);

            // --- Validation and Error Functions ---

            void LogError(AZStd::string&& message);
            void LogWarning(AZStd::string&& message);

            // --- Pass Behavior Functions ---

            // These functions come in pairs, one virtual function (name ending in -Internal) and one
            // non-virtual. Each non-virtual function will call it's virtual counterpart. Classes 
            // inheriting from Pass can override any or all of the virtual -Internal functions to 
            // customize it's behavior, hence why these functions are called the pass behavior functions.

            // Resets everything in the pass (like Attachments).
            // Called from PassSystem when pass is QueueForBuildAndInitialization.
            void Reset();
            virtual void ResetInternal() { }

            // Builds and sets up any attachments and input/output connections the pass needs.
            // Called from PassSystem when pass is QueueForBuildAndInitialization.
            void Build(bool calledFromPassSystem = false);
            virtual void BuildInternal() { }

            // Allows for additional pass initialization between building and rendering
            // Can be queued independently of Build so as to only invoke Initialize() without Build()
            void Initialize();
            virtual void InitializeInternal() { };

            // Called after the pass initialization phase has finished. Allows passes to reset various states and flags.
            void OnInitializationFinished();
            virtual void OnInitializationFinishedInternal() { };

            // The Pass's 'Render' function. Called every frame, here the pass sets up it's rendering logic with
            // the FrameGraphBuilder. This is where your derived pass needs to call ImportScopeProducer on
            // the FrameGraphBuilder if it's a ScopeProducer (see ForwardPass::FrameBeginInternal for example).
            void FrameBegin(FramePrepareParams params);
            virtual void FrameBeginInternal([[maybe_unused]] FramePrepareParams params) { }

            // Called every frame after the frame has been rendered. Allows the pass
            // to perform any post-frame cleanup, such as resetting per-frame state.            
            void FrameEnd();
            virtual void FrameEndInternal() { }

            void UpdateReadbackAttachment(FramePrepareParams params, bool beforeAddScopes);

            // --- Protected Members ---

            const Name PassNameThis{"This"};
            const Name PassNameParent{"Parent"};
            const Name PipelineKeyword{"Pipeline"};

            // List of input, output and input/output attachment bindings
            // Fixed size for performance and so we can hold pointers to the bindings for connections
            AZStd::fixed_vector<PassAttachmentBinding, PassAttachmentBindingCountMax> m_attachmentBindings;
            
            // List of attachments owned by this pass.
            // It includes both transient attachments and imported attachments
            AZStd::vector<Ptr<PassAttachment>> m_ownedAttachments;

            // List of passes before which this pass needs to execute (specified by the PassRequest)
            // Note most pass ordering is determined by attachments. This is only to be used for
            // dependencies between passes that don't have any attachments/connections in common.
            AZStd::vector<Pass*> m_executeBeforePasses;

            // List of passes that this pass needs to execute after (specified by the PassRequest)
            // Note most pass ordering is determined by attachments. This is only to be used for
            // dependencies between passes that don't have any attachments/connections in common.
            AZStd::vector<Pass*> m_executeAfterPasses;

            // The render pipeline this pass belongs to.
            RenderPipeline* m_pipeline = nullptr;

            // The PassTemplate used to create this pass
            // Null if this pass was not created by a PassTemplate
            AZStd::shared_ptr<PassTemplate> m_template = nullptr;

            // The PassRequest used to create this pass
            // Only valid if m_createdByPassRequest flag is set
            PassRequest m_request;

            // Pointer to the parent pass if this pass is a child pass
            ParentPass* m_parent = nullptr;

            struct
            {
                union
                {
                    struct
                    {
                        // Whether this pass was created with a PassRequest (in which case m_request holds valid data)
                        uint64_t m_createdByPassRequest : 1;

                        // Whether the pass is enabled (behavior can be customized by overriding IsEnabled() )
                        uint64_t m_enabled : 1;

                        // False if parent or one of it's ancestors is disabled
                        uint64_t m_parentEnabled : 1;

                        // If this is a parent pass, indicates if the pass has already created children this frame
                        // Prevents ParentPass::CreateChildPasses from executing multiple times in the same pass 
                        uint64_t m_alreadyCreatedChildren : 1;

                        // If this is a parent pass, indicates whether the pass needs to create child passes
                        uint64_t m_createChildren : 1;

                        // Whether this pass belongs to the pass hierarchy, i.e. if you can trace it's parents up to the Root pass
                        uint64_t m_partOfHierarchy : 1;

                        // Whether this pass has a DrawListTag
                        uint64_t m_hasDrawListTag : 1;

                        // Whether this pass has a PipelineViewTag
                        uint64_t m_hasPipelineViewTag : 1;

                        // Whether the pass should gather timestamp query metrics
                        uint64_t m_timestampQueryEnabled : 1;

                        // Whether the pass should gather pipeline statics
                        uint64_t m_pipelineStatisticsQueryEnabled : 1;
                    };
                    uint64_t m_allFlags = 0;
                };
            } m_flags;

            // Arbitrary message log limit so we don't get an
            // ever increasing array when an error starts spamming
            static const size_t MessageLogLimit = 256;

            AZStd::vector<AZStd::string> m_errorMessages;
            AZStd::vector<AZStd::string> m_warningMessages;

            uint32_t m_errors = 0;
            uint32_t m_warnings = 0;

            // Sort type to be used by the default sort implementation. Passes can also provide
            // fully custom sort implementations by overriding the SortDrawList() function.
            RHI::DrawListSortType m_drawListSortType = RHI::DrawListSortType::KeyThenDepth;
            
            // For read back attachment
            AZStd::shared_ptr<AttachmentReadback> m_attachmentReadback;
            PassAttachmentReadbackOption m_readbackOption;

        private:
            // Return the Timestamp result of this pass
            virtual TimestampResult GetTimestampResultInternal() const;

            // Return the PipelineStatistics result of this pass
            virtual PipelineStatisticsResult GetPipelineStatisticsResultInternal() const;

            // Used to maintain references to imported attachments so they're underlying
            // buffers and images don't get deleted during attachment build phase
            void StoreImportedAttachmentReferences();

            // Used by the RenderPipeline to create it's passes immediately instead of waiting on
            // the next Pass System update. The function internally build and initializes the pass.
            void ManualPipelineBuildAndInitialize();

            // --- Hierarchy related functions ---

            // Called when the pass gets a new spot in the pass hierarchy
            virtual void OnHierarchyChange();

            // Called when the pass is removed from it's parent list of children
            virtual void OnOrphan();

            // The pass removes itself from its parent.
            void RemoveFromParent();

            // --- Template related setup ---

            // Generates bindings from source PassTemplate
            void CreateBindingsFromTemplate();

            // Generates attachments from source PassTemplate
            void CreateAttachmentsFromTemplate();

            // Generates attachments from source PassRequest
            void CreateAttachmentsFromRequest();

            // Uses FrameGraphAttachmentInterface to create transient attachments for the pass
            void CreateTransientAttachments(RHI::FrameGraphAttachmentInterface attachmentDatabase);

            // Creates an attachment from a given description and returns a pointer to it
            template<typename AttachmentDescType>
            Ptr<PassAttachment> CreateAttachmentFromDesc(const AttachmentDescType& desc);

            // Overrides an existing attachment if matching name is found, otherwise creates and adds new attachment
            template<typename AttachmentDescType>
            void OverrideOrAddAttachment(const AttachmentDescType& desc);

            // Updates a binding on this pass using the binding it is connected to.
            // This sets the binding's attachment pointer to the connected binding's attachment
            void UpdateConnectedBinding(PassAttachmentBinding& binding);

            // Process a PassFallbackConnection to connect an output to an input to act as a short-circuit for when Pass is disabled 
            void ProcessFallbackConnection(const PassFallbackConnection& connection);

            // Sets up inputs from the list of PassConnections in PassRequest
            void SetupInputsFromRequest();

            // Sets up outputs from the list of PassConnections in PassRequest
            void SetupOutputsFromRequest();

            // Sets up explicitly declared dependencies on other passes declared in the PassRequest
            void SetupPassDependencies();

            // Sets up inputs from the list of PassConnections in PassTemplate
            void SetupInputsFromTemplate();

            // Sets up outputs from the list of PassConnections in PassTemplate
            void SetupOutputsFromTemplate();

            // Updates attachment sizes and formats from their specified source attachments
            void UpdateOwnedAttachments();

            // Updates m_attachmentUsageIndex on the bindings to handle multiple bindings using the same attachment
            void UpdateAttachmentUsageIndices();

            // --- Private Members ---

            // List of attachment binding indices for all the input bindings
            AZStd::fixed_vector<uint8_t, PassInputBindingCountMax> m_inputBindingIndices;

            // List of attachment binding indices for all the input/output bindings
            AZStd::fixed_vector<uint8_t, PassInputOutputBindingCountMax> m_inputOutputBindingIndices;

            // List of attachment binding indices for all the output bindings
            AZStd::fixed_vector<uint8_t, PassOutputBindingCountMax> m_outputBindingIndices;

            // Used to maintain references to imported attachments so they're underlying
            // buffers and images don't get deleted during attachment build phase
            AZStd::vector<Ptr<PassAttachment>> m_importedAttachmentStore;

            // Name of the pass. Will be concatenated with parent names to form a unique path
            Name m_name;

            // Path of the pass in the hierarchy. Example: Root.Ssao.Downsample
            Name m_path;

            // Depth of the tree hierarchy this pass is at.
            // Example: Root would be depth 0, Root.Ssao.Downsample depth 2
            uint32_t m_treeDepth = 0;

            // Used to track what phase of build/execution the pass is in
            PassState m_state = PassState::Uninitialized;

            // Used to track what phases of build/initialization the pass is queued for
            PassQueueState m_queueState = PassQueueState::NoQueue;
        };

        //! Struct used to return results from Pass hierarchy validation
        struct PassValidationResults
        {
            bool IsValid();
            void PrintValidationIfError();

            AZStd::vector<Pass*> m_passesWithErrors;
            AZStd::vector<Pass*> m_passesWithWarnings;
            AZStd::vector<Pass*> m_passesWithMissingInputs;
            AZStd::vector<Pass*> m_passesWithMissingOutputs;
            AZStd::vector<Pass*> m_passesWithMissingInputOutputs;
        };

    }   // namespace RPI
}   // namespace AZ


#ifdef AZ_ENABLE_TRACING

#define AZ_RPI_PASS_ASSERT(expression, ...)                             \
    if (!(expression))                                                  \
    {                                                                   \
        AZ_Assert(false, __VA_ARGS__);                                  \
        AZStd::string message = AZStd::string::format(__VA_ARGS__);     \
        LogError( AZStd::string::format(__VA_ARGS__) );                 \
    }                                                                   \

#define AZ_RPI_PASS_ERROR(expression, ...)                              \
    if (!(expression))                                                  \
    {                                                                   \
        AZ_Error("Pass System", false, __VA_ARGS__);                    \
        AZStd::string message = AZStd::string::format(__VA_ARGS__);     \
        LogError( AZStd::string::format(__VA_ARGS__) );                 \
    }                                                                   \

#define AZ_RPI_PASS_WARNING(expression, ...)                            \
    if (!(expression))                                                  \
    {                                                                   \
        AZ_Warning("Pass System", false, __VA_ARGS__);                  \
        AZStd::string message = AZStd::string::format(__VA_ARGS__);     \
        LogWarning( AZStd::string::format(__VA_ARGS__) );               \
    }                                                                   \

#else

#define AZ_RPI_PASS_ASSERT(expression, ...)   AZ_Assert(false, __VA_ARGS__);
#define AZ_RPI_PASS_ERROR(expression, ...)    AZ_Error("Pass System", false, __VA_ARGS__);
#define AZ_RPI_PASS_WARNING(expression, ...)  AZ_Warning("Pass System", false, __VA_ARGS__);

#endif


#if AZ_RPI_ENABLE_PASS_DEBUGGING

// This macro will break in pass code (functions that belong to Pass or it's child classes)
// if the name of the pass being executed is the same as the one specified for targeted debugging
// in the pass system (see functions with "TargetedPassDebugging" above)
#define AZ_RPI_BREAK_ON_TARGET_PASS                                                          \
    if(!GetName().IsEmpty() &&                                                               \
        GetName() == AZ::RPI::PassSystemInterface::Get()->GetTargetedPassDebuggingName())    \
    {                                                                                        \
        AZ::Debug::Trace::Break();                                                           \
    }

#else

#define AZ_RPI_BREAK_ON_TARGET_PASS

#endif
