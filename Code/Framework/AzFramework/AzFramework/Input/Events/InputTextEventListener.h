/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Input/Buses/Notifications/InputTextNotificationBus.h>

// Start: fix for windows defining max/min macros
#pragma push_macro("max")
#pragma push_macro("min")
#undef max
#undef min

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class that handles input text event notifications by priority
    class InputTextEventListener : public InputTextNotificationBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Predefined text event listener priority, used to sort handlers from highest to lowest
        inline static AZ::s32 GetPriorityFirst()    { return std::numeric_limits<AZ::s32>::max(); }
        inline static AZ::s32 GetPriorityDebug()    { return (GetPriorityFirst() / 4) * 3;        }
        inline static AZ::s32 GetPriorityUI()       { return GetPriorityFirst() / 2;              }
        inline static AZ::s32 GetPriorityDefault()  { return 0;                                   }
        inline static AZ::s32 GetPriorityLast()     { return std::numeric_limits<AZ::s32>::min(); }
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        explicit InputTextEventListener();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] autoConnect Whether to connect to the input notification bus on construction
        explicit InputTextEventListener(bool autoConnect);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] priority The priority used to sort relative to other text event listeners
        explicit InputTextEventListener(AZ::s32 priority);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] priority The priority used to sort relative to other text event listeners
        //! \param[in] autoConnect Whether to connect to the input notification bus on construction
        explicit InputTextEventListener(AZ::s32 priority, bool autoConnect);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Default copying
        AZ_DEFAULT_COPY(InputTextEventListener);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputTextEventListener() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputTextNotifications::GetPriority
        AZ::s32 GetPriority() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Connect to the input notification bus to start receiving input notifications
        void Connect();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Disconnect from the input notification bus to stop receiving input notifications
        void Disconnect();

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputTextNotifications::OnInputTextEvent
        void OnInputTextEvent(const AZStd::string& textUTF8, bool& o_hasBeenConsumed) final;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when input text events are generated, but not those already been
        //! consumed by a higher priority listener, or those that do not pass this listener's filter.
        //! \param[in] textUTF8 The text to process (encoded using UTF-8)
        //! \return True if the text event has been consumed, false otherwise
        ////////////////////////////////////////////////////////////////////////////////////////////
        virtual bool OnInputTextEventFiltered(const AZStd::string& textUTF8) = 0;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZ::s32 m_priority; //!< The priority used to sort relative to other input event listeners
    };
} // namespace AzFramework

// End: fix for windows defining max/min macros
#pragma pop_macro("max")
#pragma pop_macro("min")
