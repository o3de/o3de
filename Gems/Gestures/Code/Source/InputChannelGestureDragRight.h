/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Add the DragRight gesture
#if defined(CARBONATED)

#include "InputChannelGesture.h"

#include <Gestures/GestureRecognizerDragRight.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Input channel that recognizes continuous drag gestures.
    class InputChannelGestureDragRight : public InputChannelGesture
                                  , public RecognizerDragRight
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
            AZ_RTTI(TypeAndConfig, "{B6483887-D937-4458-ADC2-242A112E772D}", Type, Config);

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
        AZ_CLASS_ALLOCATOR(InputChannelGestureDragRight, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputChannelGestureDragRight, "{AD19678E-FE79-410D-8256-87FDAEA78099}", InputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel being constructed
        //! \param[in] inputDevice Input device that owns the input channel
        //! \param[in] config The configuration used to setup the gesture recognizer base class
        explicit InputChannelGestureDragRight(const AzFramework::InputChannelId& inputChannelId,
                                         const AzFramework::InputDevice& inputDevice,
                                         const Config& config);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannelGestureDragRight);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputChannelGestureDragRight() override;

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

#endif
