/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "InputChannelGesture.h"

#include <Gestures/GestureRecognizerHold.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Input channel that recognizes continuous hold gestures.
    class InputChannelGestureHold : public InputChannelGesture
                                  , public RecognizerHold
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The gesture configuration values that are exposed to the editor.
        struct TypeAndConfig : public Type, public Config
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Allocator
            AZ_CLASS_ALLOCATOR(TypeAndConfig, AZ::SystemAllocator);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Type Info
            AZ_RTTI(TypeAndConfig, "{B2D4C512-8330-46D6-AEA0-CE91EB795F19}", Type, Config);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Reflection
            static void Reflect(AZ::ReflectContext* context);

        protected:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! \ref Gestures::InputChannelGesture::CreateInputChannel
            InputChannelGesture* CreateInputChannel(const AzFramework::InputChannelId& channelId,
                                                    const AzFramework::InputDevice& inputDevice) override;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputChannelGestureHold, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputChannelGestureHold, "{FF1803B2-EF52-453E-B097-23B104D4AAA6}", InputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel being constructed
        //! \param[in] inputDevice Input device that owns the input channel
        //! \param[in] config The configuration used to setup the gesture recognizer base class
        explicit InputChannelGestureHold(const AzFramework::InputChannelId& inputChannelId,
                                         const AzFramework::InputDevice& inputDevice,
                                         const Config& config);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannelGestureHold);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputChannelGestureHold() override;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannel::GetValue
        float GetValue() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannel::GetCustomData
        const InputChannel::CustomData* GetCustomData() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref Gestures::RecognizerContinuous::OnContinuousGestureInitiated
        void OnContinuousGestureInitiated() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref Gestures::RecognizerContinuous::OnContinuousGestureUpdated
        void OnContinuousGestureUpdated() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref Gestures::RecognizerContinuous::OnContinuousGestureEnded
        void OnContinuousGestureEnded() override;
    };
} // namespace Gestures
