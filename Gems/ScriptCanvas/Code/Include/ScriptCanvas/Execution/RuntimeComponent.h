/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>
#include <ScriptCanvas/Execution/ExecutionPerformanceTimer.h>
#include <ScriptCanvas/Execution/ExecutionStateDeclarations.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>

#include "RuntimeComponent.h"


namespace ScriptCanvas
{
    using VariableIdMap = AZStd::unordered_map<VariableId, VariableId>;

    //! Runtime Component responsible for loading an executing the compiled ScriptCanvas graph from a runtime
    //! asset.
    //! It contains none of the Graph functionality of Validating Connections, as well as adding
    //! and removing nodes. Furthermore none of the functionality to remove and add variables
    //! exist in this component.
    //! It is assumed that the graph has runtime graph has already been validated and compiled
    //! at this point.
    //! This component should only be used at runtime 
    class RuntimeComponent
        : public AZ::Component
        , public AZ::EntityBus::Handler
    {
    public:
        AZ_COMPONENT(RuntimeComponent, "{95BFD916-E832-4956-837D-525DE8384282}", AZ::Component);

        static void Reflect(AZ::ReflectContext* context);

        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        RuntimeComponent() = default;

        RuntimeComponent(AZ::Data::Asset<RuntimeAsset> runtimeAsset);

        const AZ::Data::Asset<RuntimeAsset>& GetAsset() const;

        const RuntimeData& GetAssetData() const;

        GraphIdentifier GetGraphIdentifier() const;

        ExecutionMode GetExecutionMode() const;

        AZ::EntityId GetScriptCanvasId() const;

        const VariableData& GetVariableOverrides() const;

        void SetVariableOverrides(const VariableData& overrideData);

    protected:
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            // Cannot be used with either the GraphComponent or the VariableManager Component
            incompatible.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
            incompatible.push_back(AZ_CRC("ScriptCanvasVariableService", 0x819c8460));
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptCanvasRuntimeService", 0x776e1e3a));
        }

        void Activate() override;

        ActivationInfo CreateActivationInfo() const;

        void Deactivate() override {}

        void Execute();

        void Init() override;

        void InitializeExecution();

        // AZ::EntityBus handling
        void OnEntityActivated(const AZ::EntityId& entityId) override;

        void OnEntityDeactivated(const AZ::EntityId&) override;

        void StopExecution();

    private:
        AZ::Data::Asset<RuntimeAsset> m_runtimeAsset;
        ExecutionStatePtr m_executionState;
        AZ::EntityId m_scriptCanvasId;
                
        //! Per instance variable data overrides for the runtime asset
        //! This is serialized when building this component from the EditorScriptCanvasComponent
        // \todo remove the names from this data, and make it more lightweight
        // it only needs variable id and value
        // move the other information to the runtime system component
        VariableData m_variableOverrides;
    };
}
