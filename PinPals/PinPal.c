#define _CRT_SECURE_NO_WARNINGS
#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0600



#include <windows.h>
#include <windowsx.h>
#include "Resource.h"
#include <string.h>
#include "sqlite3.h"
#include <stdio.h>
#include <commctrl.h>

#include "PinPal.h"
#include "dbg.h"
#pragma comment(lib, "Comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


#define CON 0
//control constants
#define ID_NEW_NOTE 2999
#define ID_TEXT 3001
#define ID_LABEL 3002
#define ID_PIN_BUTTON 3003
#define ID_NEW_NOTE_BUTTON 3004
#define ID_SHOW_ALL_NOTES_BUTTON 3005
#define ID_CLOSE_ALL_BUTTON 3006
#define ID_SAVE_NOTE_BUTTON 3007
#define ID_OPTIONS_BUTTON 3008
#define ID_DELETE_NOTE_BUTTON 3009
#define ID_NOTE_TITLE 3010

//Per note offsets for cbWndExtra
#define NOTE_EDIT_HANDLE 0
#define NOTE_TOPMOST_STATE sizeof(LONG_PTR)
#define NOTE_ID (sizeof(LONG_PTR) * 2)
#define NOTE_TITLE_HANDLE (sizeof(LONG_PTR) * 3)
#define NOTE_COLOR (sizeof(LONG_PTR) * 4)

//cbWndExtra for Main Window
#define NEW_NOTE_BUTTON_HANDLE sizeof(LONG_PTR)
#define MAIN_WINDOW_SCROLL_STATE (sizeof(LONG_PTR) * 2)


//custom messages
#define WM_APP_NOTE_CLOSED (WM_APP + 1)
#define WM_APP_NOTE_DELETED (WM_APP + 2)
#define WM_APP_NOTE_EDIT (WM_APP + 3)
#define WM_APP_SAVE (WM_APP + 4)
#define WM_APP_CALL_UPDATE_WINDOW (WM_APP + 5)


//constants
const wchar_t windowClass[] = L"myWindowClass";
const wchar_t myNoteClass[] = L"myNoteclass";
const wchar_t windowTitle[] = L"PinPals";
#define NOTE_MARGIN 25
#define NOTE_HEIGHT 100
#define NOTE_WIDTH 200
#define BUTTON_HEIGHT 50
#define NUMBER_OF_NOTE_COLORS 6
#define COLOR_HEIGHT 50
#define COLOR_WIDTH 50


//Globals
sqlite3 *db = NULL;
HWND hmainWindowHandle;
int noteCount = 0;
int scrollPos = 0;
struct Note* notes_true = NULL;
HICON hPlusIcon;
HICON hDeleteIcon;
HICON hDeleteMainWindowIcon;
HICON hOptionIcon;
HICON hPinIcon;
HFONT hMainWindowContentFont;


RECT g_noteColorRects[NUMBER_OF_NOTE_COLORS];
COLORREF noteColors[NUMBER_OF_NOTE_COLORS] = {
    RGB(245, 230, 120),
    RGB(255, 210, 70), 
    RGB(100, 200, 190),
    RGB(255, 200, 150),  
    RGB(180, 245, 200),
    RGB(240, 130, 120)
};


struct ScrollState {
    int scrollPosY; //current vertical scroll position
    int scrollPosX; //current horizontal scroll position
    int contentHeight; //visible height of scrollable content
    int viewPortHeight; //visible area height
};


