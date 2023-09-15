/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>
#include <AzFramework/Input/Events/InputChannelEventSink.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>


#include <UIKit/UIKit.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
@interface VirtualKeyboardTextFieldDelegate : NSObject <UITextFieldDelegate>
{
    AzFramework::InputDeviceVirtualKeyboard::Implementation* m_inputDevice;
    UITextField* m_textField;
@public
    float m_activeTextFieldNormalizedBottomY;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (id)initWithInputDevice: (AzFramework::InputDeviceVirtualKeyboard::Implementation*)inputDevice
            withTextField: (UITextField*)textField;

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)keyboardWillChangeFrame: (NSNotification*)notification;

////////////////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)textFieldShouldClear: (UITextField*)textField;

////////////////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)textFieldShouldReturn: (UITextField*)textField;

////////////////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)textField: (UITextField*)textField
        shouldChangeCharactersInRange: (NSRange)range
        replacementString: (NSString*)string;
@end // VirtualKeyboardTextFieldDelegate interface

////////////////////////////////////////////////////////////////////////////////////////////////////
@implementation VirtualKeyboardTextFieldDelegate

////////////////////////////////////////////////////////////////////////////////////////////////////
- (id)initWithInputDevice: (AzFramework::InputDeviceVirtualKeyboard::Implementation*)inputDevice
            withTextField: (UITextField*)textField
{
    if ((self = [super init]))
    {
        self->m_inputDevice = inputDevice;
        self->m_textField = textField;

        // Resgister to be notified when the keyboard frame size changes so we can then adjust the
        // position of the view accordingly to ensure we don't obscure the text field being edited.
        // We don't need to explicitly remove the observer:
        // https://developer.apple.com/library/mac/releasenotes/Foundation/RN-Foundation/index.html#10_11NotificationCenter
        [[NSNotificationCenter defaultCenter] addObserver: self
                                                 selector: @selector(keyboardWillChangeFrame:)
                                                     name: UIKeyboardWillChangeFrameNotification
                                                   object: nil];
    }

    return self;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)keyboardWillChangeFrame: (NSNotification*)notification
{
    if (!m_textField || !m_textField.superview)
    {
        return;
    }

    // Get the keyboard rect in terms of the view to account for orientation.
    CGRect keyboardRect = [notification.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
    keyboardRect = [m_textField.superview convertRect: keyboardRect fromView: nil];

    // Calculate the offset needed so the active text field is not being covered by the keyboard.
    const double activeTextFieldBottom = m_activeTextFieldNormalizedBottomY * m_textField.superview.bounds.size.height;
    const double offsetY = AZ::GetMin(0.0, keyboardRect.origin.y - activeTextFieldBottom);

    // Create the offset view rect and transform it into the coordinate space of the main window.
    CGRect offsetViewRect = CGRectMake(0, offsetY, m_textField.superview.bounds.size.width,
                                                   m_textField.superview.bounds.size.height);
    offsetViewRect = [m_textField.superview convertRect: offsetViewRect toView: nil];

    // Remove any existing offset applied in previous calls to this function.
    offsetViewRect.origin.x -= m_textField.superview.frame.origin.x;
    offsetViewRect.origin.y -= m_textField.superview.frame.origin.y;

    m_textField.superview.frame = offsetViewRect;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)textFieldShouldClear: (UITextField*)textField
{
    // Queue an 'clear' command event.
    m_inputDevice->QueueRawCommandEvent(AzFramework::InputDeviceVirtualKeyboard::Command::EditClear);

    // Return false so that the text field itself does not update.
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)textFieldShouldReturn: (UITextField*)textField
{
    // Queue an 'enter' command event.
    m_inputDevice->QueueRawCommandEvent(AzFramework::InputDeviceVirtualKeyboard::Command::EditEnter);

    // Return false so that the text field itself does not update.
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)textField: (UITextField*)textField
        shouldChangeCharactersInRange: (NSRange)range
        replacementString: (NSString*)string
{
    // If the string length is 0, the user has pressed the backspace key on the virtual keyboard.
    const AZStd::string textUTF8 = string.length ? string.UTF8String : "\b";
    m_inputDevice->QueueRawTextEvent(textUTF8);

    // Return false so that the text field itself does not update.
    return FALSE;
}
@end // VirtualKeyboardTextFieldDelegate implementation

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for ios virtual keyboard input devices
    class InputDeviceVirtualKeyboardiOS : public InputDeviceVirtualKeyboard::Implementation
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceVirtualKeyboardiOS, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceVirtualKeyboardiOS(InputDeviceVirtualKeyboard& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceVirtualKeyboardiOS() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceVirtualKeyboard::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceVirtualKeyboard::Implementation::HasTextEntryStarted
        bool HasTextEntryStarted() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceVirtualKeyboard::Implementation::TextEntryStart
        void TextEntryStart(const InputTextEntryRequests::VirtualKeyboardOptions& options) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceVirtualKeyboard::Implementation::TextEntryStop
        void TextEntryStop() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceVirtualKeyboard::Implementation::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        UITextField* m_textField = nullptr;
        VirtualKeyboardTextFieldDelegate* m_textFieldDelegate = nullptr;
        AZStd::unique_ptr<InputChannelEventSink> m_inputChannelEventSink;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboardiOS::InputDeviceVirtualKeyboardiOS(
        InputDeviceVirtualKeyboard& inputDevice)
        : InputDeviceVirtualKeyboard::Implementation(inputDevice)
        , m_textField(nullptr)
        , m_textFieldDelegate(nullptr)
    {
        // Create a UITextField that we can call becomeFirstResponder on to show the keyboard.
        m_textField = [[UITextField alloc] initWithFrame: CGRectZero];

        // Create and set the text field's delegate so we can respond to keyboard input.
        m_textFieldDelegate = [[VirtualKeyboardTextFieldDelegate alloc] initWithInputDevice: this withTextField: m_textField];
        m_textField.delegate = m_textFieldDelegate;

        // Disable autocapitalization and autocorrection, which both behave strangely.
        m_textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
        m_textField.autocorrectionType = UITextAutocorrectionTypeNo;

        // Hide the text field so it will never actually be shown.
        m_textField.hidden = YES;

        // Add something to the text field so delete works.
        m_textField.text = @" ";
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboardiOS::~InputDeviceVirtualKeyboardiOS()
    {
        if (m_textField)
        {
            m_textField.delegate = nullptr;
            [m_textFieldDelegate release];
            m_textFieldDelegate = nullptr;

            [m_textField removeFromSuperview];
            [m_textField release];
            m_textField = nullptr;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceVirtualKeyboardiOS::IsConnected() const
    {
        // Virtual keyboard input is always available
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceVirtualKeyboardiOS::HasTextEntryStarted() const
    {
        return m_textField ? m_textField.isFirstResponder : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboardiOS::TextEntryStart(const InputTextEntryRequests::VirtualKeyboardOptions& options)
    {
        UIWindow* foundWindow = nil;        
#if defined(__IPHONE_13_0) || defined(__TVOS_13_0)
        if(@available(iOS 13.0, tvOS 13.0, *))
        {
            NSArray* windows = [[UIApplication sharedApplication] windows];
            for (UIWindow* window in windows)
            {
                if (window.isKeyWindow)
                {
                    foundWindow = window;
                    break;
                }
            }
        }
#else
        foundWindow = [[UIApplication sharedApplication] keyWindow];
#endif
        
        // Get the application's root view.
        UIView* rootView = foundWindow ? foundWindow.rootViewController.view : nullptr;
        if (!rootView)
        {
            return;
        }

        // Add the text field to the root view.
        [rootView addSubview: m_textField];

        // On iOS we must set m_activeTextFieldNormalizedBottomY before showing the virtual keyboard
        // by calling becomeFirstResponder, which then sends a UIKeyboardWillChangeFrameNotification.
        m_textFieldDelegate->m_activeTextFieldNormalizedBottomY = options.m_normalizedMinY;

        [m_textField becomeFirstResponder];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboardiOS::TextEntryStop()
    {
        // On iOS we must set m_activeTextFieldNormalizedBottomY before hiding the virtual keyboard
        // by calling resignFirstResponder, which then sends a UIKeyboardWillChangeFrameNotification.
        m_textFieldDelegate->m_activeTextFieldNormalizedBottomY = 0.0f;

        [m_textField resignFirstResponder];
        [m_textField removeFromSuperview];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboardiOS::TickInputDevice()
    {
        // The ios event loop has just been pumped in InputSystemComponentIos::PreTickInputDevices,
        // so we now just need to process any raw events that have been queued since the last frame
        ProcessRawEventQueues();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class IosDeviceVirtualKeyboardImplFactory
        : public InputDeviceVirtualKeyboard::ImplementationFactory
    {
    public:
        AZStd::unique_ptr<InputDeviceVirtualKeyboard::Implementation> Create(InputDeviceVirtualKeyboard& inputDevice) override
        {
            return AZStd::make_unique<InputDeviceVirtualKeyboardiOS>(inputDevice);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeUISystemComponent::InitializeDeviceVirtualKeyboardImplentationFactory()
    {
        m_deviceVirtualKeyboardImplFactory = AZStd::make_unique<IosDeviceVirtualKeyboardImplFactory>();
        AZ::Interface<InputDeviceVirtualKeyboard::ImplementationFactory>::Register(m_deviceVirtualKeyboardImplFactory.get());
    }
} // namespace AzFramework
