/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace EMotionFX::MotionMatching
{
    class FeatureSchema;

    class DebugDrawRequests
        : public AZ::EBusTraits
    {
    public:
        AZ_RTTI(DebugDrawRequests, "{7BBA4249-EC00-445C-8A0C-4472841049C3}");

        virtual void DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay) = 0;
    };
    using DebugDrawRequestBus = AZ::EBus<DebugDrawRequests>;

    class MotionMatchingRequests
    {
    public:
        AZ_RTTI(MotionMatchingRequests, "{b08f73cc-a922-49ef-8c0e-07166b43ea65}");
        virtual ~MotionMatchingRequests() = default;
    };

    class MotionMatchingEditorRequests
    {
    public:
        AZ_RTTI(MotionMatchingEditorRequests, "{A162E323-10FC-45A6-BE1A-9770CD459BE6}");
        virtual ~MotionMatchingEditorRequests() = default;

        virtual void SetDebugDrawFeatureSchema([[maybe_unused]] FeatureSchema* featureSchema) = 0;
        virtual FeatureSchema* GetDebugDrawFeatureSchema() const = 0;
    };
    
    class MotionMatchingBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using MotionMatchingRequestBus = AZ::EBus<MotionMatchingRequests, MotionMatchingBusTraits>;
    using MotionMatchingInterface = AZ::Interface<MotionMatchingRequests>;

    using MotionMatchingEditorRequestBus = AZ::EBus<MotionMatchingEditorRequests, MotionMatchingBusTraits>;
    using MotionMatchingEditorInterface = AZ::Interface<MotionMatchingEditorRequests>;

} // namespace EMotionFX::MotionMatching
