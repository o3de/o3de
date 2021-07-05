/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/NativeUI/NativeUISystemComponent.h>

#include <AzCore/Math/MathUtils.h>
#include <AzCore/PlatformIncl.h>

namespace
{
    // Constants that control the dialog box layout.
    const int MAX_DIALOG_WIDTH = 400;
    const int MAX_DIALOG_HEIGHT = 300;
    const int MAX_BUTTON_WIDTH = 100;
    const int MIN_BUTTON_WIDTH = 20;
    const int DIALOG_LEFT_MARGIN = 15;
    const int DIALOG_RIGHT_MARGIN = 15;
    const int DIALOG_TOP_MARGIN = 15;
    const int DIALOG_BOTTOM_MARGIN = 15;
    const int MESSAGE_WIDTH_PADDING = 50;
    const int MESSAGE_HEIGHT_PADDING = 20;
    const int MESSAGE_TO_BUTTON_Y_DELTA = 20;
    const int BUTTON_TO_BUTTON_X_DELTA = 20;
    const int BUTTON_TO_BUTTON_Y_DELTA = 20;
    const int CHARACTER_WIDTH = 4;
    const int LINE_HEIGHT = 8;
    const int BUTTON_HEIGHT = 10;
    const int MAX_ITEMS = 11; // Message + a maximum of 10 buttons
    const int WINDOW_CLASS = 0xFFFF;
    const int BUTTONS_ITEM_CLASS = 0x0080;
    const int STATIC_ITEM_CLASS = 0x0082;
    
    // Windows expects the data in this structure to be in a specific order and it should be DWORD aligned.
    struct ItemTemplate
    {
        // The template for each item in the dialog box.
        DLGITEMTEMPLATE m_itemTemplate;
        // Each item has a class type associated with it. Eg: 0x0080 for buttons, 0x0081 for edit controls etc.,
        WORD m_itemClass[2];
        // Windows expects a title field for each item at init even if we set the text later right before it's displayed.
        // The size is set to 2 because Windows expects the structure to be DWORD aligned.
        WCHAR m_padding[2];
        // Not used but Windows still looks for it.
        WORD m_creationData;
    };

    // Windows expects the data in this structure to be in a specific order and it should be WORD aligned
    struct DlgTemplate
    {
        // The template for the dialog.
        DLGTEMPLATE m_dlgTemplate;
        // The menu type.
        WORD m_menu;
        // The class type.
        WORD m_dialogClass;
        // The title of the dialog box. We set the title right before displaying the dialog but Windows expects this.
        WCHAR m_title;
        
        // Template for all items.
        ItemTemplate m_itemTemplates[MAX_ITEMS];
    };

    struct DlgInfo
    {
        // Pass info about the dialog to the callback.
        AZStd::string m_title;
        AZStd::string m_message;
        AZStd::vector<AZStd::string> m_options;
        int m_buttonChosen;
    };

    struct ButtonDimensions
    {
        // Dimensions of a button.
        int m_width;
        int m_height;
    };

    struct MessageDimensions
    {
        // Dimensions of a message.
        int m_width;
        int m_height;
    };

    struct ButtonRow
    {
        // Info about a row of buttons.
        int m_numButtons;
        int m_width;
    };

    INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
    {
        static DlgInfo* info = nullptr;

        switch (uiMsg)
        {
        case WM_INITDIALOG:
        {
            // Set the text for window title, message, buttons.
            info = (DlgInfo*)lParam;
            SetWindowText(hDlg, info->m_title.c_str());
            SetWindowText(GetDlgItem(hDlg, 0), info->m_message.c_str());
            for (int i = 0; i < info->m_options.size(); i++)
            {
                SetWindowText(GetDlgItem(hDlg, i + 1), info->m_options[i].c_str());
            }
        }
            break;
        case WM_COMMAND:
        {
            // Store chosen option and close the dialog.
            if (info != nullptr && LOWORD(wParam) > 0 && LOWORD(wParam) <= info->m_options.size())
            {
                info->m_buttonChosen = LOWORD(wParam);
                EndDialog(hDlg, 0);
            }
            return TRUE;
        }
        default:
            return FALSE;
        }
        return TRUE;
    }

