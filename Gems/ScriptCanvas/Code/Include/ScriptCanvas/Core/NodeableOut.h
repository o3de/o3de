/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    struct BehaviorValueParameter;
}

namespace ScriptCanvas
{
    namespace Execution
    {
        static constexpr size_t MaxNodeableOutStackSize = 512;

        using FunctorOut = AZStd::function<void(AZ::BehaviorValueParameter* result, AZ::BehaviorValueParameter* arguments, int numArguments)>;

        using ReturnTypeIsVoid = AZStd::true_type;
        using ReturnTypeIsNotVoid = AZStd::false_type;

        using HeapAllocatorType = AZStd::allocator;
        using StackAllocatorType = AZStd::static_buffer_allocator<MaxNodeableOutStackSize, 32>;
    } // namespace Execution

} // namespace ScriptCanvas
