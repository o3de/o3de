/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <ScriptCanvas/Execution/ExecutionStateDeclarations.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>
#include <ScriptCanvas/Grammar/DebugMap.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    class RuntimeComponent;

    struct RuntimeData;
    struct RuntimeDataOverrides;

    constexpr const AZ::u32 UserDataMark = AZ_CRC_CE("UserDataMark");

    struct ExecutionStateConfig
    {
        const RuntimeData& runtimeData;
        const RuntimeDataOverrides& overrides;
        ExecutionUserData&& userData;

        ExecutionStateConfig(const RuntimeDataOverrides& overrides);
        ExecutionStateConfig(const RuntimeDataOverrides& overrides, ExecutionUserData&& userData);
    };

    /// <summary>
    /// \class ExecutionState - the base abstract class that is the interface for the ScriptCanvas runtime and the hosting environment.
    /// It allows for customization of initialization, starting, and stopping execution. It only works on on valid runtime data, and holds
    /// user data. For example, in the Entity/Component system, the user data stores the information required to provide the Entity and
    /// Component that own the running graph. The actual runtime implementation is entirely up to subclasses.
    /// </summary>
    class ExecutionState
    {
    public:
        AZ_RTTI(ExecutionState, k_ExecutionStateAzTypeIdString);
        AZ_CLASS_ALLOCATOR(ExecutionState, AZ::SystemAllocator);

        const AZ::u32 m_lightUserDataMark = UserDataMark;

        ExecutionState(ExecutionStateConfig& config);

        virtual ~ExecutionState() = default;

        virtual void Execute() = 0;

        AZ::Data::AssetId GetAssetId() const;

        const Grammar::DebugExecution* GetDebugSymbolIn(size_t index) const;

        const Grammar::DebugExecution* GetDebugSymbolIn(size_t index, const AZ::Data::AssetId& id) const;

        const Grammar::DebugExecution* GetDebugSymbolOut(size_t index) const;

        const Grammar::DebugExecution* GetDebugSymbolOut(size_t index, const AZ::Data::AssetId& id) const;

        const Grammar::DebugExecution* GetDebugSymbolReturn(size_t index) const;

        const Grammar::DebugExecution* GetDebugSymbolReturn(size_t index, const AZ::Data::AssetId& id) const;

        const Grammar::DebugDataSource* GetDebugSymbolVariableChange(size_t index) const;

        const Grammar::DebugDataSource* GetDebugSymbolVariableChange(size_t index, const AZ::Data::AssetId& id) const;

        virtual ExecutionMode GetExecutionMode() const = 0;

        const RuntimeDataOverrides& GetRuntimeDataOverrides() const;

        const RuntimeData& GetRuntimeData() const;

        const ExecutionUserData& GetUserData() const;

        virtual void Initialize() = 0;

        virtual bool IsPure() const { return false; }

        ExecutionUserData& ModUserData() const;

        ExecutionStatePtr SharedFromThis();

        ExecutionStateConstPtr SharedFromThisConst() const;

        virtual void StopExecution() = 0;

        AZStd::string ToString() const;

        ExecutionStateWeakPtr WeakFromThis();

        ExecutionStateWeakConstPtr WeakFromThisConst() const;

    private:
        const RuntimeData& m_runtimeData;
        const RuntimeDataOverrides& m_overrides;
        ExecutionUserData m_userData;
    };
}