LRESULT CALLBACK NoteWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{   
    int topMostState;
    
    switch (msg)
    {
    case WM_CREATE:
    {
        SetWindowLongPtr(hwnd, NOTE_TOPMOST_STATE, 0);
        SetWindowLongPtr(hwnd, NOTE_COLOR, (LONG_PTR)noteColors[0]);
        HWND notePin = CreateWindowEx(
            0,L"BUTTON",L"Pin",WS_CHILD | WS_VISIBLE| BS_OWNERDRAW,
            0,0,50,50,hwnd,(HMENU)ID_PIN_BUTTON,GetModuleHandle(0),NULL
        );


        HWND newNoteButton = CreateWindowEx(
            0, L"BUTTON", L"New Note", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            50, 0, 50, 50, hwnd, (HMENU)ID_NEW_NOTE_BUTTON, GetModuleHandle(0), NULL
        );
        HWND showAllNotes = CreateWindowEx(
            0, L"BUTTON", L"Pin Board", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            100, 0, 50, 50, hwnd, (HMENU)ID_SHOW_ALL_NOTES_BUTTON, GetModuleHandle(0), NULL
        );

        HWND deleteNote = CreateWindowEx(
            0, L"BUTTON", L"Del", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            150, 0, 50, 50, hwnd, (HMENU)ID_DELETE_NOTE_BUTTON, GetModuleHandle(0), 0
        );

        HWND titleEdit = CreateWindowEx(
            0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            50, 0, 300, 20, hwnd, (HMENU)ID_NOTE_TITLE, GetModuleHandle(NULL),
            NULL
        );
        SendMessage(titleEdit, EM_SETCUEBANNER, FALSE, (LPARAM)L"Title");



        HWND textArea = CreateWindowEx(
            0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE |ES_AUTOVSCROLL | WS_VSCROLL | ES_WANTRETURN,
            0, 0, 100, 100, hwnd, (HMENU)ID_TEXT, GetModuleHandle(NULL),
            NULL
        );

        HFONT titleFont = CreateFont(
            -24,                // Height 
            0, 0, 0,            // Width, escapement, orientation
            FW_NORMAL,          // Weight
            FALSE, FALSE, FALSE,// Italic, underline, strikeout
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI"          // Font faceres
        );

        HFONT noteEditFont = CreateFont(
            -18,                // Height 
            0, 0, 0,            // Width, escapement, orientation
            FW_NORMAL,          // Weight
            FALSE, FALSE, FALSE,// Italic, underline, strikeout
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Calibri"          // Font faceres
        );



        SendMessage(textArea, WM_SETFONT, (WPARAM)noteEditFont, TRUE);
        SendMessage(titleEdit, WM_SETFONT, (WPARAM)titleFont, TRUE);
        SetWindowLongPtr(hwnd, NOTE_EDIT_HANDLE, (LONG_PTR)textArea);
        SetWindowLongPtr(hwnd, NOTE_TITLE_HANDLE, (LONG_PTR)titleEdit);
        calculateColorRectPosition(hwnd);


    }break;

    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;

        if (dis->CtlID == ID_DELETE_NOTE_BUTTON)
        {
            HDC hdc = dis->hDC;
            RECT rc = dis->rcItem;

            int iconSize = 24;
            int btnWidth = rc.right - rc.left;
            int btnHeight = rc.bottom - rc.top;

            int iconX = rc.left + (btnWidth - iconSize) / 2;
            int iconY = rc.top + (btnHeight - iconSize) / 2;

            DrawIconEx(hdc, iconX, iconY, hDeleteIcon,
                iconSize, iconSize,
                0,         
                0,        
                DI_NORMAL);

            return TRUE;
        }

        if (dis->CtlID == ID_SHOW_ALL_NOTES_BUTTON)
        {
            HDC hdc = dis->hDC;
            RECT rc = dis->rcItem;

            HDC parentDC = GetDC(GetParent(dis->hwndItem));
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;

            BitBlt(hdc, 0, 0, w, h, parentDC, rc.left, rc.top, SRCCOPY);

            ReleaseDC(GetParent(dis->hwndItem), parentDC);

            COLORREF normalColor = RGB(230, 230, 230);
            COLORREF hoverColor = RGB(210, 210, 210);
            COLORREF clickColor = RGB(180, 180, 180);
            COLORREF textColor = RGB(30, 30, 30);

            COLORREF bg = normalColor;

            if (dis->itemState & ODS_SELECTED)
                bg = clickColor;
            else if (dis->itemState & ODS_HOTLIGHT)
                bg = hoverColor;

            HBRUSH brush = CreateSolidBrush(bg);

            HPEN pen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
            HPEN oldPen = SelectObject(hdc, pen);
            HBRUSH oldBrush = SelectObject(hdc, brush);

            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 18, 18);

            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(brush);
            DeleteObject(pen);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, textColor);

            wchar_t text[64];
            GetWindowText(dis->hwndItem, text, 64);

            DrawText(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            return TRUE;
        }

        if (dis->CtlID == ID_NEW_NOTE_BUTTON)
        {
            HDC hdc = dis->hDC;
            RECT rc = dis->rcItem;

            HDC parentDC = GetDC(GetParent(dis->hwndItem));
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;

            BitBlt(hdc, 0, 0, w, h, parentDC, rc.left, rc.top, SRCCOPY);

            ReleaseDC(GetParent(dis->hwndItem), parentDC);

            COLORREF normalColor = RGB(230, 230, 230);
            COLORREF hoverColor = RGB(210, 210, 210);
            COLORREF clickColor = RGB(180, 180, 180);
            COLORREF textColor = RGB(30, 30, 30);

            COLORREF bg = normalColor;

            if (dis->itemState & ODS_SELECTED)
                bg = clickColor;
            else if (dis->itemState & ODS_HOTLIGHT)
                bg = hoverColor;

            HBRUSH brush = CreateSolidBrush(bg);

            HPEN pen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
            HPEN oldPen = SelectObject(hdc, pen);
            HBRUSH oldBrush = SelectObject(hdc, brush);

            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 18, 18);

            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(brush);
            DeleteObject(pen);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, textColor);

            wchar_t text[64];
            GetWindowText(dis->hwndItem, text, 64);

            DrawText(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            return TRUE;
        }

        if (dis->CtlID == ID_PIN_BUTTON)
        {
            HDC hdc = dis->hDC;
            RECT rc = dis->rcItem;
            
            BOOL isPressed = (dis->itemState & ODS_SELECTED);
            BOOL isFocused = (dis->itemState & ODS_FOCUS);
            BOOL isDisabled = (dis->itemState & ODS_DISABLED);
            BOOL isHot = (dis->itemState & ODS_HOTLIGHT);

            
           int isPinned = (int)GetWindowLongPtr(hwnd, NOTE_TOPMOST_STATE); //Pin state

            COLORREF bgNormal = isPinned ? RGB(100, 200, 100) : RGB(230, 230, 230);  // Green if pinned. Placeholder for now
            COLORREF bgHover = isPinned ? RGB(80, 180, 80) : RGB(210, 210, 210);
            COLORREF bgPressed = isPinned ? RGB(60, 160, 60) : RGB(180, 180, 180);
            COLORREF bgDisabled = RGB(180, 180, 180);

            COLORREF bg = bgNormal;

            if (isDisabled) bg = bgDisabled;
            else if (isPressed) bg = bgPressed;
            else if (isHot) bg = bgHover;

            HBRUSH brush = CreateSolidBrush(bg);
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(180,180,180));
            HPEN oldPen = SelectObject(hdc, pen);

            HBRUSH oldBrush = SelectObject(hdc, brush);

            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 20, 20);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(30,30,30));

            wchar_t text[256];
            GetWindowText(dis->hwndItem, text, 256);

            int iconSize = 24;  
            int btnWidth = rc.right - rc.left;
            int btnHeight = rc.bottom - rc.top;

            int iconX = rc.left + iconSize;
            int iconY = rc.top + (btnHeight - iconSize) / 2;

            DrawIconEx(hdc, iconX, iconY, hPinIcon,
                iconSize, iconSize,
                0,         
                0,      
                DI_NORMAL);

            RECT textRect = rc;
            textRect.left = iconX + 8;

            DrawText(hdc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(brush);
            DeleteObject(pen);
            return TRUE;
        }
    }
    break;


    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT windowRect;
        GetClientRect(hwnd, &windowRect);

        int windowHeight = windowRect.bottom - windowRect.top;
        int windowWidth = windowRect.right - windowRect.left;

        int totalColorPalleteWidth = NUMBER_OF_NOTE_COLORS * COLOR_WIDTH;
        int halfwayWindowXValue = (windowWidth - totalColorPalleteWidth) / 2;

        int colorYValue = windowHeight - 50;

        for (int i = 0; i < NUMBER_OF_NOTE_COLORS; i++)
        {
            HBRUSH brush = CreateSolidBrush(noteColors[i]);
            HBRUSH old = SelectObject(hdc, brush);

            RECT r = { halfwayWindowXValue, colorYValue, halfwayWindowXValue + 50, windowHeight };
            
            Rectangle(hdc, r.left, r.top, r.right, r.bottom);
            halfwayWindowXValue += 50;
            SelectObject(hdc, old);
            DeleteObject(brush);


        }
        EndPaint(hwnd, &ps);
    }break;

    case WM_ERASEBKGND:
    {
        COLORREF newBackgroundColor = GetWindowLongPtr(hwnd,NOTE_COLOR);
        PAINTSTRUCT ps;
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);

        HBRUSH hBrush = CreateSolidBrush(newBackgroundColor);
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);
        return 1; // background handled
    }

    case WM_LBUTTONDOWN:
    {
        POINT ptClick;
        ptClick.x = GET_X_LPARAM(lParam);
        ptClick.y = GET_Y_LPARAM(lParam);

        for (int i = 0; i < NUMBER_OF_NOTE_COLORS; i++) {

            if (PtInRect(&g_noteColorRects[i], ptClick))
            {
                SetWindowLongPtr(hwnd, NOTE_COLOR, (LONG_PTR)noteColors[i]);
                SendMessage(hmainWindowHandle, WM_APP_SAVE, (WPARAM)noteColors[i], (LPARAM)hwnd);
                SendMessage(hwnd, WM_ERASEBKGND, (WPARAM)noteColors[i], (LPARAM)hwnd);
                SendMessage(hwnd, WM_CTLCOLOREDIT,0,0);
                InvalidateRect(hwnd, NULL, 1);
                UpdateWindow(hwnd);
                break;
            }
        }
    }

    
    case WM_CTLCOLOREDIT:
    {    HWND hEdit = (HWND)lParam;
    

    // Check if it's the text area and if it's empty
    if (hEdit == GetDlgItem(hwnd, ID_TEXT))
    {
        int len = GetWindowTextLength(hEdit);
        if (len == 0)
        {
            HDC hdc = (HDC)wParam;
            
            SetTextColor(hdc, RGB(150, 150, 150)); // Gray color
            SetBkMode(hdc, TRANSPARENT);
            
            RECT rc;
            GetClientRect(hEdit, &rc);
            rc.left += 2; 
            rc.top += 2;
            
            DrawText(hdc, L"Your pin here...", -1, &rc, 
                     DT_LEFT | DT_TOP | DT_NOPREFIX);
        }
       }

        COLORREF storedColor = (COLORREF)GetWindowLongPtr(hwnd, NOTE_COLOR);
        COLORREF newBackgroundColor = storedColor ? storedColor : noteColors[0];
        HDC hdc = (HDC)wParam;
        HBRUSH hBrush = CreateSolidBrush(newBackgroundColor);
        SetBkColor(hdc, newBackgroundColor);
        return (LRESULT)hBrush;
    }
  

    case WM_COMMAND:
    {
        int ctrlId = LOWORD(wParam);
        int notifCode = HIWORD(wParam);

        if (ctrlId == ID_TEXT && notifCode == EN_CHANGE) {
            KillTimer(hwnd, 1);
            SetTimer(hwnd, 1, 300, NULL);  // debounce 300ms
        }
        else if (ctrlId == ID_NOTE_TITLE && notifCode == EN_CHANGE) {
            KillTimer(hwnd, 1);
            SetTimer(hwnd, 1, 300, NULL);  // debounce 300ms
        }
        else{

            switch (notifCode) {
            case BN_CLICKED:
                if (ctrlId == ID_PIN_BUTTON) {
                    {
                        topMostState = (int)GetWindowLongPtr(hwnd, NOTE_TOPMOST_STATE);
                        if (!topMostState) {
                            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                                SWP_NOMOVE | SWP_NOSIZE);

                            SetWindowLongPtr(hwnd, NOTE_TOPMOST_STATE, (LONG_PTR)1);
                        }
                        else {
                            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                            SetWindowLongPtr(hwnd, NOTE_TOPMOST_STATE, 0);

                        }
                    }
                }
                else if (ctrlId == ID_SHOW_ALL_NOTES_BUTTON) {
                    if (IsIconic(hmainWindowHandle)) {
                        ShowWindow(hmainWindowHandle, SW_RESTORE);
                    }
                    ShowWindow(hmainWindowHandle, SW_SHOW);
                    BringWindowToTop(hmainWindowHandle);
                    SetForegroundWindow(hmainWindowHandle);


                }

                else if (ctrlId == ID_NEW_NOTE_BUTTON) {
                    WORD ctrlId = ID_NEW_NOTE;
                    WORD notifCode = BN_CLICKED; 

                    PostMessage(hmainWindowHandle, WM_COMMAND, MAKELPARAM(ctrlId, notifCode), 0);
                }

                else if (ctrlId == ID_SAVE_NOTE_BUTTON) {
                    SendMessage(hmainWindowHandle, WM_APP_SAVE, (WPARAM)hwnd, (LPARAM)hwnd);
                }
                else if (ctrlId == ID_DELETE_NOTE_BUTTON) {
                    SendMessage(hmainWindowHandle, WM_APP_NOTE_DELETED, (WPARAM)hwnd, 0);
                    DestroyWindow(hwnd);
                    
                }

                break;
            }
    }
    }break;

    case WM_TIMER:
        if (wParam == 1) {
            KillTimer(hwnd, 1);
            COLORREF noteColor = (COLORREF)GetWindowLongPtr(hwnd, NOTE_COLOR);
            SendMessage(hmainWindowHandle, WM_APP_SAVE,noteColor, (LPARAM)hwnd);
            SendMessage(hmainWindowHandle, WM_PAINT, (WPARAM)hwnd, (LPARAM)hwnd);
        }
        break;

    case WM_SIZE:
    {
        int windowWidth = LOWORD(lParam);
        int windowHeight = HIWORD(lParam);


        HWND textArea = (HWND)GetWindowLongPtr(hwnd, NOTE_EDIT_HANDLE);
        HWND titleEdit = (HWND)GetWindowLongPtr(hwnd, NOTE_TITLE_HANDLE);
        HWND pinButton = GetDlgItem(hwnd, ID_PIN_BUTTON);
        HWND showAllButton = GetDlgItem(hwnd, ID_SHOW_ALL_NOTES_BUTTON);
        HWND newNoteButton = GetDlgItem(hwnd, ID_NEW_NOTE_BUTTON);
        HWND deleteNoteButton = GetDlgItem(hwnd, ID_DELETE_NOTE_BUTTON);

        int buttonWidth = 100;
        int buttonHeight = 24;
        int titleEditHeight = 35;
        int noteEditHeight = windowHeight - titleEditHeight - buttonHeight - COLOR_HEIGHT;

        MoveWindow(pinButton, 0, 0, buttonWidth, buttonHeight, TRUE);
        MoveWindow(newNoteButton, buttonWidth,0, buttonWidth, buttonHeight, TRUE);
        MoveWindow(showAllButton, buttonWidth * 2, 0, buttonWidth, buttonHeight, TRUE);
        MoveWindow(deleteNoteButton, windowWidth - 24, 0, 24,24, TRUE);

//Fit text area on resize
       MoveWindow(titleEdit, 0, buttonHeight, windowWidth, titleEditHeight, TRUE);
       MoveWindow(textArea, 0, titleEditHeight + buttonHeight, windowWidth, noteEditHeight, TRUE);

       InvalidateRect(hwnd, 0, 1);
       UpdateWindow(hwnd);
       calculateColorRectPosition(hwnd);

    }break;

    case WM_CLOSE:
        SendMessage(hmainWindowHandle, WM_APP_NOTE_CLOSED, (WPARAM)hwnd, (LPARAM)hwnd);
		//If I destroy I will have to spawn a new window, grab the text(from the
		//RECT for now until I can get SQLite running) stick it in the edit and somehow reestablish abort
		//link between the new window and the note
        //DestroyWindow(hwnd);
		ShowWindow(hwnd,SW_HIDE);
        break;
    case WM_DESTROY:

        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}




LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //For tracking note updates
    static int idIsPresent = 0;
    static int noteUpdateId = 0;


    switch (msg)
    {

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        WPARAM scrollCmd;

        if (delta > 0)
            scrollCmd = SB_LINEUP;
        else
            scrollCmd = SB_LINEDOWN;
        SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(scrollCmd, 0), 0);
        return 0;

    }
	case WM_LBUTTONDOWN:
	{

        struct ScrollState* pScrollState = (struct ScrollState*)GetWindowLongPtr(hwnd, sizeof(LONG_PTR) * 2);
		POINT ptClick;
		ptClick.x = GET_X_LPARAM(lParam);
		ptClick.y = GET_Y_LPARAM(lParam);

        POINT ptActual;
        ptActual.x = ptClick.x + pScrollState->scrollPosX;
        ptActual.y = ptClick.y + pScrollState->scrollPosY;

		for(int i = 0; i < noteCount; i++)
		{
			RECT rectXButton;
            int buttonSize = 24;
            rectXButton.left   = notes_true[i].rect.right - buttonSize - 5;
            rectXButton.top    = notes_true[i].rect.top + 5;
            rectXButton.right  = notes_true[i].rect.right;
            rectXButton.bottom = notes_true[i].rect.top + buttonSize;
			
            if (PtInRect(&rectXButton, ptActual) && PtInRect(&notes_true[i].rect, ptActual))
            {
                SendMessage(hwnd, WM_APP_NOTE_DELETED, 0, (LPARAM)notes_true[i].id);
                break;
            }
            
			if(PtInRect(&notes_true[i].rect,ptActual))
			{
                int noteId = notes_true[i].id;
                int noteLength = notes_true[i].textLen;
                SendMessage(hwnd, WM_APP_CALL_UPDATE_WINDOW, (WPARAM)noteId, (LPARAM)noteLength);
			break;
			}
		}
	}break;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    //Wndproc
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    case WM_SIZE:
    {
        RECT rect;
        GetClientRect(hwnd, &rect);
        struct ScrollState* pScrollState = (struct ScrollState*)GetWindowLongPtr(hwnd, sizeof(LONG_PTR) * 2);
        pScrollState->viewPortHeight = rect.bottom;

        //set up vertical scroll. Damn this took a while to figure out
        SCROLLINFO si;
        si.nMax = pScrollState->contentHeight;
        si.nPage = pScrollState->viewPortHeight;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

        int windowWidth = rect.right - rect.left;
        int windowHeight = rect.bottom - rect.top;
        int scrollbarWidth = GetSystemMetrics(SM_CXVSCROLL);

        if (noteCount > 0 && (windowWidth - NOTE_MARGIN) < 800) {
            for (int i = 0; i < noteCount; i++) {
                notes_true[i].rect.right = windowWidth - NOTE_MARGIN;
            }

        }

        int totalContentHeight = (noteCount > 8)
            ? notes_true[noteCount - 1].rect.bottom + NOTE_MARGIN
            : 0;


        int maxScroll = max(0, totalContentHeight - windowHeight);


        int topRightX = windowWidth - scrollbarWidth - 50; 
        int topRightY = 0;                                 

        HWND newNoteButton = (HWND)GetWindowLongPtr(hwnd, NEW_NOTE_BUTTON_HANDLE);
        MoveWindow(newNoteButton, topRightX - 50, topRightY, BUTTON_HEIGHT, BUTTON_HEIGHT, TRUE);
        HWND closeAllButton = GetDlgItem(hwnd, ID_CLOSE_ALL_BUTTON);
        MoveWindow(closeAllButton, topRightX, topRightY, 50, 50, TRUE);
        InvalidateRect(hwnd, NULL, 1);
        UpdateWindow(hwnd);

    }break;
    //WndProc
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    case WM_CREATE:
    {

        RECT rec;
        GetClientRect(hwnd, &rec);
        int windowWidth = rec.right - rec.left;
        int windowHeight = rec.bottom - rec.top;
        int scrollbarWidth = GetSystemMetrics(SM_CXVSCROLL);
        int topRightX = windowWidth - scrollbarWidth - 100;


        HWND optionsButton = CreateWindowEx(
            0, L"BUTTON", L"",
            WS_CHILD | BS_OWNERDRAW | WS_VISIBLE,
            0, 0, 50, 50, hwnd, (HMENU)ID_OPTIONS_BUTTON, GetModuleHandle(NULL),
            NULL
        );

        HWND newNoteButton = CreateWindowEx(
            0, L"BUTTON", L"",
            WS_CHILD | BS_OWNERDRAW | WS_VISIBLE,
            topRightX, 50, 50, 50, hwnd, (HMENU)ID_NEW_NOTE, GetModuleHandle(NULL),
            NULL
        );

        HWND closeAllButton = CreateWindowEx(0, L"BUTTON", L"Close",
            WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE,
            topRightX, 0, 100, 50, hwnd, (HMENU)ID_CLOSE_ALL_BUTTON, GetModuleHandle(0), NULL
        );

        hMainWindowContentFont = CreateFont(
            -17,                // Height 
            0, 0, 0,            // Width, escapement, orientation
            FW_NORMAL,          // Weight
            FALSE, FALSE, FALSE,// Italic, underline, strikeout
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI"          // Font faceres
        );
        SetWindowLongPtr(hwnd, NEW_NOTE_BUTTON_HANDLE, (LONG_PTR)newNoteButton);
        
        //initialising scroll state in main window extra byte
        struct ScrollState* pScrollState = malloc(sizeof(struct ScrollState));
        pScrollState->scrollPosY = 0;
        pScrollState->scrollPosX = 0;
        pScrollState->contentHeight = 0;
        pScrollState->viewPortHeight = 0;
        SetWindowLongPtr(hwnd, MAIN_WINDOW_SCROLL_STATE,(LONG_PTR)pScrollState);

        SendMessage(hwnd, WM_SIZE, 0, MAKELPARAM(windowWidth, windowHeight));
        RecalculateNotePositions(hwnd);
    }break;

    case WM_VSCROLL:
    {

        SCROLLINFO si;
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_ALL;
        GetScrollInfo(hwnd, SB_VERT, &si);

        int yPos = si.nPos;  // current position
      

        switch (LOWORD(wParam))
        {
        case SB_LINEUP:
            si.nPos -= 20;
            break;

        case SB_LINEDOWN:
            si.nPos += 20;
            break;

        case SB_PAGEUP:
            si.nPos -= si.nPage;
            break;

        case SB_PAGEDOWN:
            si.nPos += si.nPage;
            break;

        case SB_THUMBTRACK:
            si.nPos = si.nTrackPos;
            break;

        default:
            break;
        }

       //set new position
        si.fMask = SIF_POS;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        GetScrollInfo(hwnd, SB_VERT, &si);

        //calculate how much was scrolled
        int scrollDelta = yPos - si.nPos;

        //update stored scroll position
        struct ScrollState* pScrollState = (struct ScrollState*)GetWindowLongPtr(hwnd, sizeof(LONG_PTR) * 2);
        pScrollState->scrollPosY = si.nPos;

        //scroll window content
        ScrollWindow(hwnd,0, scrollDelta, NULL, NULL);
        UpdateWindow(hwnd);
        return 0;
    }
    break;


    ////////////////////////////////////////////////////////////////////////////////////
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT lpDraw = (LPDRAWITEMSTRUCT)lParam;

        if (lpDraw->CtlID == ID_NEW_NOTE)
        {
            FillRect(lpDraw->hDC, &lpDraw->rcItem, (HBRUSH)(COLOR_WINDOW + 2));

            if (lpDraw->itemState & ODS_SELECTED)
            {
                FillRect(lpDraw->hDC, &lpDraw->rcItem, CreateSolidBrush(RGB(220, 220, 220)));
            }

            // Compute icon placement
            int iconSize = 24; 
            int btnWidth = lpDraw->rcItem.right - lpDraw->rcItem.left;
            int btnHeight = lpDraw->rcItem.bottom - lpDraw->rcItem.top;

            if (iconSize > btnWidth || iconSize > btnHeight)
                iconSize = min(btnWidth, btnHeight);

            int iconX = lpDraw->rcItem.left + (btnWidth - iconSize) / 2;
            int iconY = lpDraw->rcItem.top + (btnHeight - iconSize) / 2;

            DrawIconEx(lpDraw->hDC, iconX, iconY, hPlusIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);

            return TRUE;
        }

        if (lpDraw->CtlID == ID_OPTIONS_BUTTON)
        {
            FillRect(lpDraw->hDC, &lpDraw->rcItem, (HBRUSH)(COLOR_WINDOW + 1));

            // Highlight when pressed
            if (lpDraw->itemState & ODS_SELECTED)
            {
                FillRect(lpDraw->hDC, &lpDraw->rcItem, CreateSolidBrush(RGB(220, 220, 220)));
            }

            int iconSize = 30;  
            int btnWidth = lpDraw->rcItem.right - lpDraw->rcItem.left;
            int btnHeight = lpDraw->rcItem.bottom - lpDraw->rcItem.top;

            if (iconSize > btnWidth || iconSize > btnHeight)
                iconSize = min(btnWidth, btnHeight);

            int iconX = lpDraw->rcItem.left + (btnWidth - iconSize) / 2;
            int iconY = lpDraw->rcItem.top + (btnHeight - iconSize) / 2;

            DrawIconEx(lpDraw->hDC, iconX, iconY, hOptionIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);

            return TRUE;
        }

    }
    break;

    /////////////////////////////////////////////////////////////////////////////////////////////////////      


    //TODO: Implement scrolling

    //WndProc
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        int yScrollPos = GetScrollPos(hwnd, SB_VERT);
       SetViewportOrgEx(hdc, 0, -yScrollPos, NULL);

       HFONT oldFont = (HFONT)SelectObject(hdc, hMainWindowContentFont);


        SetBkMode(hdc, TRANSPARENT);

        int theY = NOTE_MARGIN +50;

        for (int i = 0; i < noteCount; i++)
        {

            COLORREF color = getNoteColor(notes_true[i].id) ? getNoteColor(notes_true[i].id) : noteColors[0];
            HBRUSH hBrush = CreateSolidBrush(color);
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hBrush);


            RoundRect(hdc, notes_true[i].rect.left, notes_true[i].rect.top, notes_true[i].rect.right, notes_true[i].rect.bottom,20,20);
            

            RECT textRect = notes_true[i].rect;
            InflateRect(&textRect, -5, -5);
            SelectObject(hdc, oldBrush);
            DeleteObject(hBrush);

            // fetch content from DB using each note's unique ID #needs optimization, hitting DB on every repaint is a waste. I need to cache these values later

            char* title = getNoteTitle(notes_true[i].id);
            char* content = getNoteContent(notes_true[i].id);

            if (title) {
                RECT titleRect = textRect;

                titleRect.bottom = titleRect.top + 20;

                SetTextColor(hdc, RGB(0, 0, 0));
                SetBkMode(hdc, TRANSPARENT);

                DrawTextA(hdc, title, -1, &titleRect,
                    DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);

                SelectObject(hdc, hMainWindowContentFont);


                textRect.top += 24; // title height + spacing
            }

            if (content) {
                DrawTextA(
                    hdc,
                    content,
                    -1,
                    &textRect,
                    DT_LEFT | DT_TOP | DT_WORDBREAK
                );

            }
            else {
                DrawTextA(
                    hdc,
                    "",
                    -1,
                    &textRect,
                    DT_LEFT | DT_TOP | DT_WORDBREAK
                );
            }


  
			RECT rectXButton;
            int buttonSize = 24;
            rectXButton.left   = notes_true[i].rect.right - buttonSize;
            rectXButton.top    = notes_true[i].rect.top;
            rectXButton.right  = notes_true[i].rect.right;
            rectXButton.bottom = notes_true[i].rect.top + buttonSize;

            DrawIconEx(hdc, rectXButton.left - 5, rectXButton.top + 5, hDeleteMainWindowIcon,
                buttonSize, buttonSize,
                0, NULL, DI_NORMAL);

           if(content) free(content);
           if (title) free(title);
          
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
    break;
    

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case WM_APP_CALL_UPDATE_WINDOW:
    {

        int noteId = (int)wParam;
        char* utf8NoteTitleValue = getNoteTitle(noteId);
        wchar_t* noteTitleValue = Utf8ToWide(utf8NoteTitleValue);
        char* utf8NoteValue = getNoteContent(noteId);
        wchar_t* noteValue = Utf8ToWide(utf8NoteValue);

        COLORREF noteColor = getNoteColor(noteId);
        HWND noteWindow = CreateWindowEx(
            0,
            myNoteClass,
            windowTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
            NULL, NULL, GetModuleHandle(NULL), NULL);
        SetWindowLongPtr(noteWindow, NOTE_ID, (LONG_PTR)noteId);
        SetWindowLongPtr(noteWindow, NOTE_COLOR, (LONG_PTR)noteColor);
        ShowWindow(noteWindow, SW_SHOW);


        int hEdit = GetDlgItem(noteWindow, ID_TEXT);
        int hNoteTitleEdit = GetDlgItem(noteWindow, ID_NOTE_TITLE);

        if (hEdit && noteValue) {
            SetWindowText(hEdit, noteValue);
            SetWindowText(hNoteTitleEdit, noteTitleValue);

        }
                
        idIsPresent = 1;
        noteUpdateId = noteId;
        free(utf8NoteTitleValue);
        free(utf8NoteValue);
        free(noteValue);
        free(noteTitleValue);
        noteValue = NULL;

    }break;
	
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case WM_APP_NOTE_DELETED:
    {

        int noteId;

        if (lParam) {
            //lParam is note ID from main window
            noteId = (int)lParam;
        }
        else {
            //wParam is window handle from note
            HWND hNoteWindow = (HWND)wParam;
            noteId = (int)GetWindowLongPtr(hNoteWindow, NOTE_ID);
        }
 

        for (int i = 0; i < noteCount; i++) {
            if (notes_true[i].id == noteId) {

                deleteNoteFromDatabase(noteId);
                for (int j = i; j < noteCount - 1; j++) {
                    notes_true[j] = notes_true[j + 1];
                }
                noteCount--;

                if (noteCount > 0) {
                    struct Note* pTempNotes = realloc(notes_true, noteCount * sizeof(struct Note));
                    if (pTempNotes != NULL) notes_true = pTempNotes;
                    
                }
                else {
                    free(notes_true);
                    notes_true = NULL;

                }
                RecalculateNotePositions(hwnd);
                InvalidateRect(hwnd, NULL, TRUE);
                break;
            }
        }
    }
    break;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case WM_COMMAND:
    {
        int ctrlId = LOWORD(wParam);
        int notifCode = HIWORD(wParam);

            switch (notifCode)
            {
            case BN_CLICKED:
            {
                
        if (ctrlId == ID_NEW_NOTE){

            RECT rect;
            GetWindowRect(hwnd, &rect);
            int windowWidth = rect.right - rect.left;
            int windowHeight = rect.bottom - rect.top;
                    HWND note = CreateWindowEx(
                        0,
                        myNoteClass,
                        windowTitle,
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
                        NULL, NULL, GetModuleHandle(NULL), NULL);

                    if (note == NULL) {
                        MessageBox(hwnd, L"Note creation failed", L"Error!", MB_OK | MB_ICONERROR);
                        break;
                    }
                        
                        notes_true = realloc(notes_true, sizeof(struct Note) * (noteCount + 1));
                        if (notes_true == NULL) {
                            MessageBox(hwnd, L"Memory allocation failed", L"!Error", MB_OKCANCEL);
                            break;
                        }
                        else {
                            //shift chunk up by one and free up position 0
                                memmove(notes_true + 1,notes_true, sizeof(struct Note) * noteCount);
                                notes_true[0] = (struct Note){0}; 
                                
                                
                                notes_true[0].rect.left = NOTE_MARGIN;
                                notes_true[0].rect.top = 50 + NOTE_MARGIN;
                                notes_true[0].rect.right = NOTE_MARGIN + NOTE_WIDTH;
                                notes_true[0].rect.bottom = notes_true[0].rect.top + NOTE_HEIGHT;
                                notes_true[0].noteColor = (uint32_t)noteColors[0];
                                
                                noteCount++;		
                                idIsPresent = 0;
                                noteUpdateId = 0;
                                RecalculateNotePositions(hwnd);
                                InvalidateRect(hwnd, NULL, TRUE);
                                UpdateWindow(hwnd);
                        }
        }
        else if (ctrlId == ID_CLOSE_ALL_BUTTON) {
            free(notes_true);
            sqlite3_close(db);
            PostQuitMessage(0);
        }
        
        else if (ctrlId == ID_SAVE_NOTE_BUTTON) {
            SendMessage(hmainWindowHandle,WM_APP_SAVE, (WPARAM)hwnd, (LPARAM)hwnd);
        }break;
            }
        }
    }break;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case WM_CLOSE:
        //DestroyWindow(hwnd);
        ShowWindow(hwnd, SW_HIDE);
        break;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case WM_APP_SAVE:
    {
        HWND noteHandle = (HWND)lParam;
        HWND hEdit = GetDlgItem(noteHandle, ID_TEXT);
        HWND hTitleEdit = GetDlgItem(noteHandle, ID_NOTE_TITLE);

        int noteContentLength = GetWindowTextLength(hEdit);
        int noteTitleLength = GetWindowTextLength(hTitleEdit);
        uint32_t noteColor = (uint32_t)wParam;

        if (noteContentLength <= 0 && noteTitleLength <= 0 && noteColor <= 0) break;

        // --- Allocate wide buffers ---
        wchar_t* noteContentBuffer = malloc((noteContentLength + 1) * sizeof(wchar_t));
        wchar_t* noteTitleBuffer = malloc((noteTitleLength + 1) * sizeof(wchar_t));

        GetWindowText(hEdit, noteContentBuffer, noteContentLength + 1);
        GetWindowText(hTitleEdit, noteTitleBuffer, noteTitleLength + 1);

        //sqlite expects utf8
        // ---- Convert content to UTF-8 ----
        int utf8ContentLen = WideCharToMultiByte(CP_UTF8, 0, noteContentBuffer, -1, NULL, 0, NULL, NULL);
        char* utf8Content = malloc(utf8ContentLen);
        WideCharToMultiByte(CP_UTF8, 0, noteContentBuffer, -1, utf8Content, utf8ContentLen, NULL, NULL);

        // ---- Convert title to UTF-8 ----
        int utf8TitleLen = WideCharToMultiByte(CP_UTF8, 0, noteTitleBuffer, -1, NULL, 0, NULL, NULL);
        char* utf8Title = malloc(utf8TitleLen);
        WideCharToMultiByte(CP_UTF8, 0, noteTitleBuffer, -1, utf8Title, utf8TitleLen, NULL, NULL);

        char sql[512];


        if (idIsPresent) {
            updateDatabaseEntry(noteUpdateId,utf8Content, utf8Title,noteColor);
        }
        else {
            snprintf(sql, sizeof(sql),
                "INSERT INTO notes (title, content,color) VALUES ('%s', '%s',%u);",utf8Title, utf8Content,noteColor);

            char* errmsg = NULL;
            int rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);

            if (rc != SQLITE_OK) {
                MessageBoxA(hwnd, errmsg ? errmsg : "Database insert failed", "SQLite Error", MB_OK | MB_ICONERROR);
                sqlite3_free(errmsg);
            }
            else {

                sqlite3_int64 lastId = sqlite3_last_insert_rowid(db);
                idIsPresent = 1;
                noteUpdateId = lastId;
                SetWindowLongPtr(noteHandle, NOTE_ID, (LONG_PTR)lastId);
                notes_true[0].id = (int)lastId;

                //copy wide tring
                wcsncpy_s(notes_true[0].title, sizeof(notes_true[0].title) / sizeof(wchar_t), noteTitleBuffer, _TRUNCATE);
                wcsncpy_s(notes_true[0].text, sizeof(notes_true[0].text) / sizeof(wchar_t), noteContentBuffer, _TRUNCATE);
                
                notes_true[0].textLen = noteContentLength;
                notes_true[0].rect = (RECT){ 0,0,0,0 };
                notes_true[0].noteColor = noteColor;
            }
        }
        RecalculateNotePositions(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        free(noteContentBuffer);
        free(noteTitleBuffer);
        free(utf8Content);
        free(utf8Title);
    }break;

    case WM_DESTROY:
    { 

        struct ScrollState* pScrollState = (struct ScrollState*)GetWindowLongPtr(hwnd, sizeof(LONG_PTR) * 2);
        if (pScrollState) {
            free(pScrollState);
        }
        if(noteCount == 0)
            PostQuitMessage(0);
    }


    DeleteObject(hMainWindowContentFont);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

