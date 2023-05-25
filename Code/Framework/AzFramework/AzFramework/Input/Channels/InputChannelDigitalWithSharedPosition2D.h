/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Channels/InputChannelDigital.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class for input channels that emit one dimensional digital input value and share a position.
    //! Examples: mouse button
    class InputChannelDigitalWithSharedPosition2D : public InputChannelDigital
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputChannelDigitalWithSharedPosition2D, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputChannelDigitalWithSharedPosition2D, "{EFCEC2F4-A81F-4218-A878-1D7676FB1FC6}", InputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel being constructed
        //! \param[in] inputDevice Input device that owns the input channel
        //! \param[in] sharedPositionData Shared ptr to the common position data
        explicit InputChannelDigitalWithSharedPosition2D(
            const AzFramework::InputChannelId& inputChannelId,
            const InputDevice& inputDevice,
            const SharedPositionData2D& sharedPositionData);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannelDigitalWithSharedPosition2D);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelDigitalWithSharedPosition2D() override = default;

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
