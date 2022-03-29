/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Script/ScriptAsset.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Execution/ExecutionStateDeclarations.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>

#if !defined(_RELEASE)
#define SCRIPT_CANVAS_RUNTIME_ASSET_CHECK
#endif

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    class RuntimeComponent;

    struct ExecutionStateConfig
    {
        const RuntimeData& runtimeData;
        AZ::Data::Asset<RuntimeAsset> asset;
        AZStd::any userData;

        ExecutionStateConfig(AZ::Data::Asset<RuntimeAsset> asset);
        ExecutionStateConfig(AZ::Data::Asset<RuntimeAsset> asset, AZStd::any&& userData);
    };

    class ExecutionState
        : public AZStd::enable_shared_from_this<ExecutionState>
    {
    public:
        // chopping block - begin
    protected:
        const RuntimeComponent* GetRuntimeComponent() const;
    public:
        AZ::ComponentId GetComponentId() const;

        AZ::EntityId GetEntityId() const;

        GraphIdentifier GetGraphIdentifier() const;

        GraphIdentifier GetGraphIdentifier(const AZ::Data::AssetId& id) const;

        AZ::EntityId GetScriptCanvasId() const;

        // chopping block - end

        AZ_RTTI(ExecutionState, "{85C66E59-F012-460E-9756-B36819753F4D}", AZStd::enable_shared_from_this<ExecutionState>);
        AZ_CLASS_ALLOCATOR(ExecutionState, AZ::SystemAllocator, 0);

        static ExecutionStatePtr Create(ExecutionStateConfig& config);

        static void Reflect(AZ::ReflectContext* reflectContext);

        AZStd::any m_userData;

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

        virtual void Initialize() = 0;

        ExecutionStatePtr SharedFromThis();

        ExecutionStateConstPtr SharedFromThisConst() const;

        virtual void StopExecution() = 0;

        AZStd::string ToString() const;

        ExecutionStateWeakPtr WeakFromThis();

        ExecutionStateWeakConstPtr WeakFromThisConst() const;
    };

}
