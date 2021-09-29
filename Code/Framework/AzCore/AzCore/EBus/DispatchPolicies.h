/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ::EBusPolicies
{
    struct PostDispatchTagType {};
    inline constexpr PostDispatchTagType PostDispatchTag;
    /**
    * Default EBus Policy used to indicate the EBus Event/Broadcast/Enumerate*
    * functions should perform no actions before and after finishing
    * dispatching on a thread
    * Any EBus which desires to have a function invoked after an EBus eventing operation
    * has completely finished on a thread dispatching should implement a struct with a
    * <ret-type> operator()(PostDispatchTagType) function override the `ThreadDispatchPolicy` type alias
    * in their EBusTraits derived class during Bus Configuration
    */
    struct ThreadDispatch
    {
        constexpr void operator()(PostDispatchTagType) = delete;
    };
}
