/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/allocator.h>
#include <AzCore/std/allocator_static.h>
#include <AzCore/std/typetraits/internal/type_sequence_traits.h>

#include <ScriptCanvas/Core/NodeableOut.h>

namespace ScriptCanvas
{    
    namespace Execution
    {
        template<typename Callable, typename Allocator, typename... Args, size_t... IndexSequence>
        FunctorOut CreateOutWithArgs(Callable&& callable, Allocator& allocator, AZStd::Internal::pack_traits_arg_sequence<Args...>, std::index_sequence<IndexSequence...>, ReturnTypeIsNotVoid)
        {
            auto nodeCallWrapper = [callable = AZStd::forward<Callable>(callable)](AZ::BehaviorValueParameter* result, AZ::BehaviorValueParameter* arguments, int numArguments) mutable
            {
                constexpr size_t numFunctorArguments = sizeof...(Args);
                (void)numArguments;
                AZ_Assert(numArguments == numFunctorArguments, "number of arguments doesn't match number of parameters");
                AZ_Assert(result, "no null result allowed");
                result->StoreResult(AZStd::invoke(callable, *arguments[IndexSequence].GetAsUnsafe<Args>()...));
            };

            static_assert(!AZStd::is_same_v<Allocator, StackAllocatorType> || sizeof(nodeCallWrapper) <= MaxNodeableOutStackSize, "Lambda is too large to fit within NodebleOut functor");
            return FunctorOut(AZStd::move(nodeCallWrapper), allocator);
        }

        template<typename Callable, typename Allocator, typename... Args, size_t... IndexSequence>
        FunctorOut CreateOutWithArgs(Callable&& callable, Allocator& allocator, AZStd::Internal::pack_traits_arg_sequence<Args...>, std::index_sequence<IndexSequence...>, ReturnTypeIsVoid)
        {
            auto nodeCallWrapper = [callable = AZStd::forward<Callable>(callable)](AZ::BehaviorValueParameter*, AZ::BehaviorValueParameter* arguments, int numArguments) mutable
            {
                constexpr size_t numFunctorArguments = sizeof...(Args);
                (void)numArguments;
                AZ_Assert(numArguments == numFunctorArguments, "number of arguments doesn't match number of parameters");
                AZStd::invoke(callable, *arguments[IndexSequence].GetAsUnsafe<Args>()...);
            };

            static_assert(!AZStd::is_same_v<Allocator, StackAllocatorType> || sizeof(nodeCallWrapper) <= MaxNodeableOutStackSize, "Lambda is too large to fit within NodebleOut functor");
            return FunctorOut(AZStd::move(nodeCallWrapper), allocator);
        }

        template<typename Callable, typename Allocator>
        FunctorOut CreateOut(Callable&& callable, Allocator& allocator)
        {
            using CallableTraits = AZStd::function_traits<AZStd::remove_cvref_t<Callable>>;
            return CreateOutWithArgs
                ( AZStd::forward<Callable&&>(callable)
                , allocator
                , typename CallableTraits::arg_types{}
                , typename CallableTraits::template expand_args<std::index_sequence_for>{}
                , typename AZStd::is_void<typename CallableTraits::return_type>::type{});
        }

    } 
       
} 
