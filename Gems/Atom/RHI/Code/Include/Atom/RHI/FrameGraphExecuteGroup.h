/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/FrameSchedulerEnums.h>
#include <Atom/RHI/FrameGraphExecuteContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ::RHI
{
    //! Provides a platform-independent implementation of ExecuteGroupContext.
    //!
    //! This class handles the ExecuteGroupContext methods and exposes a new API for platforms to override. Platforms
    //! which utilize hierarchical command lists (e.g. Vulkan / Metal) can store the primary command list on this
    //! class, and then organize the group structure such that child parallel command lists for a particular scope
    //! become FrameGraphExecuteContexts.
    //!
    //! Alternatively, this class can be used to structure work into batches so that submission occurs once for a
    //! set of command lists.
    class FrameGraphExecuteGroup
    {
        friend class FrameGraphExecuter;
    public:
        AZ_RTTI(FrameGraphExecuteGroup, "{159FAD55-17EC-4B09-986F-01B740F96448}");

        virtual ~FrameGraphExecuteGroup() = default;

        // Returns whether every context in the group has finished its begin / end cycle.
        bool IsComplete() const;

        // Returns whether the group itself has finished the begin / end cycle.
        bool IsSubmittable() const;

        /// Returns the number of execute contexts in the group.
        uint32_t GetContextCount() const;

        /// Begins the context at index \param contextIndex.
        FrameGraphExecuteContext* BeginContext(uint32_t contextindex);

        //! Ends the context at index \ param contextIndex. This
        //! invalidates the FrameGraphExecuteContext* provided by BeginContext.
        void EndContext(uint32_t contextIndex);

        //! Returns the job policy for this group. The policy informs
        //! whether each context in the group can be independently traversed. If serial,
        //! then BeginContext and EndContext must be called IN ORDER on the same thread.
        //! If parallel, they may be called independently from any thread.
        JobPolicy GetJobPolicy() const;

    protected:
        FrameGraphExecuteGroup() = default;

        //! Used when a context group consists of a single scope partitioned across several command lists.
        //! Must be called by the derived class at initialization time.
        struct InitRequest
        {
            /// The scope id used for all the contexts in this group (one context for each command list).
            ScopeId m_scopeId;

            /// The index of the device the group is running on.
            int m_deviceIndex = MultiDevice::DefaultDeviceIndex;

            /// The submit count for the scope
            uint32_t m_submitCount = 0;

            /// The ordered array of command lists in the group. This can be null if the user wishes to
            /// assign command lists at context begin time.
            CommandList* const * m_commandLists = nullptr;

            /// The number of command lists in the array.
            uint32_t m_commandListCount = 0;

            /// The job policy used for this group.
            JobPolicy m_jobPolicy = JobPolicy::Serial;
        };

        void Init(const InitRequest& request);

        //! Used when a context group consists of a single command list partitions across several scopes.
        //! Must be called by the derived class at initialization time. This type of group only supports
        //! JobPolicy::Serial usage (this is because command lists are not thread-safe).
        struct InitMergedRequest
        {
            /// The command list shared by all scopes in the group. This can be null if the
            /// user wishes to fill in the command list at context creation time.
            CommandList* m_commandList = nullptr;

            /// The index of the device the group is running on.
            int m_deviceIndex = MultiDevice::DefaultDeviceIndex;

            struct ScopeEntry
            {
                ScopeId m_scopeId;
                uint32_t m_submitCount = 0;
            };

            /// An ordered list of scope ids and submit counts in the group.
            const ScopeEntry* m_scopeEntries = nullptr;

            /// The number of scopes in the ScopeEntry array.
            uint32_t m_scopeCount = 0;
        };

        void Init(const InitMergedRequest& request);

    private:
        //////////////////////////////////////////////////////////////////////////
        // Platform API

        // Called when the group has begun recording.
        virtual void BeginInternal();

        // Called when a context in the group has begun recording.
        virtual void BeginContextInternal(
            FrameGraphExecuteContext& context,
            uint32_t contextIndex);

        // Called when a context in the group has ended recording.
        virtual void EndContextInternal(
            FrameGraphExecuteContext& context,
            uint32_t contextIndex);

        // Called when the group has finished recording.
        virtual void EndInternal();
        //////////////////////////////////////////////////////////////////////////

        JobPolicy m_jobPolicy = JobPolicy::Serial;
        AZStd::vector<FrameGraphExecuteContext> m_contexts;
        AZStd::atomic_int m_contextCountActive = { 0 };
        AZStd::atomic_int m_contextCountCompleted = { 0 };
        AZStd::atomic_bool m_isSubmittable = { false };
    };
}