//WinMain
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{

    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    hPlusIcon = (HICON)LoadImage(
        hInstance,
        MAKEINTRESOURCE(IDI_PLUS_ICON),
        IMAGE_ICON,
        24, 24,
        LR_CREATEDIBSECTION | LR_DEFAULTCOLOR
    );   

    hOptionIcon = (HICON)LoadImage(
        hInstance,
        MAKEINTRESOURCE(IDI_ICON1),
        IMAGE_ICON,
        256, 256,
        LR_CREATEDIBSECTION | LR_DEFAULTCOLOR
    );

    hDeleteIcon = (HICON)LoadImage(
        hInstance,
        MAKEINTRESOURCE(IDI_DELETE_ICON),
        IMAGE_ICON,
        24,24,
        LR_DEFAULTCOLOR | LR_SHARED
    );

    hDeleteMainWindowIcon = (HICON)LoadImage(
        hInstance,
        MAKEINTRESOURCE(IDI_DELETE_MAIN_WINDOW),
        IMAGE_ICON,
        24, 24,
        LR_DEFAULTCOLOR | LR_SHARED
    );

    hPinIcon = (HICON)LoadImage(
        hInstance,
        MAKEINTRESOURCE(IDI_PIN_ICON),
        IMAGE_ICON,
        24, 24,
        LR_DEFAULTCOLOR | LR_SHARED
    );



   OpenDatabase();
   noteCount = getNoteCount(db);
   notes_true = malloc(sizeof(struct Note) * noteCount) ;

   wchar_t* content;
   RECT rect = { 10, 10, 210, 110 };

   sqlite3_stmt* stmt;
   const char* sql = "SELECT id, title, content,color FROM notes ORDER BY id DESC;";

   if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
       MessageBoxA(NULL, sqlite3_errmsg(db), L"Failed to prepare select", MB_OK | MB_ICONERROR);
   }
   else {
       int i = 0;
       while (sqlite3_step(stmt) == SQLITE_ROW) 
       {
           notes_true[i].id = sqlite3_column_int(stmt, 0);
           
           const uint32_t color = (uint32_t)sqlite3_column_int64(stmt, 3);
           notes_true[i].noteColor = color;
           notes_true[i].rect = rect;

           // ----- Read UTF-8 from SQLite -----
           const char* titleUtf8 = (const char*)sqlite3_column_text(stmt, 1);
           const char* contentUtf8 = (const char*)sqlite3_column_text(stmt, 2);

           // Safe for NULL columns
           if (!titleUtf8)   titleUtf8 = "";
           if (!contentUtf8) contentUtf8 = "";

           // ----- Convert UTF-8 → UTF-16 for title -----
           int titleLenW = MultiByteToWideChar(CP_UTF8, 0, titleUtf8, -1, NULL, 0);
           if (titleLenW > 0) {
               MultiByteToWideChar(CP_UTF8, 0, titleUtf8, -1,
                   notes_true[i].title,
                   sizeof(notes_true[i].title) / sizeof(wchar_t));
           }
           else {
               notes_true[i].title[0] = L'\0';
           }

           // ----- Convert UTF-8 → UTF-16 for content -----
           int contentLenW = MultiByteToWideChar(CP_UTF8, 0, contentUtf8, -1, NULL, 0);
           if (contentLenW > 0) {
               MultiByteToWideChar(CP_UTF8, 0, contentUtf8, -1,
                   notes_true[i].text,
                   sizeof(notes_true[i].text) / sizeof(wchar_t));
               notes_true[i].textLen = contentLenW - 1; // no null terminator
           }
           else {
               notes_true[i].text[0] = L'\0';
               notes_true[i].textLen = 0;
           }

           i++;
       }
       sqlite3_finalize(stmt);
   }

   ////////////////////////////////////

    WNDCLASSEX wc;
    MSG Msg;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(LONG_PTR) *3;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PINPALS));
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = windowClass;
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PINPALS));

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    WNDCLASSEX noteClass = {0};
    noteClass.cbSize = sizeof(WNDCLASSEX);
    noteClass.style = 0;
    noteClass.lpfnWndProc = NoteWndProc;
    noteClass.cbClsExtra = 0;
    noteClass.cbWndExtra = 0;
    noteClass.hbrBackground = CreateSolidBrush(noteColors[0]);
    noteClass.hInstance = hInstance;
    noteClass.lpszClassName = myNoteClass;
    noteClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PINPALS));
    noteClass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PINPALS));
    noteClass.cbWndExtra = sizeof(LONG_PTR) * 5;

    if (!RegisterClassEx(&noteClass))
    {
        MessageBox(NULL, "Note Registration Failed", "Error!", MB_ICONEXCLAMATION);
    }

    hmainWindowHandle = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        windowClass,
        windowTitle,
        WS_OVERLAPPEDWINDOW | WS_VSCROLL ,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
        NULL, NULL, hInstance, NULL);

    if (hmainWindowHandle == NULL)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hmainWindowHandle, nCmdShow);
    UpdateWindow(hmainWindowHandle);

    while (GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
   
    return Msg.wParam;
}