    MessageDimensions ComputeMessageDimensions(const AZStd::string& message)
    {
        MessageDimensions messageDimensions;
        messageDimensions.m_width = messageDimensions.m_height = 0;

        int numLines = 0;
        int searchPos = 0;

        // Max usable space in the dialog.
        int maxMessageWidth = MAX_DIALOG_WIDTH - DIALOG_LEFT_MARGIN - DIALOG_RIGHT_MARGIN + MESSAGE_WIDTH_PADDING;
        int maxMessageHeight = MAX_DIALOG_HEIGHT - DIALOG_TOP_MARGIN - DIALOG_BOTTOM_MARGIN + MESSAGE_HEIGHT_PADDING;

        // Compute the number of lines. Look for new line characters.
        do 
        {
            numLines++;
            int lineLength = 0;
            int prevNewLine = searchPos;
            searchPos = static_cast<int>(message.find('\n', searchPos));

            if (searchPos == AZStd::string::npos)
            {
                lineLength = static_cast<int>(message.size() - prevNewLine - 1);
            }
            else
            {
                lineLength = searchPos - prevNewLine;
                searchPos++;
            }

            // If the line width is greater than the max width for a line, we add additional lines.
            // numLines is later used to compute the height of the message box.
            int lineWidth = lineLength * CHARACTER_WIDTH;
            numLines += (lineWidth / maxMessageWidth);

            // Keep track of the max width for a line. This will be the width of the message.
            if (lineWidth > messageDimensions.m_width)
            {
                messageDimensions.m_width = lineWidth;
            }
        } while (searchPos != AZStd::string::npos);

        messageDimensions.m_height = numLines * LINE_HEIGHT;

        messageDimensions.m_width = AZ::GetClamp(messageDimensions.m_width + MESSAGE_WIDTH_PADDING, 0, maxMessageWidth);
        messageDimensions.m_height = AZ::GetClamp(messageDimensions.m_height + MESSAGE_HEIGHT_PADDING, 0, maxMessageHeight);

        return messageDimensions;
    }

    void ComputeButtonDimensions(const AZStd::vector<AZStd::string>& options, AZStd::vector<ButtonDimensions>& buttonDimensions)
    {
        // Compute width of each button. We don't allow multiple lines for buttons.
        for (int i = 0; i < options.size(); i++)
        {
            ButtonDimensions dimensions;
            int length = static_cast<int>(options[i].size() - 1);
            dimensions.m_height = BUTTON_HEIGHT;
            dimensions.m_width = MIN_BUTTON_WIDTH + (length * CHARACTER_WIDTH);
            dimensions.m_width = AZ::GetClamp(dimensions.m_width, MIN_BUTTON_WIDTH, MAX_BUTTON_WIDTH);
            buttonDimensions.push_back(dimensions);
        }
    }

    void AddButtonRow(AZStd::vector<ButtonRow>& rows, int numButtons, int rowWidth)
    {
        ButtonRow buttonRow;
        buttonRow.m_numButtons = numButtons;
        buttonRow.m_width = rowWidth;
        rows.push_back(buttonRow);
    }

    void ComputeButtonRows(const AZStd::vector<ButtonDimensions>& buttonDimensions, AZStd::vector<ButtonRow>& rows)
    {
        // Max usable width in the dialog box.
        int maxButtonRowWidth = MAX_DIALOG_WIDTH - DIALOG_LEFT_MARGIN - DIALOG_RIGHT_MARGIN;
        
        int rowWidth = 0;
        int buttonRowIndex = 0;

        // Compute the number of buttons we can put on a single row.
        for (int i = 0; i < buttonDimensions.size(); i++, buttonRowIndex++)
        {
            int nextButtonWidth = buttonDimensions[i].m_width;
            if (buttonRowIndex > 0)
            {
                nextButtonWidth += BUTTON_TO_BUTTON_X_DELTA;
            }
            
            // We can't add another button to this row. Move it to the next row.
            if (rowWidth + nextButtonWidth > maxButtonRowWidth)
            {
                // Store this button row.
                AddButtonRow(rows, buttonRowIndex, rowWidth);
                rowWidth = nextButtonWidth;
                buttonRowIndex = 0;
            }
            else
            {
                rowWidth += nextButtonWidth;
            }
        }

        // Add the last button row.
        AddButtonRow(rows, buttonRowIndex, rowWidth);
    }
}

