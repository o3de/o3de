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

#include <AzCore/RTTI/BehaviorContext.h>

#include "NodeableOut.h"
#include <ScriptCanvas/Execution/ExecutionState.h>

namespace AZ
{
    class SerializeContext;
}

namespace ScriptCanvas
{
    namespace Grammar
    {
        class SubgraphInterface;
    } 

    // derive from this to make an object that when wrapped with a NodeableNode can be instantly turned into a node that is easily embedded in graphs,
    // and easily compiled in
    class Nodeable
    {
    public:
        AZ_RTTI(Nodeable, "{C8195695-423A-4960-A090-55B2E94E0B25}");
        AZ_CLASS_ALLOCATOR(Nodeable, AZ::SystemAllocator, 0);

        // reflect nodeable class API
        static void Reflect(AZ::ReflectContext* reflectContext);

        Nodeable();

        Nodeable(ExecutionStateWeakPtr executionState);

        virtual ~Nodeable() = default;

        void CallOut(const AZ::Crc32 key, AZ::BehaviorValueParameter* resultBVP, AZ::BehaviorValueParameter* argsBVPs, int numArguments) const;

        AZ::Data::AssetId GetAssetId() const;

        AZ::EntityId GetEntityId() const;

        const Execution::FunctorOut& GetExecutionOut(AZ::Crc32 key) const;
        
        const Execution::FunctorOut& GetExecutionOutChecked(AZ::Crc32 key) const;

        virtual NodePropertyInterface* GetPropertyInterface(AZ::Crc32 /*propertyId*/) { return nullptr; }

        AZ::EntityId GetScriptCanvasId() const;

        void Deactivate();

        // \todo delete until needed, this level of optimization is not so necessary
        // any would only be good if graphs could opt into it, and execution slots could annotate changing activity level
        virtual bool IsActive() const { return false; }

        void InitializeExecutionOuts(const AZ::Crc32* begin, const AZ::Crc32* end);

        void InitializeExecutionOuts(const AZStd::vector<AZ::Crc32>& keys);

        void SetExecutionOut(AZ::Crc32 key, Execution::FunctorOut&& out);

        void SetExecutionOutChecked(AZ::Crc32 key, Execution::FunctorOut&& out);

    protected:
        void InitializeExecutionState(ExecutionState* executionState);

        virtual void OnDeactivate() {}

        // all of these hooks are known at compile time, so no branching
        // we will need with and without result calls for each time for method
        // methods with result but no result requested, etc
        
        template<typename t_Return>
        void OutResult(const AZ::Crc32 key, t_Return& result) const
        {
            // this is correct, it is up to the FunctorOut referenced by key to decide what to do with these params (whether to modify or handle strings differently)
            AZ::BehaviorValueParameter resultBVP(&result);

            CallOut(key, &resultBVP, nullptr, 0);

#if !defined(RELEASE) 
            if (!resultBVP.GetAsUnsafe<t_Return>())
            {
                AZ_Error("ScriptCanvas", false, "%s:CallOut(%u) failed to provide a useable result", TYPEINFO_Name(), (AZ::u32)key);
                return;
            }
#endif
            result = *resultBVP.GetAsUnsafe<t_Return>();
        }

        // Required to decay array type to pointer type
        template<typename T>
        using decay_array = AZStd::conditional_t<AZStd::is_array_v<AZStd::remove_reference_t<T>>, std::remove_extent_t<AZStd::remove_reference_t<T>>*, T&&>;

        template<typename t_Return, typename... t_Args>
        void OutResult(const AZ::Crc32 key, t_Return& result, t_Args&&... args) const
        {
            // this is correct, it is up to the FunctorOut referenced by key to decide what to do with these params (whether to modify or handle strings differently)
            AZStd::tuple<decay_array<t_Args>...> lvalueWrapper(AZStd::forward<t_Args>(args)...);
            using BVPReserveArray = AZStd::array<AZ::BehaviorValueParameter, sizeof...(args)>;
            auto MakeBVPArrayFunction = [](auto&&... element)
            {
                return BVPReserveArray{ {AZ::BehaviorValueParameter{&element}...} };
            };
            BVPReserveArray argsBVPs = AZStd::apply(MakeBVPArrayFunction, lvalueWrapper);
            AZ::BehaviorValueParameter resultBVP(&result);

            CallOut(key, &resultBVP, argsBVPs.data(), sizeof...(t_Args));

#if !defined(RELEASE) 
            if (!resultBVP.GetAsUnsafe<t_Return>())
            {
                AZ_Error("ScriptCanvas", false, "%s:CallOut(%u) failed to provide a useable result", TYPEINFO_Name(), (AZ::u32)key);
                return;
            }
#endif
            result = *resultBVP.GetAsUnsafe<t_Return>();
        }

        template<typename... t_Args>
        void ExecutionOut(const AZ::Crc32 key, t_Args&&... args) const
        {
            // this is correct, it is up to the FunctorOut referenced by key to decide what to do with these params (whether to modify or handle strings differently)
            AZStd::tuple<decay_array<t_Args>...> lvalueWrapper(AZStd::forward<t_Args>(args)...);
            using BVPReserveArray = AZStd::array<AZ::BehaviorValueParameter, sizeof...(args)>;
            auto MakeBVPArrayFunction = [](auto&&... element)
            {
                return BVPReserveArray{ {AZ::BehaviorValueParameter{&element}...} };
            };
            BVPReserveArray argsBVPs = AZStd::apply(MakeBVPArrayFunction, lvalueWrapper);

            CallOut(key, nullptr, argsBVPs.data(), sizeof...(t_Args));
        }

        void ExecutionOut(const AZ::Crc32 key) const
        {
            // this is correct, it is up to the FunctorOut referenced by key to decide what to do with these params (whether to modify or handle strings differently)
            CallOut(key, nullptr, nullptr, 0);
        }
        
    private:
        // keep this here, and don't even think about putting it back in the FunctorOuts by any method*, lambda capture or other.
        // programmers will need this for internal node state debugging and who knows what other reasons
        // * Lua execution is an exception
        ExecutionStateWeakPtr m_executionState = nullptr;
        Execution::FunctorOut m_noOpFunctor;
        AZStd::unordered_map<AZ::Crc32, Execution::FunctorOut> m_outs;
    };
    
}
