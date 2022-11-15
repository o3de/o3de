/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/BehaviorContext.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/NodeableOut.h>
#include <ScriptCanvas/Execution/ExecutionState.h>

namespace ScriptCanvas
{
    /*
    Note: Many parts of AzAutoGen, compilation, and runtime depend on the order of declaration and addition of slots.
    The display order can be manipulated in the editor, but it will always just be a change of view.

    Whenever in doubt, this is the order, in pseudo code

    for in : Ins do
        somethingOrdered(in)
        for branch : in.Branches do
            somethingOrdered(branch)
        end
    end
    for out : Outs do
        somethingOrdered(out)
    end
    */

    // derive from this to make an object that when wrapped with a NodeableNode can be instantly turned into a node that is easily embedded in graphs,
    // and easily compiled in
    class Nodeable
    {
    public:
        AZ_RTTI(Nodeable, "{C8195695-423A-4960-A090-55B2E94E0B25}");
        AZ_CLASS_ALLOCATOR(Nodeable, AZ::SystemAllocator, 0);

        // reflect nodeable class API
        static void Reflect(AZ::ReflectContext* reflectContext);

        // the run-time constructor for non-EBus handlers
        Nodeable();

        // this constructor is used by EBus handlers only
        Nodeable(ExecutionStateWeakPtr executionState);

        virtual ~Nodeable() = default;

        void CallOut(size_t index, AZ::BehaviorArgument* resultBVP, AZ::BehaviorArgument* argsBVPs, int numArguments) const;

        const Execution::FunctorOut& GetExecutionOut(size_t index) const;
        
        const Execution::FunctorOut& GetExecutionOutChecked(size_t index) const;

        virtual NodePropertyInterface* GetPropertyInterface(AZ::Crc32 /*propertyId*/) { return nullptr; }

        void Deactivate();

        // \todo delete until needed, this level of optimization is not so necessary
        // any would only be good if graphs could opt into it, and execution slots could annotate changing activity level
        virtual bool IsActive() const { return false; }

        void InitializeExecutionOuts(size_t count);

        void SetExecutionOut(size_t index, Execution::FunctorOut&& out);

        void SetExecutionOutChecked(size_t index, Execution::FunctorOut&& out);

    protected:
        ExecutionStateWeakConstPtr GetExecutionState() const;

        void InitializeExecutionOutByRequiredCount();

        void InitializeExecutionState(ExecutionState* executionState);

        virtual void OnInitializeExecutionState() {}

        virtual void OnDeactivate() {}

        virtual size_t GetRequiredOutCount() const { return 0; }

        // Required to decay array type to pointer type
        template<typename T>
        using decay_array = AZStd::conditional_t<AZStd::is_array_v<AZStd::remove_reference_t<T>>, std::remove_extent_t<AZStd::remove_reference_t<T>>*, T&&>;

        // all of these hooks are known at compile time, so no branching
        // we will need with and without result calls for each type of method
        // methods with result but no result requested, etc
        
        template<typename... t_Args>
        void ExecutionOut(size_t index, t_Args&&... args) const
        {
            // it is up to the FunctorOut referenced by key to decide what to do with these params (whether to modify or handle strings differently)
            AZStd::tuple<decay_array<t_Args>...> lvalueWrapper(AZStd::forward<t_Args>(args)...);
            using BVPReserveArray = AZStd::array<AZ::BehaviorArgument, sizeof...(args)>;
            auto MakeBVPArrayFunction = [](auto&&... element)
            {
                return BVPReserveArray{ {AZ::BehaviorArgument{&element}...} };
            };

            BVPReserveArray argsBVPs = AZStd::apply(MakeBVPArrayFunction, lvalueWrapper);
            CallOut(index, nullptr, argsBVPs.data(), sizeof...(t_Args));
        }

        void ExecutionOut(size_t index) const
        {
            // it is up to the FunctorOut referenced by key to decide what to do with these params (whether to modify or handle strings differently)
            CallOut(index, nullptr, nullptr, 0);
        }
        template<typename t_Return>
        void ExecutionOutResult(size_t index, t_Return& result) const
        {
            // It is up to the FunctorOut referenced by the index to decide what to do with these params (whether to modify or handle strings differently)
            AZ::BehaviorArgument resultBVP(&result);
            CallOut(index, &resultBVP, nullptr, 0);

#if defined(SC_RUNTIME_CHECKS_ENABLED) 
            if (!resultBVP.GetAsUnsafe<t_Return>())
            {
                AZ_Error("ScriptCanvas", false, "%s:CallOut(%zu) failed to provide a useable result", TYPEINFO_Name(), index);
                return;
            }
#endif
            result = *resultBVP.GetAsUnsafe<t_Return>();
        }

        template<typename t_Return, typename... t_Args>
        void ExecutionOutResult(size_t index, t_Return& result, t_Args&&... args) const
        {
            // it is up to the FunctorOut referenced by key to decide what to do with these params (whether to modify or handle strings differently)
            AZStd::tuple<decay_array<t_Args>...> lvalueWrapper(AZStd::forward<t_Args>(args)...);
            using BVPReserveArray = AZStd::array<AZ::BehaviorArgument, sizeof...(args)>;
            auto MakeBVPArrayFunction = [](auto&&... element)
            {
                return BVPReserveArray{ {AZ::BehaviorArgument{&element}...} };
            };

            BVPReserveArray argsBVPs = AZStd::apply(MakeBVPArrayFunction, lvalueWrapper);
            AZ::BehaviorArgument resultBVP(&result);
            CallOut(index, &resultBVP, argsBVPs.data(), sizeof...(t_Args));

#if defined(SC_RUNTIME_CHECKS_ENABLED) 
            if (!resultBVP.GetAsUnsafe<t_Return>())
            {
                AZ_Error("ScriptCanvas", false, "%s:CallOut(%zu) failed to provide a useable result", TYPEINFO_Name(), index);
                return;
            }
#endif
            result = *resultBVP.GetAsUnsafe<t_Return>();
        }

    private:
        ExecutionStateWeakPtr m_executionState = nullptr;
        Execution::FunctorOut m_noOpFunctor;
        AZStd::vector<Execution::FunctorOut> m_outs;
    };
    
}
