/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/XcbConnectionManager.h>
#include <AzFramework/XcbEventHandler.h>
#include <AzFramework/XcbInterface.h>

#include <xcb/xfixes.h>
#include <xcb/xinput.h>

// The maximum number of raw input axis this mouse device supports.
constexpr uint32_t MAX_XI_RAW_AXIS = 2;

// The sensitivity of the wheel.
constexpr float MAX_XI_WHEEL_SENSITIVITY = 140.0f;

namespace AzFramework
{
    class XcbInputDeviceMouse
        : public InputDeviceMouse::Implementation
        , public XcbEventHandlerBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(XcbInputDeviceMouse, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        XcbInputDeviceMouse(InputDeviceMouse& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        virtual ~XcbInputDeviceMouse();

        static XcbInputDeviceMouse::Implementation* Create(InputDeviceMouse& inputDevice);

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorState
        void SetSystemCursorState(SystemCursorState systemCursorState) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorState
        SystemCursorState GetSystemCursorState() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorPositionNormalized
        void SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorPositionNormalized
        AZ::Vector2 GetSystemCursorPositionNormalized() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::TickInputDevice
        void TickInputDevice() override;

        //! This method is called by the Editor to accommodate some events with the Editor. Never called in Game mode.
        void PollSpecialEvents() override;

        //! Handle X11 events.
        void HandleXcbEvent(xcb_generic_event_t* event) override;

        //! Initialize XFixes extension. Used for barriers.
        static bool InitializeXFixes();

        //! Initialize XInput extension. Used for raw input during confinement and showing/hiding the cursor.
        static bool InitializeXInput();

        //! Enables/Disables XInput Raw Input events.
        void SetEnableXInput(bool enable);

        //! Create barriers.
        void CreateBarriers(xcb_window_t window, bool create);

        //! Helper function.
        void SystemCursorStateToLogic(SystemCursorState systemCursorState, bool& confined, bool& cursorShown);

        //! Shows/Hides the cursor.
        void ShowCursor(xcb_window_t window, bool show);

        //! Get the normalized cursor position. The coordinates returned are relative to the specified window.
        AZ::Vector2 GetSystemCursorPositionNormalizedInternal(xcb_window_t window) const;

        //! Set the normalized cursor position. The normalized position will be relative to the specified window.
        void SetSystemCursorPositionNormalizedInternal(xcb_window_t window, AZ::Vector2 positionNormalized);

        //! Handle button press/release events.
        void HandleButtonPressEvents(uint32_t detail, bool pressed);

        //! Handle motion notify events.
        void HandlePointerMotionEvents(const xcb_generic_event_t* event);

        //! Will set cursor states and confinement modes.
        void HandleCursorState(xcb_window_t window, SystemCursorState systemCursorState);

        //! Will handle all raw input events.
        void HandleRawInputEvents(const xcb_ge_generic_event_t* event);

        //! Convert XInput fp1616 to float.
        inline float fp1616ToFloat(xcb_input_fp1616_t value) const
        {
            return static_cast<float>((value >> 16) + (value & 0xffff) / 0xffff);
        }

        //! Convert XInput fp3232 to float.
        inline float fp3232ToFloat(xcb_input_fp3232_t value) const
        {
            return static_cast<float>(value.integral) + static_cast<float>(value.frac / (float)(1ull << 32));
        }

        const InputChannelId* InputChannelFromMouseEvent(xcb_button_t button, bool& isWheel, float& direction) const
        {
            isWheel = false;
            direction = 1.0f;
            switch (button)
            {
            case XCB_BUTTON_INDEX_1:
                return &InputDeviceMouse::Button::Left;
            case XCB_BUTTON_INDEX_2:
                return &InputDeviceMouse::Button::Right;
            case XCB_BUTTON_INDEX_3:
                return &InputDeviceMouse::Button::Middle;
            case XCB_BUTTON_INDEX_4:
                isWheel = true;
                direction = 1.0f;
                break;
            case XCB_BUTTON_INDEX_5:
                isWheel = true;
                direction = -1.0f;
                break;
            default:
                break;
            }

            return nullptr;
        }

        // Barriers work only with positive values. We clamp here to zero.
        inline int16_t Clamp(int16_t value) const
        {
            return value < 0 ? 0 : value;
        }

    private:
        //! The current system cursor state
        SystemCursorState m_systemCursorState;

        //! The cursor position before it got hidden.
        AZ::Vector2 m_cursorHiddenPosition;

        AZ::Vector2 m_systemCursorPositionNormalized;
        uint32_t m_systemCursorPosition[MAX_XI_RAW_AXIS];

        static xcb_connection_t* s_xcbConnection;
        static xcb_screen_t* s_xcbScreen;

        //! Will be true if the xfixes extension could be initialized.
        static bool m_xfixesInitialized;

        //! Will be true if the xinput2 extension could be initialized.
        static bool m_xInputInitialized;

        //! The window that had focus
        xcb_window_t m_prevConstraintWindow;

        //! The current window that has focus
        xcb_window_t m_focusWindow;

        //! Will be true if the cursor is shown else false.
        bool m_cursorShown;

        struct XFixesBarrierProperty
        {
            xcb_xfixes_barrier_t id;
            uint32_t direction;
            int16_t x0, y0, x1, y1;
        };

        //! Array that holds barrier information used to confine the cursor.
        std::vector<XFixesBarrierProperty> m_activeBarriers;
    };
} // namespace AzFramework
