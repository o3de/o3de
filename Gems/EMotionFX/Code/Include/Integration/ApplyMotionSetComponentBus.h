/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if defined(CARBONATED)
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/EntityId.h>

namespace EMotionFX
{
    namespace Integration
    {
#if defined(CARBONATED)
        enum MotionSetGender : int32_t
        {
            MOTION_NONE,
            MOTION_NEUTRAL,
            MOTION_FEMALE,
            MOTION_MALE
        };
#endif

        /**
         * EmotionFX Anim Graph Component Request Bus
         * Used for making requests to the EMotionFX Anim Graph Components.
         */
        class ApplyMotionSetComponentRequests
            : public AZ::ComponentBus
        {
        public:

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            // applies the defined motion set to this Entity's AnimGraphInstance
#if defined(CARBONATED)
            virtual void Apply(const AZ::EntityId& id, const MotionSetGender& preferredGender) = 0;
#else
            virtual void Apply(const AZ::EntityId& id) = 0;
#endif
        };

        using ApplyMotionSetComponentRequestBus = AZ::EBus<ApplyMotionSetComponentRequests>;

        //Editor
        class EditorApplyMotionSetComponentRequests
            : public AZ::ComponentBus
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            /// Retrieves the component's MotionSet AssetId.
            /// \return assetId
            virtual const AZ::Data::AssetId& GetMotionSetAssetId() = 0;
        };

        using EditorApplyMotionSetComponentRequestBus = AZ::EBus<EditorApplyMotionSetComponentRequests>;        
    } // namespace Integration
} // namespace EMotionFX
#endif
