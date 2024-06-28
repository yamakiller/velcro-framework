#include <vcore/native_ui/native_ui_system.h>

#include <vcore/math/math_utils.h>
#include <vcore/platform_incl.h>
#include <vcore/std/string/conversions.h>
#include <vcore/std/containers/vector.h>

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
        DLGITEMTEMPLATE ItemTemplate;
        // Each item has a class type associated with it. Eg: 0x0080 for buttons, 0x0081 for edit controls etc.,
        WORD ItemClass[2];
        // Windows expects a title field for each item at init even if we set the text later right before it's displayed.
        // The size is set to 2 because Windows expects the structure to be DWORD aligned.
        WCHAR Padding[2];
        // Not used but Windows still looks for it.
        WORD CreationData;
    };

      // Windows expects the data in this structure to be in a specific order and it should be WORD aligned
    struct DlgTemplate
    {
        // The template for the dialog.
        DLGTEMPLATE DlgTemplate;
        // The menu type.
        WORD Menu;
        // The class type.
        WORD DialogClass;
        // The title of the dialog box. We set the title right before displaying the dialog but Windows expects this.
        WCHAR Title;
        
        // Template for all items.
        ItemTemplate ItemTemplates[MAX_ITEMS];
    };

    struct DlgInfo
    {
        // Pass info about the dialog to the callback.
        VStd::string Title;
        VStd::string Message;
        VStd::vector<VStd::string> Options;
        int ButtonChosen;
    };

    struct ButtonDimensions
    {
        // Dimensions of a button.
        int Width;
        int Height;
    };

    struct MessageDimensions
    {
        // Dimensions of a message.
        int Width;
        int Height;
    };

    struct ButtonRow
    {
        // Info about a row of buttons.
        int NumButtons;
        int Width;
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
            VStd::wstring titleW;
            VStd::to_wstring(titleW, info->Title.c_str());
            SetWindowTextW(hDlg, titleW.c_str());
            VStd::wstring messageW;
            VStd::to_wstring(messageW, info->Message.c_str());
            SetWindowTextW(GetDlgItem(hDlg, 0), messageW.c_str());
            for (int i = 0; i < info->Options.size(); i++)
            {
                VStd::wstring optionW;
                VStd::to_wstring(optionW, info->Options[i].c_str());
                SetWindowTextW(GetDlgItem(hDlg, i + 1), optionW.c_str());
            }
        }
            break;
        case WM_COMMAND:
        {
            // Store chosen option and close the dialog.
            if (info != nullptr && LOWORD(wParam) > 0 && LOWORD(wParam) <= info->Options.size())
            {
                info->ButtonChosen = LOWORD(wParam);
                EndDialog(hDlg, 0);
            }
            return TRUE;
        }
        default:
            return FALSE;
        }
        return TRUE;
    }

    MessageDimensions ComputeMessageDimensions(const VStd::string& message)
    {
        MessageDimensions messageDimensions;
        messageDimensions.Width = messageDimensions.Height = 0;

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

            if (searchPos == VStd::string::npos)
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
            if (lineWidth > messageDimensions.Width)
            {
                messageDimensions.Width = lineWidth;
            }
        } while (searchPos != VStd::string::npos);

        messageDimensions.Height = numLines * LINE_HEIGHT;

        messageDimensions.Width = V::GetClamp(messageDimensions.Width + MESSAGE_WIDTH_PADDING, 0, maxMessageWidth);
        messageDimensions.Height = V::GetClamp(messageDimensions.Height + MESSAGE_HEIGHT_PADDING, 0, maxMessageHeight);

        return messageDimensions;
    }

    void ComputeButtonDimensions(const VStd::vector<VStd::string>& options, VStd::vector<ButtonDimensions>& buttonDimensions)
    {
        // Compute width of each button. We don't allow multiple lines for buttons.
        for (int i = 0; i < options.size(); i++)
        {
            ButtonDimensions dimensions;
            int length = static_cast<int>(options[i].size() - 1);
            dimensions.Height = BUTTON_HEIGHT;
            dimensions.Width = MIN_BUTTON_WIDTH + (length * CHARACTER_WIDTH);
            dimensions.Width = V::GetClamp(dimensions.Width, MIN_BUTTON_WIDTH, MAX_BUTTON_WIDTH);
            buttonDimensions.push_back(dimensions);
        }
    }

    void AddButtonRow(VStd::vector<ButtonRow>& rows, int numButtons, int rowWidth)
    {
        ButtonRow buttonRow;
        buttonRow.NumButtons = numButtons;
        buttonRow.Width = rowWidth;
        rows.push_back(buttonRow);
    }

    void ComputeButtonRows(const VStd::vector<ButtonDimensions>& buttonDimensions, VStd::vector<ButtonRow>& rows)
    {
        // Max usable width in the dialog box.
        int maxButtonRowWidth = MAX_DIALOG_WIDTH - DIALOG_LEFT_MARGIN - DIALOG_RIGHT_MARGIN;
        
        int rowWidth = 0;
        int buttonRowIndex = 0;

        // Compute the number of buttons we can put on a single row.
        for (int i = 0; i < buttonDimensions.size(); i++, buttonRowIndex++)
        {
            int nextButtonWidth = buttonDimensions[i].Width;
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

namespace V
{
    namespace NativeUI
    {
        VStd::string NativeUISystem::DisplayBlockingDialog(const VStd::string& title, const VStd::string& message, const VStd::vector<VStd::string>& options) const
        {
            if (m_mode == NativeUI::Mode::DISABLED)
            {
                return {};
            }

            if (options.size() >= MAX_ITEMS)
            {
                V_Assert(false, "Cannot create dialog box with more than %d buttons", (MAX_ITEMS - 1));
                return "";
            }

            MessageDimensions messageDimensions;

            // Compute all the dimensions.
            messageDimensions = ComputeMessageDimensions(message);

            VStd::vector<ButtonDimensions> buttonDimensions;
            ComputeButtonDimensions(options, buttonDimensions);

            VStd::vector<ButtonRow> buttonRows;
            ComputeButtonRows(buttonDimensions, buttonRows);

            int buttonRowsWidth = 0;
            for (int i = 0; i < buttonRows.size(); i++)
            {
                if (buttonRows[i].Width > buttonRowsWidth)
                {
                    buttonRowsWidth = buttonRows[i].Width;
                }
            }

            int buttonRowsHeight = static_cast<int>((buttonRows.size() * BUTTON_HEIGHT) + ((buttonRows.size() - 1) * BUTTON_TO_BUTTON_Y_DELTA));

            int dialogWidth = V::GetMax(buttonRowsWidth, messageDimensions.Width) + DIALOG_LEFT_MARGIN + DIALOG_RIGHT_MARGIN;
            int dialogHeight;

            int totalHeight = DIALOG_TOP_MARGIN + messageDimensions.Height + MESSAGE_TO_BUTTON_Y_DELTA + buttonRowsHeight + DIALOG_BOTTOM_MARGIN;
            if (totalHeight <= MAX_DIALOG_HEIGHT)
            {
                dialogHeight = totalHeight;
            }
            else
            {
                messageDimensions.Height -= (totalHeight - MAX_DIALOG_HEIGHT);
                dialogHeight = MAX_DIALOG_HEIGHT;
            }

            // Create the template with the computed dimensions.
            DlgTemplate dlgTemplate;

            dlgTemplate.DlgTemplate.style = DS_CENTER | WS_POPUP | WS_CAPTION | WS_VISIBLE | WS_SYSMENU | WS_THICKFRAME;
            dlgTemplate.DlgTemplate.dwExtendedStyle = WS_EX_TOPMOST;
            dlgTemplate.DlgTemplate.cdit = static_cast<WORD>(options.size() + 1);
            dlgTemplate.DlgTemplate.x = dlgTemplate.DlgTemplate.y = 0;
            dlgTemplate.DlgTemplate.cx = static_cast<short>(dialogWidth);
            dlgTemplate.DlgTemplate.cy = static_cast<short>(dialogHeight);
            dlgTemplate.Menu = dlgTemplate.DialogClass = 0;
            MultiByteToWideChar(CP_ACP, 0, "", -1, &dlgTemplate.Title, 1);

            int dialogUsableWidth = dialogWidth - DIALOG_LEFT_MARGIN - DIALOG_RIGHT_MARGIN;

            dlgTemplate.ItemTemplates[0].ItemTemplate.style = SS_LEFT | WS_CHILD | WS_VISIBLE;
            dlgTemplate.ItemTemplates[0].ItemTemplate.dwExtendedStyle = 0;
            dlgTemplate.ItemTemplates[0].ItemTemplate.x = static_cast<short>(DIALOG_LEFT_MARGIN + (dialogUsableWidth - messageDimensions.Width) / 2);
            dlgTemplate.ItemTemplates[0].ItemTemplate.y = DIALOG_TOP_MARGIN;
            dlgTemplate.ItemTemplates[0].ItemTemplate.cx = static_cast<short>(messageDimensions.Width);
            dlgTemplate.ItemTemplates[0].ItemTemplate.cy = static_cast<short>(messageDimensions.Height);
            dlgTemplate.ItemTemplates[0].ItemTemplate.id = 0;
            dlgTemplate.ItemTemplates[0].ItemClass[0] = WINDOW_CLASS;
            dlgTemplate.ItemTemplates[0].ItemClass[1] = STATIC_ITEM_CLASS;
            dlgTemplate.ItemTemplates[0].CreationData = 0;
            MultiByteToWideChar(CP_ACP, 0, " ", -1, dlgTemplate.ItemTemplates[0].Padding, 2);

            int buttonXOffset = 0;
            int buttonYOffset = DIALOG_TOP_MARGIN + messageDimensions.Height + MESSAGE_TO_BUTTON_Y_DELTA;
            for (int i = 0, buttonRow = 0, buttonRowIndex = 0; i < options.size(); i++, buttonRowIndex++)
            {
                if (buttonRowIndex >= buttonRows[buttonRow].NumButtons)
                {
                    buttonRowIndex = 0;
                    buttonRow++;
                    buttonXOffset = 0;
                    buttonYOffset += BUTTON_HEIGHT + BUTTON_TO_BUTTON_Y_DELTA;
                }

                dlgTemplate.ItemTemplates[i + 1].ItemTemplate.style = BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP;
                dlgTemplate.ItemTemplates[i + 1].ItemTemplate.dwExtendedStyle = 0;
                dlgTemplate.ItemTemplates[i + 1].ItemTemplate.x = static_cast<short>(DIALOG_LEFT_MARGIN + (dialogUsableWidth - buttonRows[buttonRow].Width) / 2 + buttonXOffset);
                dlgTemplate.ItemTemplates[i + 1].ItemTemplate.y = static_cast<short>(buttonYOffset);
                dlgTemplate.ItemTemplates[i + 1].ItemTemplate.cx = static_cast<short>(buttonDimensions[i].Width);
                dlgTemplate.ItemTemplates[i + 1].ItemTemplate.cy = static_cast<short>(buttonDimensions[i].Height);
                dlgTemplate.ItemTemplates[i + 1].ItemTemplate.id = static_cast<WORD>(i + 1);
                dlgTemplate.ItemTemplates[i + 1].ItemClass[0] = WINDOW_CLASS;
                dlgTemplate.ItemTemplates[i + 1].ItemClass[1] = BUTTONS_ITEM_CLASS;
                dlgTemplate.ItemTemplates[i + 1].CreationData = 0;
                MultiByteToWideChar(CP_ACP, 0, " ", -1, dlgTemplate.ItemTemplates[i + 1].Padding, 2);

                buttonXOffset += (buttonDimensions[i].Width + BUTTON_TO_BUTTON_X_DELTA);
            }

            // Pass the title, message, and options to the callback so we can set the text there.
            DlgInfo info;
            info.Title = title;
            info.Message = message;
            info.Options = options;
            info.ButtonChosen = -1;

            LRESULT ret = DialogBoxIndirectParam(GetModuleHandle(NULL), (DLGTEMPLATE*)&dlgTemplate, GetDesktopWindow(), DlgProc, (LPARAM)&info);
            V_UNUSED(ret);
            V_Assert(ret != -1, "Error displaying native UI dialog.");

            if (info.ButtonChosen > 0 && info.ButtonChosen <= options.size())
            {
                return options[info.ButtonChosen - 1];
            }

            return "";
        }
    } // namespace NativeUI
} // namespace  V