/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ScopeProducer.h>

namespace AZ::RHI
{
    template <typename UserData>
    class EmptyCompileFunction
    {
    public:
        void operator() (const FrameGraphCompileContext&, UserData&) {}
    };

    template <typename UserData>
    class EmptyExecuteFunction
    {
    public:
        void operator() (const FrameGraphExecuteContext&, const UserData&) {}
    };

    //! This specialized scope producer provides a simple functional model for managing a scope.
    //! It may help reduce boilerplate in cases where a very simple scope is required and it
    //! becomes impractical to marshal data from a parent class.
    template <
        typename UserData,
        typename PrepareFunction,
        typename CompileFunction = EmptyCompileFunction<UserData>,
        typename ExecuteFunction = EmptyExecuteFunction<UserData>>
    class ScopeProducerFunction
        : public ScopeProducer
    {
    public:
        AZ_CLASS_ALLOCATOR(ScopeProducerFunction, SystemAllocator);

        template <typename UserDataParam>
        ScopeProducerFunction(
            const ScopeId& scopeId,
            UserDataParam&& userData,
            PrepareFunction prepareFunction,
            CompileFunction compileFunction = CompileFunction(),
            ExecuteFunction executeFunction = ExecuteFunction(),
            int deviceIndex = RHI::MultiDevice::InvalidDeviceIndex)
            : ScopeProducer(scopeId, deviceIndex)
            , m_userData{AZStd::forward<UserDataParam>(userData)}
            , m_prepareFunction{AZStd::move(prepareFunction)}
            , m_compileFunction{AZStd::move(compileFunction)}
            , m_executeFunction{AZStd::move(executeFunction)}
        {}

        UserData& GetUserData()
        {
            return m_userData;
        }

        const UserData& GetUserData() const
        {
            return m_userData;
        }

    private:
        //////////////////////////////////////////////////////////////////////////
        // ScopeProducer overrides
        void SetupFrameGraphDependencies(FrameGraphInterface builder) override
        {
            m_prepareFunction(builder, m_userData);
        }

        void CompileResources(const FrameGraphCompileContext& context) override
        {
            m_compileFunction(context, m_userData);
        }

        void BuildCommandList(const FrameGraphExecuteContext& context) override
        {
            m_executeFunction(context, m_userData);
        }
        //////////////////////////////////////////////////////////////////////////

        UserData m_userData;
        PrepareFunction m_prepareFunction;
        CompileFunction m_compileFunction;
        ExecuteFunction m_executeFunction;
    };

    // Helper class to build scope producer with functions
    class ScopeProducerFunctionNoData final : public RHI::ScopeProducer
    {
    public:
        AZ_CLASS_ALLOCATOR(ScopeProducerFunctionNoData, SystemAllocator);

        using PrepareFunction = AZStd::function<void(RHI::FrameGraphInterface)>;
        using CompileFunction = AZStd::function<void(const RHI::FrameGraphCompileContext&)>;
        using ExecuteFunction = AZStd::function<void(const RHI::FrameGraphExecuteContext&)>;

        ScopeProducerFunctionNoData(
            const RHI::ScopeId& scopeId,
            PrepareFunction prepareFunction,
            CompileFunction compileFunction,
            ExecuteFunction executeFunction,
            HardwareQueueClass hardwareQueueClass = HardwareQueueClass::Graphics,
            int deviceIndex = RHI::MultiDevice::InvalidDeviceIndex)
            : ScopeProducer(scopeId, deviceIndex)
            , m_prepareFunction{ AZStd::move(prepareFunction) }
            , m_compileFunction{ AZStd::move(compileFunction) }
            , m_executeFunction{ AZStd::move(executeFunction) }
        {
            InitScope(scopeId, hardwareQueueClass, deviceIndex);
        }

    private:
        //////////////////////////////////////////////////////////////////////////
        // ScopeProducer overrides
        void SetupFrameGraphDependencies(RHI::FrameGraphInterface builder) override
        {
            m_prepareFunction(builder);
        }

        void CompileResources(const RHI::FrameGraphCompileContext& context) override
        {
            m_compileFunction(context);
        }

        void BuildCommandList(const RHI::FrameGraphExecuteContext& context) override
        {
            m_executeFunction(context);
        }
        //////////////////////////////////////////////////////////////////////////

        PrepareFunction m_prepareFunction;
        CompileFunction m_compileFunction;
        ExecuteFunction m_executeFunction;
    };
}
