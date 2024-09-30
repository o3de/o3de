/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/FrameSchedulerEnums.h>
#include <Atom/RHI/Object.h>
#include <Atom/RHI/ObjectCache.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/Buffer.h>

//! Struct used as a key for m_imageReverseLookupHash map below. The reason for using a struct instead of a hash directly is
//! so that the map can handle hash collision correctly by using the == operator. This struct contains
//! the resource name and the view descriptor related to the image views.
struct ImageResourceViewData
{
    AZ::Name m_name;
    AZ::RHI::ImageViewDescriptor m_imageViewDescriptor;

    bool operator==(const ImageResourceViewData& other) const
    {
        return this->m_name == other.m_name && this->m_imageViewDescriptor == other.m_imageViewDescriptor;
    };

    AZ::HashValue64 GetHash() const
    {
        return m_imageViewDescriptor.GetHash(AZ::TypeHash64(m_name.GetHash()));
    }
};

//! Struct used as a key for m_bufferReverseLookupHash map below. The reason for using a struct instead of a hash directly is
//! so that the map can handle hash collision correctly by using the == operator. This struct contains
//! the resource name and the view descriptor related to the buffer views.
struct BufferResourceViewData
{
    AZ::Name m_name;
    AZ::RHI::BufferViewDescriptor m_bufferViewDescriptor;

    bool operator==(const BufferResourceViewData& other) const
    {
        return this->m_name == other.m_name && this->m_bufferViewDescriptor == other.m_bufferViewDescriptor;
    };

    AZ::HashValue64 GetHash() const
    {
        return m_bufferViewDescriptor.GetHash(AZ::TypeHash64(m_name.GetHash()));
    }
};

namespace AZStd
{
   template<>
   struct hash<ImageResourceViewData>
   {
       size_t operator()(const ImageResourceViewData& imageResourceViewData) const noexcept
       {
           return aznumeric_cast<size_t>(imageResourceViewData.GetHash());
       }
   };
}
 
namespace AZStd
{
    template<>
    struct hash<BufferResourceViewData>
    {
        size_t operator()(const BufferResourceViewData& bufferResourceViewData) const noexcept
        {
            return aznumeric_cast<size_t>(bufferResourceViewData.GetHash());
        }
    };
}


namespace AZ::RHI
{
    class FrameGraph;
    class FrameGraphAttachmentDatabase;
    class ResourcePoolFrameAttachment;
    class TransientAttachmentPool;
    class Scope;

    /**
        * @brief Fill this request structure and pass to FrameGraphCompiler::Compile.
        */
    struct FrameGraphCompileRequest
    {
        /// The graph to compile. Must be a valid instance that was just built (but not compiled).
        /// i.e. by calling FrameGraph::End. It is not valid to re-use a compiled FrameGraph
        /// instance. It must be cleared and re-built each time.
        FrameGraph* m_frameGraph = nullptr;

        /// The transient attachment pool used for transient attachment allocations. Must be a valid instance.
        TransientAttachmentPool* m_transientAttachmentPool = nullptr;

        /// The verbosity requested for compilation. Logs are emitted using the AzCore logging functions.
        FrameSchedulerLogVerbosity m_logVerbosity = FrameSchedulerLogVerbosity::None;

        /// Flags controlling compilation behavior.
        FrameSchedulerCompileFlags m_compileFlags = FrameSchedulerCompileFlags::None;

        /// Flags controlling statistics of the pools.
        FrameSchedulerStatisticsFlags m_statisticsFlags = FrameSchedulerStatisticsFlags::None;
    };

