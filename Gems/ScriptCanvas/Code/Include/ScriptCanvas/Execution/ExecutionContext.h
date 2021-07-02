/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvas
{
    class RuntimeComponent;
    class VariableData;

    struct RuntimeData;

    namespace Execution
    {
        using ActivationInputArray = AZStd::array<AZ::BehaviorValueParameter, 128>;

        struct ActivationData
        {
            ActivationData(const RuntimeComponent& component, ActivationInputArray& storage);
            ActivationData(const AZ::EntityId entityId, const VariableData& variableOverrides, const RuntimeData& runtimeData, ActivationInputArray& storage);

            const AZ::EntityId entityId;
            const VariableData& variableOverrides;
            const RuntimeData& runtimeData;
            ActivationInputArray& storage;
        };
        
        struct ActivationInputRange
        {
            AZ::BehaviorValueParameter* inputs = nullptr;
            bool requiresDependencyConstructionParameters = false;
            size_t nodeableCount = 0;
            size_t variableCount = 0;
            size_t entityIdCount = 0;
            size_t totalCount = 0;
        };

        class Context
        {
        public:
            AZ_TYPE_INFO(Context, "{2C137581-19F4-42EB-8BF3-14DBFBC02D8D}");
            AZ_CLASS_ALLOCATOR(Context, AZ::SystemAllocator, 0);

            static ActivationInputRange CreateActivateInputRange(ActivationData& activationData);
            static void InitializeActivationData(RuntimeData& runtimeData);
            static void UnloadData(RuntimeData& runtimeData);

        private:
            static void IntializeActivationInputs(RuntimeData& runtimeData, AZ::BehaviorContext& behaviorContext);
            static void IntializeStaticCloners(RuntimeData& runtimeData, AZ::BehaviorContext& behaviorContext);
        };

    } 
} 