//Function definitions
////////////////////////////////
int getNoteCount(sqlite3* db) {
    sqlite3_stmt* stmt;
    int count = 0;
    const char* sql = "SELECT COUNT(*) FROM notes;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
}

int OpenDatabase(void) {
    int rc = sqlite3_open("notes.db", &db);
    if (rc != SQLITE_OK) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "sqlite3_open failed", MB_OK | MB_ICONERROR);
        sqlite3_close(db);
        db = NULL;
        return rc;
    }

    const char* create_sql =
        "CREATE TABLE IF NOT EXISTS notes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT,"
        "content TEXT,"
        "color INTEGER,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);";

    char* errmsg = NULL;
    rc = sqlite3_exec(db, create_sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        MessageBoxA(NULL, errmsg ? errmsg : "Unknown error", "sqlite3_exec failed", MB_OK | MB_ICONERROR);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        db = NULL;
        return rc;
    }

  
    return SQLITE_OK;
}


int addToDatabase(struct Note* note)
{
    int rc;
    sqlite3_stmt* stmt;

    const char* insert_sql =
        "INSERT INTO notes (title, content, color) VALUES (?, ?, ?);";

    rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "Prepare failed", MB_OK | MB_ICONERROR);
        return rc;
    }

    sqlite3_bind_text(stmt, 1, note->title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, note->text, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)note->noteColor);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        MessageBoxA(NULL, "Failed to insert note", "Error", MB_OK | MB_ICONERROR);
        return rc;
    }

    // Get the last inserted ID
    sqlite3_int64 last_id = sqlite3_last_insert_rowid(db);

    const char* select_sql =
        "SELECT title, content, color FROM notes WHERE id = ?;";

    rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, last_id);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* title = sqlite3_column_text(stmt, 0);
            const unsigned char* content = sqlite3_column_text(stmt, 1);

            char message[512];
            snprintf(message, sizeof(message), "Title: %s\nContent: %s", title, content);
        }
        sqlite3_finalize(stmt);
    }
    else {
        MessageBoxA(NULL, "Failed to prepare SELECT", "Error", MB_OK | MB_ICONERROR);
    }
    note->id = (int)last_id;

    return SQLITE_OK;
}

