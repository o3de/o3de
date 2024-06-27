/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Scope.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RHI/FrameGraphExecuteContext.h>

namespace AZ::RHI
{
    //! ScopeProducer is the base class for systems which produce scopes on the frame scheduler.
    //! The user is expected to inherit from this class and implement three virtual methods:
    //!     - SetupFrameGraphDependencies
    //!     - CompileResources
    //!     - BuildCommandList
    //! It can then be registered with the frame scheduler each frame. Internally, this process
    //! generates a Scope which is inserted to the frame graph internally.
    //!
    //! EXAMPLE:
    //!
    //! class MyScope : public RHI::ScopeProducer
    //! {
    //! public:
    //!     MyScope()
    //!         : RHI::ScopeProducer(RHI::ScopeId{"MyScopeId"})
    //!     {}
    //!
    //! private:
    //!     void SetupFrameGraphDependencies(FrameGraphInterface frameGraph) override
    //!     {
    //!         // Create attachments on the builder, use them.
    //!     }
    //!
    //!     void CompileResources(const FrameGraphCompileContext& context) override
    //!     {
    //!         // Use the provided context to access image / buffer views and
    //!         // build ShaderResourceGroups.
    //!     }
    //!
    //!     void BuildCommandList(const FrameGraphExecuteContext& context) override
    //!     {
    //!         // A context is provided which allows you to access the command
    //!         // list for execution.
    //!     }
    //! };
    //!
    //! MyScope scope;
    //!
    //! // Register with the scheduler each frame. Callbacks will be
    //! // invoked.
    //! frameScheduler.AddScope(scope);

    class ScopeProducer
    {
        friend class FrameScheduler;
    public:
        virtual ~ScopeProducer() = default;
        ScopeProducer(const ScopeId& scopeId, int deviceIndex = MultiDevice::InvalidDeviceIndex);

        //! Returns the scope id associated with this scope producer.
        const ScopeId& GetScopeId() const;

        //! Returns the scope associated with this scope producer.
        const Scope* GetScope() const;

        //! Returns the device index of the device the scope should run on.
        //! May return InvalidDeviceIndex to signal that no device index is specified.
        virtual int GetDeviceIndex() const;

    protected:

        //!  Protected default constructor for classes that inherit from
        //!  ScopeProducer but that can't supply a ScopeId at construction.
        ScopeProducer();

        //!  Sets the HardwareQueueClass on the scope
        void SetHardwareQueueClass(HardwareQueueClass hardwareQueueClass);

        //!  DEPRECATED.
        //!  @deprecated Use InitScope instead
        void SetScopeId(const ScopeId& scopeId);

        //!  Initializes the scope with a ScopeId, HardwareQueueClass and device index.
        //!  Used by classes that inherit from ScopeProducer but can't supply a ScopeId at construction.
        void InitScope(const ScopeId& scopeId, HardwareQueueClass hardwareQueueClass = HardwareQueueClass::Graphics, int deviceIndex = MultiDevice::InvalidDeviceIndex);

    private:
        //////////////////////////////////////////////////////////////////////////
        // User Overrides - Derived classes should override from these methods.

        //! This function is called during the schedule setup phase. The client is expected to declare
        //! attachments using the provided \param frameGraph.
        virtual void SetupFrameGraphDependencies(FrameGraphInterface frameGraph) = 0;

        //! This function is called after compilation of the frame graph, but before execution. The provided
        //! FrameGraphAttachmentContext allows you to access RHI views associated with attachment
        //! ids. This is the method to build ShaderResourceGroups from transient attachment views.
        virtual void CompileResources(const FrameGraphCompileContext& context) { AZ_UNUSED(context); }

        //! This function is called at command list recording time and may be called multiple times
        //! if the schedule decides to split work items across command lists. In this case, each invocation
        //! will provide a command list and invocation index.
        virtual void BuildCommandList(const FrameGraphExecuteContext& context) { AZ_UNUSED(context); }

        //////////////////////////////////////////////////////////////////////////

        Scope* GetScope();

        ScopeId m_scopeId;
        Ptr<Scope> m_scope;
        int m_deviceIndex{MultiDevice::InvalidDeviceIndex};
    };
}
