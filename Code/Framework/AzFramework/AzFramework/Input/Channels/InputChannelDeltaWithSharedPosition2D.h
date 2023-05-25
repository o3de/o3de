/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Channels/InputChannelDelta.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class for input channels that emit one dimensional delta input values and share a position.
    //! Examples: mouse movement
    class InputChannelDeltaWithSharedPosition2D : public InputChannelDelta
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputChannelDeltaWithSharedPosition2D, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputChannelDeltaWithSharedPosition2D, "{F7EC8D6F-DC27-4CDF-80F4-EFA7DCC33837}", InputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel being constructed
        //! \param[in] inputDevice Input device that owns the input channel
        //! \param[in] sharedPositionData Shared ptr to the common position data
        explicit InputChannelDeltaWithSharedPosition2D(
            const AzFramework::InputChannelId& inputChannelId,
            const InputDevice& inputDevice,
            const SharedPositionData2D& sharedPositionData);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannelDeltaWithSharedPosition2D);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelDeltaWithSharedPosition2D() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the shared position data
        //! \return Pointer to the shared position data
        const AzFramework::InputChannel::CustomData* GetCustomData() const override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        const SharedPositionData2D m_sharedPositionData; //!< Shared position data
    };
} // namespace LmbrCentral