void deleteNoteFromDatabase(int noteId) {
    if (!db) return;

    sqlite3_stmt* stmt = NULL;
    const char* sql = "DELETE FROM notes WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "Prepare failed", MB_OK | MB_ICONERROR);
        return;
    }

    sqlite3_bind_int(stmt, 1, noteId);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "Delete failed", MB_OK | MB_ICONERROR);
    }

    sqlite3_finalize(stmt);
}

char* getNoteContent(int noteId) {
    if (!db) {
        MessageBox(NULL, "Database not open", "Error", MB_OK | MB_ICONERROR);
        return NULL;
    }

    const char* sql = "SELECT content FROM notes WHERE id = ?;";
    sqlite3_stmt* stmt = NULL;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "sqlite3_prepare_v2 failed", MB_OK | MB_ICONERROR);
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, noteId);

    char* result = NULL;

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        if (text) {
            result = _strdup((const char*)text);
        }
    }
    else if (rc != SQLITE_DONE) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "sqlite3_step failed", MB_OK | MB_ICONERROR);
    }

    sqlite3_finalize(stmt);
    return result; // caller must free
}

char* getNoteTitle(int noteId) {
    if (!db) {
        MessageBox(NULL, "Database not open", "Error", MB_OK | MB_ICONERROR);
        return NULL;
    }

    const char* sql = "SELECT title FROM notes WHERE id = ?;";
    sqlite3_stmt* stmt = NULL;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "sqlite3_prepare_v2 failed", MB_OK | MB_ICONERROR);
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, noteId);

    char* result = NULL;

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        if (text) {
            result = _strdup((const char*)text);
        }
    }
    else if (rc != SQLITE_DONE) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "sqlite3_step failed", MB_OK | MB_ICONERROR);
    }

    sqlite3_finalize(stmt);
    return result; // caller must free
}

