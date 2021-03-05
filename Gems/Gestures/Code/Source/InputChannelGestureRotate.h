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

#include <Gestures/GestureRecognizerRotate.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Input channel that recognizes continuous pinch gestures.
    class InputChannelGestureRotate : public InputChannelGesture
                                    , public RecognizerRotate
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
            AZ_RTTI(TypeAndConfig, "{3CDFD30E-547C-4978-A01E-E51EBC9B791E}", Type, Config);

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
        AZ_CLASS_ALLOCATOR(InputChannelGestureRotate, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputChannelGestureRotate, "{12B90006-9CE5-4C75-A82C-5FD2BCFD347A}", InputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel being constructed
        //! \param[in] inputDevice Input device that owns the input channel
        //! \param[in] config The configuration used to setup the gesture recognizer base class
        explicit InputChannelGestureRotate(const AzFramework::InputChannelId& inputChannelId,
                                           const AzFramework::InputDevice& inputDevice,
                                           const Config& config);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannelGestureRotate);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputChannelGestureRotate() override;

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