    //! FrameGraphCompiler controls compilation of FrameGraph each frame. FrameScheduler owns
    //! and drives an instance of this class, so end-users should never need to interact with it directly.
    //! Platform implementations, on the other hand, are required to override this class in order to perform
    //! platform-specific scope construction.
    //!
    //! The compiler is designed to be invoked every frame; the graph is simply rebuilt each time. The compile
    //! operation is also done on a single thread; so overhead should be kept to a minimum.
    //!
    //! The RHI base class performs platform-independent compilation before passing control down to the derived
    //! platform implementation. The provided FrameGraph instance is compiled in-place according to the
    //! following phases:
    //!
    //!      == Cross-Queue Graph Edges == 
    //!
    //! FrameGraph contains a graph of Scope instances. Scopes are topologically sorted prior to
    //! compilation as part of the graph construction process. Scopes associate directly to a "Hardware Queue Class":
    //! Graphics, Compute, or Copy. These three queue classes must be synchronized between each other. To make this
    //! easier on platforms, the base compiler takes the topologically flattened graph and collates it into
    //! three independent sorted lists--one for each queue class. Then, a queue-centric producer-consumer graph is
    //! constructed across the scopes. Specifically:
    //!
    //!  class Scope
    //!  {
    //!      Scope* m_producersByQueue[HardwareQueueClassCount];
    //!      Scope* m_consumersByQueue[HardwareQueueClassCount];
    //!  };
    //!
    //! This graph makes it possible to walk along a queue or across queue boundaries at dependency points. Each
    //! platform can then trivially crawl this graph to derive signal / wait fence values, if applicable.
    //!
    //!      == Transient Attachments ==
    //!
    //! Transient attachments are intra-frame and do not persist after the frame ends and can take the form of
    //! buffers or images. These attachments are owned by a DeviceTransientAttachmentPool; every frame, the pool
    //! is reset. Since attachments are always declared for usage on scopes, its full usage chain--and thus its
    //! lifetime across the frame--is immediately available.
    //!
    //! The phase first constructs the scope lifetime for each attachment. Then, memory for each attachment is
    //! allocated from the transient attachment pool, one scope at a time. This allows the pool to record begin
    //! and end usages for each attachment per scope. Internally, the platform implementation can use this
    //! information to place aliased resources onto one or more heaps of memory.
    //!
    //! One important consideration is dealing with aliasing across queue boundaries. Since queues must be
    //! synchronized between each other, attempting to alias memory across two queues at the same time will
    //! produce a race condition. To solve this, when faced with a queue overlap, the compiler extends the
    //! lifetime of the attachment until a join operation occurs. However, the compiler picks a single queue
    //! which is allowed to alias during that region by inspecting which one will see the biggest potential
    //! gain. This way, some aliasing is still allowed when async compute / copy is in use.
    //! 
    //! Finally, because the resources themselves are effectively re-created each frame, a cache of views is
    //! kept inside the compiler. The cache is big enough to avoid having to re-create views every frame, but
    //! bounded in order to release entries old views.
    //! 
    //!      == Platform-Specific Compilation ==
    //! 
    //! Finally, the compiler calls into the platform-specific compile method, which hands control over to the
    //! derived class. The platform implementation is expected to further process the frame graph and scope data
    //! down to platform-specific actions. For example:
    //! 
    //!  1) Derive transition barriers by walking the scope attachment chain on each frame attachment.
    //!  2) Derive queue fence values by walking the queue-centric scope graph.
    class FrameGraphCompiler
        : public Object
    {
    public:
        AZ_RTTI(FrameGraphCompiler, "{A126F362-C163-432E-94DE-61AA4DFDF102}", Object);

        ResultCode Init();

        void Shutdown() override final;

        //! Compiles the frame graph. Platform-independent compilation is done first
        //! according to the provided flags. At the end, the platform-dependent compilation
        //! method is invoked.
        MessageOutcome Compile(const FrameGraphCompileRequest& request);

    protected:
        FrameGraphCompiler() = default;

    private:
        //////////////////////////////////////////////////////////////////////////
        // Platform API

        /// Called when the compiler is initializing.
        virtual ResultCode InitInternal() = 0;

        /// Called when the compiler is shutting down.
        virtual void ShutdownInternal() = 0;

        /// Called when platform-independent compilation has completed. Platform-specific
        /// compilation should be done here.
        virtual MessageOutcome CompileInternal(const FrameGraphCompileRequest& request) = 0;

        //////////////////////////////////////////////////////////////////////////

        MessageOutcome ValidateCompileRequest(const FrameGraphCompileRequest& request) const;

        void CompileQueueCentricScopeGraph(
            FrameGraph& frameGraph,
            FrameSchedulerCompileFlags compileFlags);

        void ExtendTransientAttachmentAsyncQueueLifetimes(
            FrameGraph& frameGraph,
            FrameSchedulerCompileFlags compileFlags);

        //! Extends the life of transient attachments to the end of the graph group.
        void ExtendTransientAttachmentGroupLifetimes(
            FrameGraph& frameGraph,
            FrameSchedulerCompileFlags compileFlags);

        //! Helper function for doing ExtendTransientAttachmentGroupLifetimes
        template<class T>
        void ExtendTransientAttachmentGroupLifetimesHelper(
            const AZStd::vector<Scope*>& scopes,
            const AZStd::vector<T*>& frameAttachments);

        //! Optimize the load store action of the first and last usage of the a transient attachments.
        void OptimizeTransientLoadStoreActions(
            FrameGraph& frameGraph,
            FrameSchedulerCompileFlags compileFlags);

        //! Helper function for OptimizeTransientLoadStoreActions
        template<class T>
        void OptimizeTransientLoadStoreActionsHelper(
             const AZStd::vector<T*>& frameAttachments);

        void CompileTransientAttachments(
            FrameGraph& frameGraph,
            AZ::RHI::TransientAttachmentPool& transientAttachmentPool,
            FrameSchedulerCompileFlags compileFlags,
            FrameSchedulerStatisticsFlags statisticsFlags);

        void CompileResourceViews(const FrameGraphAttachmentDatabase& attachmentDatabase);

        //! Remove the entry related to the provided ReverseLookupObjectType from the appropriate cache as it is probably stale now
        template<typename ReverseLookupObjectType, typename ObjectCacheType>
        void RemoveFromCache(ReverseLookupObjectType objectToRemove,
                                AZStd::unordered_map<ReverseLookupObjectType, HashValue64>& reverseHashLookupMap,
                                ObjectCache<ObjectCacheType>& objectCache);

        // Returns the resource from local cache if it exists within it or create one if it doesn't and add it to the cache
        ImageView* GetImageViewFromLocalCache(AZ::RHI::Image* image, const ImageViewDescriptor& imageViewDescriptor);
        BufferView* GetBufferViewFromLocalCache(AZ::RHI::Buffer* buffer, const BufferViewDescriptor& bufferViewDescriptor);
            
        // This cache is mainly for transient resources. It adds a dependency to the resource views and hence they wont be
        // deleted at the end of the frame and re-created at the start. Mainly used as an optimization.  
        ObjectCache<ImageView> m_imageViewCache;
        ObjectCache<BufferView> m_bufferViewCache;
            
        // The maps below are used to reverse look up view hashes so we can clear them out of m_imageViewCache/m_bufferViewCache
        // once they have been replaced with a new view instance. 
        AZStd::unordered_map<ImageResourceViewData, HashValue64> m_imageReverseLookupHash;
        AZStd::unordered_map<BufferResourceViewData, HashValue64> m_bufferReverseLookupHash;
    };
}