namespace AZ
{
    namespace NativeUI
    {
        AZStd::string NativeUISystem::DisplayBlockingDialog(const AZStd::string& title, const AZStd::string& message, const AZStd::vector<AZStd::string>& options) const
        {
            if (m_mode == NativeUI::Mode::DISABLED)
            {
                return {};
            }

            if (options.size() >= MAX_ITEMS)
            {
                AZ_Assert(false, "Cannot create dialog box with more than %d buttons", (MAX_ITEMS - 1));
                return "";
            }

            MessageDimensions messageDimensions;

            // Compute all the dimensions.
            messageDimensions = ComputeMessageDimensions(message);

            AZStd::vector<ButtonDimensions> buttonDimensions;
            ComputeButtonDimensions(options, buttonDimensions);

            AZStd::vector<ButtonRow> buttonRows;
            ComputeButtonRows(buttonDimensions, buttonRows);

            int buttonRowsWidth = 0;
            for (int i = 0; i < buttonRows.size(); i++)
            {
                if (buttonRows[i].m_width > buttonRowsWidth)
                {
                    buttonRowsWidth = buttonRows[i].m_width;
                }
            }

            int buttonRowsHeight = static_cast<int>((buttonRows.size() * BUTTON_HEIGHT) + ((buttonRows.size() - 1) * BUTTON_TO_BUTTON_Y_DELTA));

            int dialogWidth = AZ::GetMax(buttonRowsWidth, messageDimensions.m_width) + DIALOG_LEFT_MARGIN + DIALOG_RIGHT_MARGIN;
            int dialogHeight;

            int totalHeight = DIALOG_TOP_MARGIN + messageDimensions.m_height + MESSAGE_TO_BUTTON_Y_DELTA + buttonRowsHeight + DIALOG_BOTTOM_MARGIN;
            if (totalHeight <= MAX_DIALOG_HEIGHT)
            {
                dialogHeight = totalHeight;
            }
            else
            {
                messageDimensions.m_height -= (totalHeight - MAX_DIALOG_HEIGHT);
                dialogHeight = MAX_DIALOG_HEIGHT;
            }

            // Create the template with the computed dimensions.
            DlgTemplate dlgTemplate;

            dlgTemplate.m_dlgTemplate.style = DS_CENTER | WS_POPUP | WS_CAPTION | WS_VISIBLE | WS_SYSMENU | WS_THICKFRAME;
            dlgTemplate.m_dlgTemplate.dwExtendedStyle = WS_EX_TOPMOST;
            dlgTemplate.m_dlgTemplate.cdit = static_cast<WORD>(options.size() + 1);
            dlgTemplate.m_dlgTemplate.x = dlgTemplate.m_dlgTemplate.y = 0;
            dlgTemplate.m_dlgTemplate.cx = static_cast<short>(dialogWidth);
            dlgTemplate.m_dlgTemplate.cy = static_cast<short>(dialogHeight);
            dlgTemplate.m_menu = dlgTemplate.m_dialogClass = 0;
            MultiByteToWideChar(CP_ACP, 0, "", -1, &dlgTemplate.m_title, 1);

            int dialogUsableWidth = dialogWidth - DIALOG_LEFT_MARGIN - DIALOG_RIGHT_MARGIN;

            dlgTemplate.m_itemTemplates[0].m_itemTemplate.style = SS_LEFT | WS_CHILD | WS_VISIBLE;
            dlgTemplate.m_itemTemplates[0].m_itemTemplate.dwExtendedStyle = 0;
            dlgTemplate.m_itemTemplates[0].m_itemTemplate.x = static_cast<short>(DIALOG_LEFT_MARGIN + (dialogUsableWidth - messageDimensions.m_width) / 2);
            dlgTemplate.m_itemTemplates[0].m_itemTemplate.y = DIALOG_TOP_MARGIN;
            dlgTemplate.m_itemTemplates[0].m_itemTemplate.cx = static_cast<short>(messageDimensions.m_width);
            dlgTemplate.m_itemTemplates[0].m_itemTemplate.cy = static_cast<short>(messageDimensions.m_height);
            dlgTemplate.m_itemTemplates[0].m_itemTemplate.id = 0;
            dlgTemplate.m_itemTemplates[0].m_itemClass[0] = WINDOW_CLASS;
            dlgTemplate.m_itemTemplates[0].m_itemClass[1] = STATIC_ITEM_CLASS;
            dlgTemplate.m_itemTemplates[0].m_creationData = 0;
            MultiByteToWideChar(CP_ACP, 0, " ", -1, dlgTemplate.m_itemTemplates[0].m_padding, 2);

            int buttonXOffset = 0;
            int buttonYOffset = DIALOG_TOP_MARGIN + messageDimensions.m_height + MESSAGE_TO_BUTTON_Y_DELTA;
            for (int i = 0, buttonRow = 0, buttonRowIndex = 0; i < options.size(); i++, buttonRowIndex++)
            {
                if (buttonRowIndex >= buttonRows[buttonRow].m_numButtons)
                {
                    buttonRowIndex = 0;
                    buttonRow++;
                    buttonXOffset = 0;
                    buttonYOffset += BUTTON_HEIGHT + BUTTON_TO_BUTTON_Y_DELTA;
                }

                dlgTemplate.m_itemTemplates[i + 1].m_itemTemplate.style = BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP;
                dlgTemplate.m_itemTemplates[i + 1].m_itemTemplate.dwExtendedStyle = 0;
                dlgTemplate.m_itemTemplates[i + 1].m_itemTemplate.x = static_cast<short>(DIALOG_LEFT_MARGIN + (dialogUsableWidth - buttonRows[buttonRow].m_width) / 2 + buttonXOffset);
                dlgTemplate.m_itemTemplates[i + 1].m_itemTemplate.y = static_cast<short>(buttonYOffset);
                dlgTemplate.m_itemTemplates[i + 1].m_itemTemplate.cx = static_cast<short>(buttonDimensions[i].m_width);
                dlgTemplate.m_itemTemplates[i + 1].m_itemTemplate.cy = static_cast<short>(buttonDimensions[i].m_height);
                dlgTemplate.m_itemTemplates[i + 1].m_itemTemplate.id = static_cast<WORD>(i + 1);
                dlgTemplate.m_itemTemplates[i + 1].m_itemClass[0] = WINDOW_CLASS;
                dlgTemplate.m_itemTemplates[i + 1].m_itemClass[1] = BUTTONS_ITEM_CLASS;
                dlgTemplate.m_itemTemplates[i + 1].m_creationData = 0;
                MultiByteToWideChar(CP_ACP, 0, " ", -1, dlgTemplate.m_itemTemplates[i + 1].m_padding, 2);

                buttonXOffset += (buttonDimensions[i].m_width + BUTTON_TO_BUTTON_X_DELTA);
            }

            // Pass the title, message, and options to the callback so we can set the text there.
            DlgInfo info;
            info.m_title = title;
            info.m_message = message;
            info.m_options = options;
            info.m_buttonChosen = -1;

            LRESULT ret = DialogBoxIndirectParam(GetModuleHandle(NULL), (DLGTEMPLATE*)&dlgTemplate, GetDesktopWindow(), DlgProc, (LPARAM)&info);
            AZ_UNUSED(ret);
            AZ_Assert(ret != -1, "Error displaying native UI dialog.");

            if (info.m_buttonChosen > 0 && info.m_buttonChosen <= options.size())
            {
                return options[info.m_buttonChosen - 1];
            }

            return "";
        }
    }
}