uint32_t getNoteColor(int noteId) {
    if (!db) {
        MessageBox(NULL, "Database not open", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    const char* sql = "SELECT color FROM notes WHERE id = ?;";
    sqlite3_stmt* stmt = NULL;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "sqlite3_prepare_v2 failed", MB_OK | MB_ICONERROR);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, noteId);

    uint32_t result = 0;

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        result = (uint32_t)sqlite3_column_int64(stmt, 0);

    }
    else if (rc != SQLITE_DONE) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "sqlite3_step failed", MB_OK | MB_ICONERROR);
    }

    sqlite3_finalize(stmt);
    return result;
}

void RecalculateNotePositions(HWND hwnd) {
    int yOffset = NOTE_MARGIN + 50;
    RECT rect;
    GetWindowRect(hwnd,&rect);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;


    for (int i = 0; i < noteCount; i++)
    {
        notes_true[i].rect.left = NOTE_MARGIN;
        notes_true[i].rect.top = yOffset;
        notes_true[i].rect.right = (windowWidth<800) ? (windowWidth - NOTE_MARGIN) : 800;
        notes_true[i].rect.bottom = yOffset + NOTE_HEIGHT;
        yOffset += NOTE_HEIGHT + NOTE_MARGIN;
    }

    struct ScrollState* pScrollState = (struct ScrollState*)GetWindowLongPtr(hwnd, MAIN_WINDOW_SCROLL_STATE);
    if (pScrollState) {
        pScrollState->contentHeight = noteCount * (NOTE_HEIGHT + NOTE_MARGIN) + 50;
    }

    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_PAGE;
    si.nMin = 0;
    si.nMax = pScrollState->contentHeight;
    si.nPage = pScrollState->viewPortHeight;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

    InvalidateRect(hwnd, NULL, TRUE);
}





