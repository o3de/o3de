/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/FrameSchedulerEnums.h>
#include <Atom/RHI/FrameGraphExecuteGroup.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>

namespace AZ::RHI
{
    class FrameGraph;

    //! Fill this descriptor when initializing a FrameScheduler instance.
    struct FrameGraphExecuterDescriptor
    {
        AZStd::unordered_map<int, ConstPtr<PlatformLimitsDescriptor>> m_platformLimitsDescriptors;
    };

    //! FrameGraphExecuter is a context for executing the scopes of a compiled FrameGraph
    //! on the GPU using the associated device instance. The details of how scopes are executed
    //! are platform specific.
    //!
    //! The goal of this class is enable users to record and submit command lists at a granularity
    //! prescribed by the platform, while also respecting the 'threading' policy of the underlying platform.
    //! Modern platform implementations will allow full multi-threaded recording of command lists, while others
    //! will require serialization onto a single thread.
    //!
    //! To ensure the maximum flexibility for each platform, scope execution is divided into two layers: 'Execute Groups',
    //! and 'Execute Contexts'.
    //!
    //! - Execute Groups contain a list of Execute Contexts. FrameGraphExecuter::GetJobPolicy() describes the policy
    //! for whether groups can be processed serially or independently. Again, the underlying platform assigns this policy.
    //!
    //! - Execute Contexts provides a mapping between a command list and scope. A context can either represent the full
    //! set of work for scope, or it can be 1 in a set of several contexts processing the same scope. The latter scenario
    //! is common in cases where many commands are processed within the same scope and the platform decides to partition the
    //! work across several jobs. Each execute group describes its policy for whether contexts can be recorded independently
    //! on separate threads.
         
    //! The class provides two API's: one for external users (e.g. the FrameScheduler), and one
    //! for the derived platform implementation.
         
    //! To use this class, first call Begin to prepare the executer using a compiled FrameGraph
    //! instance. Then, iterate over the execute groups and process each one (either independently or
    //! serially, depending on the platform policy). Call End to complete processing of the graph.
    //!
    //! To implement this class, assign the job policy specific to your platform, and every Begin call, use the provided
    //! AddGroup method to partition the FrameGraph into execution groups. Each group and context will have platform
    //! specific overrides.
    class FrameGraphExecuter
        : public Object
    {
    public:
        virtual ~FrameGraphExecuter() = default;

        AZ_RTTI(FrameGraphExecuter, "{9DF0B900-A426-4056-8638-38A5007825F5}", Object);

            
        //! Initializes the frame graph executer. Instances are created in an uninitialized state. Attempting
        //! to use an uninitialized instance will result in an error (when validation is enabled). If the call
        //! fails, an error code is returned and the instance will remain in an uninitialized state.
        ResultCode Init(const FrameGraphExecuterDescriptor& descriptor);

        //! Shuts down the frame graph executer, releasing all internal allocations. The user
        //! may re-initialize.
        void Shutdown() override final;
            
        //! Returns the job policy for context groups. The policy dictates whether groups can be
        //! independently traversed across multiple threads. If the value is JobPolicy::Serial,
        //! BeginGroup and EndGroup must be called in order for each group index. If the value
        //! is JobPolicy::Parallel, BeginGroup and EndGroup can be called for each group independently
        //! from any thread.
        JobPolicy GetJobPolicy() const;

        //! Returns the number of context groups in the executer. The user must call BeginGroup
        //! and EndGroup on all instances prior to calling End.
        uint32_t GetGroupCount() const;

        //! Begins a new execution phase by inspecting an generating context groups from
        //! the provided frame graph instance. State within the executer is reset between
        //! each Begin / End cycle, so the implementor must rebuild the context groups each
        //! time. The frame graph instance is not stored, and must be in a compiled state.
        void Begin(const FrameGraph& frameGraph);

        //! Begins the group at specified index @param groupIndex. The index
        //! must be less than GetGroupCount. All groups must be processed
        //! each cycle prior to calling end. The returned execute group instance
        //! is valid until EndGroup is called (using the same group index), after which
        //! the user must not access it.
        FrameGraphExecuteGroup* BeginGroup(uint32_t groupIndex);

        //! Ends the group at index @param groupIndex. This
        //! invalidates the pointer returned by BeginGroup.
        void EndGroup(uint32_t groupIndex);

        //! Ends the graph execution phase. Call this after all execution jobs have joined. This
        //! resets all state held by the executer.
        void End();

    protected:
        FrameGraphExecuter() = default;

        //! Platform implementations should assign the job policy for context groups if multi-threaded recording is
        //! desired. By default, it is set to JobPolicy::Serial.
        void SetJobPolicy(JobPolicy jobPolicy);

        //! Adds a new group of the specified type (must inherit from FrameGraphExecuteGroup) and
        //! returns an instance. The schedule maintains ownership of the class. The user is expected
        //! to initialize the instance before returning the schedule to the external client. The returned
        //! instance is not persistent and will be deleted in Reset.
        template <typename FrameGraphExecuteGroupType>
        FrameGraphExecuteGroupType* AddGroup();

        //! Returns a list of the registered execute groups.
        AZStd::span<const AZStd::unique_ptr<FrameGraphExecuteGroup>> GetGroups() const;

    private:
        //////////////////////////////////////////////////////////////////////////
        // Platform API

        /// Called when the schedule is initializing.
        virtual ResultCode InitInternal(const FrameGraphExecuterDescriptor& descriptor) = 0;

        /// Called when the schedule is shutting down.
        virtual void ShutdownInternal() = 0;

        //! Called to prepare the executer with a new FrameGraph instance. State is cleared
        //! every cycle, so the platform should use this method to build the execution schedule
        //! via AddGroup.
        virtual void BeginInternal(const FrameGraph& frameGraph) = 0;

        /// Called when a group is ready to be submitted.
        virtual void ExecuteGroupInternal(FrameGraphExecuteGroup& group) = 0;

        /// Called when graph execution ends.
        virtual void EndInternal() = 0;

        //////////////////////////////////////////////////////////////////////////

        JobPolicy m_jobPolicy = JobPolicy::Serial;

        AZStd::mutex m_pendingContextGroupLock;
        AZStd::queue<FrameGraphExecuteGroup*> m_pendingGroups;
        AZStd::vector<AZStd::unique_ptr<FrameGraphExecuteGroup>> m_groups;

        FrameGraphExecuterDescriptor m_descriptor;

    };

    template <typename FrameGraphExecuteGroupType>
    FrameGraphExecuteGroupType* FrameGraphExecuter::AddGroup()
    {
        FrameGraphExecuteGroupType* group = aznew FrameGraphExecuteGroupType();
        m_groups.emplace_back(group);
        m_pendingGroups.emplace(group);
        return group;
    }
}
