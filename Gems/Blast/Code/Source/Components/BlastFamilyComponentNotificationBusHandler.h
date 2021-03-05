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

#include <Blast/BlastFamilyComponentBus.h>

namespace Blast
{
    //! Behavior handler which forwards BlastFamilyComponentNotificationBus events to script canvas.
    //! Note this class is not using the usual AZ_EBUS_BEHAVIOR_BINDER macro as the signature
    //! needs to be changed for script canvas because we cannot expose BlastActor itself to script canvas.
    class BlastFamilyComponentNotificationBusHandler
        : public BlastFamilyComponentNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(BlastFamilyComponentNotificationBusHandler, AZ::SystemAllocator, 0);
        AZ_RTTI(
            BlastFamilyComponentNotificationBusHandler, "{17C9DB55-8003-4610-B08D-7E369EC4225A}",
            AZ::BehaviorEBusHandler);
        static void Reflect(AZ::ReflectContext* context);

        BlastFamilyComponentNotificationBusHandler();

        // Script Canvas Signature
        void OnActorCreatedDummy(BlastActorData blastActor);
        void OnActorDestroyedDummy(BlastActorData blastActor);

        using EventFunctionsParameterPack = AZStd::Internal::pack_traits_arg_sequence<
            decltype(&BlastFamilyComponentNotificationBusHandler::OnActorCreatedDummy),
            decltype(&BlastFamilyComponentNotificationBusHandler::OnActorDestroyedDummy)>;

    private:
        enum class Function : int
        {
            OnActorCreated,
            OnActorDestroyed,
            Count
        };

        // AZ::BehaviorEBusHandler
        void Disconnect() override;
        bool Connect(AZ::BehaviorValueParameter* id = nullptr) override;
        bool IsConnected() override;
        bool IsConnectedId(AZ::BehaviorValueParameter* id) override;
        int GetFunctionIndex(const char* functionName) const override;

        // CollisionNotificationBus
        void OnActorCreated(const BlastActor& blastActor) override;
        void OnActorDestroyed(const BlastActor& blastActor) override;
    };
} // namespace Blast
