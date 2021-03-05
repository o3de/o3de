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

#include <AzFramework/Input/Channels/InputChannel.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Base class for all gesture related input channels
    class InputChannelGesture : public AzFramework::InputChannel
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Base class for gesture types that are exposed to the editor
        struct Type
        {
            ////////////////////////////////////////////////////////////////////////////////////////
            // Allocator
            AZ_CLASS_ALLOCATOR(Type, AZ::SystemAllocator, 0);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Type Info
            AZ_RTTI(Type, "{DA483C43-3CAC-4F27-97FD-4024C41E50B1}");

            ////////////////////////////////////////////////////////////////////////////////////////
            // Reflection
            static void Reflect(AZ::ReflectContext* context);

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Destructor
            virtual ~Type() = default;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Override to create the relevant gesture input channel
            //! \param[in] inputChannelId Id of the input channel to be created
            //! \param[in] inputDevice Input device that owns the input channel
            virtual InputChannelGesture* CreateInputChannel(const AzFramework::InputChannelId& channelId,
                                                            const AzFramework::InputDevice& inputDevice) = 0;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputChannelGesture, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputChannelGesture, "{A26F1958-7AF7-48AB-87AA-12AD76088BCA}", InputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel being constructed
        //! \param[in] inputDevice Input device that owns the input channel
        explicit InputChannelGesture(const AzFramework::InputChannelId& inputChannelId,
                                     const AzFramework::InputDevice& inputDevice)
            : AzFramework::InputChannel(inputChannelId, inputDevice) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannelGesture);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelGesture() override = default;
    };
} // namespace Gestures
