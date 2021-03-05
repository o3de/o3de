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

#include "InputChannelGesture.h"

#include <Gestures/GestureRecognizerPinch.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Input channel that recognizes continuous pinch gestures.
    class InputChannelGesturePinch : public InputChannelGesture
                                   , public RecognizerPinch
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The gesture configuration values that are exposed to the editor.
        struct TypeAndConfig : public Type, public Config
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Allocator
            AZ_CLASS_ALLOCATOR(TypeAndConfig, AZ::SystemAllocator, 0);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Type Info
            AZ_RTTI(TypeAndConfig, "{00F71451-19EB-488A-837D-ED438C75EB4B}", Type, Config);

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
        AZ_CLASS_ALLOCATOR(InputChannelGesturePinch, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputChannelGesturePinch, "{BFA07504-7C84-499E-B3C5-DA8CF4926BC5}", InputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel being constructed
        //! \param[in] inputDevice Input device that owns the input channel
        //! \param[in] config The configuration used to setup the gesture recognizer base class
        explicit InputChannelGesturePinch(const AzFramework::InputChannelId& inputChannelId,
                                          const AzFramework::InputDevice& inputDevice,
                                          const Config& config);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannelGesturePinch);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputChannelGesturePinch() override;

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