void updateDatabaseEntry(int noteId, const char* noteContent,const char* noteTitle,uint32_t noteColor) {
    sqlite3_stmt* stmt;
    int rc;
    
    const char* sql = "UPDATE notes SET title = ?, content = ?, color = ? WHERE id = ?;";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "Failed to prepare update", MB_OK | MB_ICONERROR);
        return;
    }

    sqlite3_bind_text(stmt, 1, noteTitle, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, noteContent, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, (sqlite_int64)noteColor);
    sqlite3_bind_int(stmt, 4, noteId);
    


    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        MessageBoxA(NULL, sqlite3_errmsg(db), "Update failed", MB_OK | MB_ICONERROR);
    }
    sqlite3_finalize(stmt);
}

wchar_t* Utf8ToWide(const char* utf8)
{
    if (!utf8) return NULL;

    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (sizeNeeded <= 0) return NULL;

    wchar_t* wide = malloc(sizeNeeded * sizeof(wchar_t));
    if (!wide) return NULL;

    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, sizeNeeded);
    return wide;
}

void calculateColorRectPosition(HWND hwnd) {
    RECT windowRect;
    GetClientRect(hwnd, &windowRect);

    int windowHeight = windowRect.bottom - windowRect.top;
    int windowWidth = windowRect.right - windowRect.left;

    int totalColorPalleteWidth = NUMBER_OF_NOTE_COLORS * COLOR_WIDTH;
    int halfwayWindowXValue = (windowWidth - totalColorPalleteWidth) / 2;

    for (int i = 0; i < NUMBER_OF_NOTE_COLORS; i++) {

        g_noteColorRects[i] = (RECT){ halfwayWindowXValue,windowHeight - COLOR_HEIGHT,halfwayWindowXValue + 50,windowHeight };
        halfwayWindowXValue += 50;

    }

}