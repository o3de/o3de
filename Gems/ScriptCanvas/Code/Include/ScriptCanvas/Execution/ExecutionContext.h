/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        using ActivationInputArray = AZStd::array<AZ::BehaviorArgument, 128>;

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
            AZ::BehaviorArgument* inputs = nullptr;
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
            AZ_CLASS_ALLOCATOR(Context, AZ::SystemAllocator);

            static ActivationInputRange CreateActivateInputRange(ActivationData& activationData);
            static void InitializeStaticActivationData(RuntimeData& runtimeData);

        private:
            static void InitializeStaticActivationInputs(RuntimeData& runtimeData, AZ::BehaviorContext& behaviorContext);
            static void InitializeStaticCloners(RuntimeData& runtimeData, AZ::BehaviorContext& behaviorContext);
            static void InitializeStaticCreationFunction(RuntimeData& runtimeData);
        };

        class TypeErasedReference
        {
        public:
            AZ_TYPE_INFO(TypeErasedReference, "{608FD64B-EA34-45EB-9ADB-265B8A69AE00}");

            // asserts the address is NOT nullptr
            TypeErasedReference(void* validAddress, const AZ::TypeId& type);

            void* Address() const;

            template<typename T>
            T* As() const
            {
                AZ_Assert(azrtti_typeid<T>() == m_type, "Request to cast type other than that originally set");
                return reinterpret_cast<T*>(m_address);
            }

            const AZ::TypeId& Type() const;

        private:
            void* const m_address;
            const AZ::TypeId m_type;
        };
    }
}
