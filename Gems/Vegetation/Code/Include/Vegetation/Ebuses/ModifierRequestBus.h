/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/Policies.h>

namespace Vegetation
{
    struct Descriptor;
    struct InstanceData;

    //stages determine the order of execution of modifier requests
    //currently used to ensure that positional modifiers occur first since surface related modifiers rely on a final position
    enum class ModifierStage : AZ::u8
    {
        PreProcess = 0,
        Standard,
        PostProcess,
    };

    /**
    * the EBus is used to perform modifications to vegetation instances
    */
    class ModifierRequests : public AZ::ComponentBus
    {
    public:
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;

        /**
        * Determines the order in which handlers receive events.
        * Handlers receive events based on the order in which the components are initialized,
        * unless a handler explicitly sets its position.
        */
        struct BusHandlerOrderCompare
        {
            AZ_FORCE_INLINE bool operator()(ModifierRequests* left, ModifierRequests* right) const { return left->GetModifierStage() < right->GetModifierStage(); }
        };

        virtual void Execute(InstanceData& instanceData) const = 0;

        virtual ModifierStage GetModifierStage() const { return ModifierStage::Standard; };
    };

    typedef AZ::EBus<ModifierRequests> ModifierRequestBus;
}
