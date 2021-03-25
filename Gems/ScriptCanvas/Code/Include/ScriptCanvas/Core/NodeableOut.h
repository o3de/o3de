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