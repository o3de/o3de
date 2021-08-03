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

    struct RuntimeData;
    struct RuntimeDataOverrides;

    namespace Execution
    {
        using ActivationInputArray = AZStd::array<AZ::BehaviorValueParameter, 128>;

        struct ActivationData
        {
            const RuntimeDataOverrides& variableOverrides;
            const RuntimeData& runtimeData;
            ActivationInputArray& storage;

            ActivationData(const RuntimeDataOverrides& variableOverrides, ActivationInputArray& storage);

            const void* GetVariableSource(size_t index, size_t& overrideIndexTracker) const;
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

            static ActivationInputRange CreateActivateInputRange(ActivationData& activationData, const AZ::EntityId& forSliceSupportOnly);
            static void InitializeActivationData(RuntimeData& runtimeData);
            static void UnloadData(RuntimeData& runtimeData);

        private:
            static void IntializeActivationInputs(RuntimeData& runtimeData, AZ::BehaviorContext& behaviorContext);
            static void IntializeStaticCloners(RuntimeData& runtimeData, AZ::BehaviorContext& behaviorContext);
        };

    }
}
