/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
