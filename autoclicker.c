/*
 * Auto Clicker by 22eustachy
 * Compile: gcc autoclicker.c -o autoclicker.exe -lcomctl32 -luser32 -mwindows
 * Requires: MinGW-w64 on Windows
 *
 * FIXES (keyboard):
 *  1. wScan is now filled with MapVirtualKey(vkCode, MAPVK_VK_TO_VSC)
 *     for both key-down and key-up strokes.
 *  2. KEYEVENTF_EXTENDEDKEY is set for all keys that need it
 *     (F-keys, arrows, nav cluster, numpad keys).
 *  3. Double-press delay for keyboard reduced to 10 ms (was 50 ms).
 *  4. StartClicking() guards against a separator item being active.
 */

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma comment(lib, "comctl32.lib")

/* ── Control IDs ── */
#define IDC_HOURS       101
#define IDC_MINS        102
#define IDC_SECS        103
#define IDC_MS          104
#define IDC_RAND_CHECK  105
#define IDC_RAND_MS     106
#define IDC_BTN_COMBO   107
#define IDC_TYPE_COMBO  108
#define IDC_RPT_RADIO1  109
#define IDC_RPT_RADIO2  110
#define IDC_RPT_COUNT   111
#define IDC_CUR_RADIO   112
#define IDC_PICK_RADIO  113
#define IDC_X_EDIT      114
#define IDC_Y_EDIT      115
#define IDC_START       116
#define IDC_STOP        117
#define IDC_PICK_BTN    118

#define WM_STOP_CLICK   (WM_USER + 10)

/* ── Button table ── */
typedef struct {
    const char *name;
    BOOL        isMouse;
    DWORD       mouseDown;
    DWORD       mouseUp;
    DWORD       mouseData; /* XBUTTON1 / XBUTTON2 / 0 */
    WORD        vkCode;
} ButtonEntry;

static ButtonEntry g_buttons[] = {
    /* ── Mouse ── */
    { "Left Button",   TRUE,  MOUSEEVENTF_LEFTDOWN,   MOUSEEVENTF_LEFTUP,   0,        0 },
    { "Right Button",  TRUE,  MOUSEEVENTF_RIGHTDOWN,  MOUSEEVENTF_RIGHTUP,  0,        0 },
    { "Middle Button", TRUE,  MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP, 0,        0 },
    { "X1 Button",     TRUE,  MOUSEEVENTF_XDOWN,      MOUSEEVENTF_XUP,      XBUTTON1, 0 },
    { "X2 Button",     TRUE,  MOUSEEVENTF_XDOWN,      MOUSEEVENTF_XUP,      XBUTTON2, 0 },
    /* ── Keyboard ── */
    { "Space",      FALSE, 0,0,0, VK_SPACE   },
    { "Enter",      FALSE, 0,0,0, VK_RETURN  },
    { "Tab",        FALSE, 0,0,0, VK_TAB     },
    { "Backspace",  FALSE, 0,0,0, VK_BACK    },
    { "Delete",     FALSE, 0,0,0, VK_DELETE  },
    { "Escape",     FALSE, 0,0,0, VK_ESCAPE  },
    { "Shift",      FALSE, 0,0,0, VK_SHIFT   },
    { "Ctrl",       FALSE, 0,0,0, VK_CONTROL },
    { "Alt",        FALSE, 0,0,0, VK_MENU    },
    { "A", FALSE,0,0,0,'A' }, { "B", FALSE,0,0,0,'B' }, { "C", FALSE,0,0,0,'C' },
    { "D", FALSE,0,0,0,'D' }, { "E", FALSE,0,0,0,'E' }, { "F", FALSE,0,0,0,'F' },
    { "G", FALSE,0,0,0,'G' }, { "H", FALSE,0,0,0,'H' }, { "I", FALSE,0,0,0,'I' },
    { "J", FALSE,0,0,0,'J' }, { "K", FALSE,0,0,0,'K' }, { "L", FALSE,0,0,0,'L' },
    { "M", FALSE,0,0,0,'M' }, { "N", FALSE,0,0,0,'N' }, { "O", FALSE,0,0,0,'O' },
    { "P", FALSE,0,0,0,'P' }, { "Q", FALSE,0,0,0,'Q' }, { "R", FALSE,0,0,0,'R' },
    { "S", FALSE,0,0,0,'S' }, { "T", FALSE,0,0,0,'T' }, { "U", FALSE,0,0,0,'U' },
    { "V", FALSE,0,0,0,'V' }, { "W", FALSE,0,0,0,'W' }, { "X", FALSE,0,0,0,'X' },
    { "Y", FALSE,0,0,0,'Y' }, { "Z", FALSE,0,0,0,'Z' },
    { "0", FALSE,0,0,0,'0' }, { "1", FALSE,0,0,0,'1' }, { "2", FALSE,0,0,0,'2' },
    { "3", FALSE,0,0,0,'3' }, { "4", FALSE,0,0,0,'4' }, { "5", FALSE,0,0,0,'5' },
    { "6", FALSE,0,0,0,'6' }, { "7", FALSE,0,0,0,'7' }, { "8", FALSE,0,0,0,'8' },
    { "9", FALSE,0,0,0,'9' },
    { "F1",  FALSE,0,0,0,VK_F1  }, { "F2",  FALSE,0,0,0,VK_F2  },
    { "F3",  FALSE,0,0,0,VK_F3  }, { "F4",  FALSE,0,0,0,VK_F4  },
    { "F5",  FALSE,0,0,0,VK_F5  }, { "F6",  FALSE,0,0,0,VK_F6  },
    { "F7",  FALSE,0,0,0,VK_F7  }, { "F8",  FALSE,0,0,0,VK_F8  },
    { "F9",  FALSE,0,0,0,VK_F9  }, { "F10", FALSE,0,0,0,VK_F10 },
    { "F11", FALSE,0,0,0,VK_F11 }, { "F12", FALSE,0,0,0,VK_F12 },
    { "Up",        FALSE,0,0,0,VK_UP     }, { "Down",      FALSE,0,0,0,VK_DOWN   },
    { "Left",      FALSE,0,0,0,VK_LEFT   }, { "Right",     FALSE,0,0,0,VK_RIGHT  },
    { "Home",      FALSE,0,0,0,VK_HOME   }, { "End",       FALSE,0,0,0,VK_END    },
    { "Page Up",   FALSE,0,0,0,VK_PRIOR  }, { "Page Down", FALSE,0,0,0,VK_NEXT   },
    { "Insert",    FALSE,0,0,0,VK_INSERT },
    { "Num 0", FALSE,0,0,0,VK_NUMPAD0 }, { "Num 1", FALSE,0,0,0,VK_NUMPAD1 },
    { "Num 2", FALSE,0,0,0,VK_NUMPAD2 }, { "Num 3", FALSE,0,0,0,VK_NUMPAD3 },
    { "Num 4", FALSE,0,0,0,VK_NUMPAD4 }, { "Num 5", FALSE,0,0,0,VK_NUMPAD5 },
    { "Num 6", FALSE,0,0,0,VK_NUMPAD6 }, { "Num 7", FALSE,0,0,0,VK_NUMPAD7 },
    { "Num 8", FALSE,0,0,0,VK_NUMPAD8 }, { "Num 9", FALSE,0,0,0,VK_NUMPAD9 },
};
#define BUTTON_COUNT ((int)(sizeof(g_buttons)/sizeof(g_buttons[0])))
#define MOUSE_COUNT  5   /* first 5 entries are mouse */

/* ── Globals ── */
static HWND   g_hwnd    = NULL;
static HANDLE g_thread  = NULL;
static BOOL   g_running = FALSE;
static HHOOK  g_mouseHook = NULL;
static HHOOK  g_kbHook    = NULL;
static BOOL   g_pickMode  = FALSE;

/* ── Click config (passed to thread) ── */
typedef struct {
    DWORD intervalMs;
    int   randOffsetMs;
    int   btnIndex;      /* index into g_buttons[] */
    BOOL  isDouble;
    BOOL  repeatForever;
    int   repeatCount;
    BOOL  useCurLoc;
    int   x, y;
} ClickConfig;

/* ─────────────────────────────────────────────
   FIX: Returns TRUE for keys that require KEYEVENTF_EXTENDEDKEY.
   Without this flag the wrong scancode is sent for the nav cluster,
   arrow keys, and F-keys, and many targets ignore the event entirely.
   ───────────────────────────────────────────── */
static BOOL NeedsExtendedKey(WORD vk)
{
    switch (vk) {
        case VK_RMENU: case VK_RCONTROL:
        case VK_INSERT: case VK_DELETE:
        case VK_HOME:   case VK_END:
        case VK_PRIOR:  case VK_NEXT:
        case VK_UP:     case VK_DOWN:
        case VK_LEFT:   case VK_RIGHT:
        case VK_NUMLOCK: case VK_CANCEL:
        case VK_SNAPSHOT:
        case VK_DIVIDE:   /* numpad / */
        case VK_F1:  case VK_F2:  case VK_F3:  case VK_F4:
        case VK_F5:  case VK_F6:  case VK_F7:  case VK_F8:
        case VK_F9:  case VK_F10: case VK_F11: case VK_F12:
            return TRUE;
        default:
            return FALSE;
    }
}

/* ─────────────────────────────────────────────
   Send a single click (mouse or key press+release)
   ───────────────────────────────────────────── */
static void SendOneClick(const ClickConfig *cfg)
{
    ButtonEntry *b = &g_buttons[cfg->btnIndex];
    int repeats = cfg->isDouble ? 2 : 1;

    for (int r = 0; r < repeats; r++) {
        if (b->isMouse) {
            INPUT inp[3];
            int   n = 0;
            memset(inp, 0, sizeof(inp));

            /* Optional absolute move */
            if (!cfg->useCurLoc) {
                int sw = GetSystemMetrics(SM_CXSCREEN);
                int sh = GetSystemMetrics(SM_CYSCREEN);
                inp[n].type       = INPUT_MOUSE;
                inp[n].mi.dx      = (LONG)(cfg->x * 65535.0 / sw);
                inp[n].mi.dy      = (LONG)(cfg->y * 65535.0 / sh);
                inp[n].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
                n++;
            }

            inp[n].type            = INPUT_MOUSE;
            inp[n].mi.dwFlags      = b->mouseDown;
            inp[n].mi.mouseData    = b->mouseData;
            n++;
            inp[n].type            = INPUT_MOUSE;
            inp[n].mi.dwFlags      = b->mouseUp;
            inp[n].mi.mouseData    = b->mouseData;
            n++;
            SendInput(n, inp, sizeof(INPUT));
        } else {
            /* ── FIX 1: populate wScan so targets that ignore vkCode still work ── */
            /* ── FIX 2: set KEYEVENTF_EXTENDEDKEY for keys that require it       ── */
            WORD  vk      = b->vkCode;
            WORD  scan    = (WORD)MapVirtualKey(vk, MAPVK_VK_TO_VSC);
            DWORD exFlags = NeedsExtendedKey(vk) ? KEYEVENTF_EXTENDEDKEY : 0;

            INPUT inp[2];
            memset(inp, 0, sizeof(inp));

            /* Key down */
            inp[0].type           = INPUT_KEYBOARD;
            inp[0].ki.wVk         = vk;
            inp[0].ki.wScan       = scan;
            inp[0].ki.dwFlags     = exFlags;

            /* Key up */
            inp[1].type           = INPUT_KEYBOARD;
            inp[1].ki.wVk         = vk;
            inp[1].ki.wScan       = scan;
            inp[1].ki.dwFlags     = KEYEVENTF_KEYUP | exFlags;

            SendInput(2, inp, sizeof(INPUT));
        }

        /* FIX 3: inter-press delay — 10 ms for keys (50 ms was only for mouse) */
        if (cfg->isDouble && r == 0)
            Sleep(b->isMouse ? 50 : 10);
    }
}

/* ─────────────────────────────────────────────
   Worker thread
   ───────────────────────────────────────────── */
static DWORD WINAPI ClickThread(LPVOID param)
{
    ClickConfig *cfg   = (ClickConfig *)param;
    int          count = 0;

    while (g_running) {
        if (!cfg->repeatForever && count >= cfg->repeatCount) break;

        DWORD delay = cfg->intervalMs;
        if (cfg->randOffsetMs > 0) {
            int rnd = (rand() % (cfg->randOffsetMs * 2 + 1)) - cfg->randOffsetMs;
            delay = (DWORD)max(1, (int)delay + rnd);
        }
        Sleep(delay);

        if (!g_running) break;
        SendOneClick(cfg);
        count++;
    }

    g_running = FALSE;
    free(cfg);
    PostMessage(g_hwnd, WM_STOP_CLICK, 0, 0);
    return 0;
}

/* ─────────────────────────────────────────────
   Low-level mouse hook  (for "Pick location")
   ───────────────────────────────────────────── */
static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && g_pickMode && wParam == WM_LBUTTONDOWN) {
        MSLLHOOKSTRUCT *ms = (MSLLHOOKSTRUCT *)lParam;
        char buf[16];
        sprintf(buf, "%d", (int)ms->pt.x);
        SetWindowTextA(GetDlgItem(g_hwnd, IDC_X_EDIT), buf);
        sprintf(buf, "%d", (int)ms->pt.y);
        SetWindowTextA(GetDlgItem(g_hwnd, IDC_Y_EDIT), buf);
        g_pickMode = FALSE;
        UnhookWindowsHookEx(g_mouseHook);
        g_mouseHook = NULL;
        ShowWindow(g_hwnd, SW_RESTORE);
        SetForegroundWindow(g_hwnd);
        return 1; /* consume click */
    }
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

/* ─────────────────────────────────────────────
   Low-level keyboard hook  (F6 start / stop)
   ───────────────────────────────────────────── */
static LRESULT CALLBACK KeyHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT *)lParam;
        if (kb->vkCode == VK_F6) {
            PostMessage(g_hwnd, WM_COMMAND,
                        g_running ? IDC_STOP : IDC_START, 0);
        }
    }
    return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
}

/* ─────────────────────────────────────────────
   Helpers: read an int from edit box
   ───────────────────────────────────────────── */
static int GetEditInt(HWND hwnd, int id)
{
    char buf[32];
    GetWindowTextA(GetDlgItem(hwnd, id), buf, sizeof(buf));
    return atoi(buf);
}

/* ─────────────────────────────────────────────
   Start / Stop
   ───────────────────────────────────────────── */
static void StartClicking(HWND hwnd)
{
    if (g_running) return;

    ClickConfig *cfg = (ClickConfig *)malloc(sizeof(ClickConfig));
    if (!cfg) return;

    cfg->intervalMs = (DWORD)(
        GetEditInt(hwnd, IDC_HOURS) * 3600000 +
        GetEditInt(hwnd, IDC_MINS)  *   60000 +
        GetEditInt(hwnd, IDC_SECS)  *    1000 +
        GetEditInt(hwnd, IDC_MS));
    if (cfg->intervalMs == 0) cfg->intervalMs = 10;

    cfg->randOffsetMs = IsDlgButtonChecked(hwnd, IDC_RAND_CHECK)
                      ? GetEditInt(hwnd, IDC_RAND_MS) : 0;

    /* Combo stores button index in item-data; separator items have -1 */
    HWND hCombo = GetDlgItem(hwnd, IDC_BTN_COMBO);
    int  sel    = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    int  bidx   = (int)SendMessage(hCombo, CB_GETITEMDATA, sel, 0);

    /* FIX 4: guard against a separator row being active (e.g. via keyboard nav) */
    if (bidx < 0) {
        free(cfg);
        MessageBoxA(hwnd,
            "Please select a valid button (not a separator).",
            "Auto Clicker", MB_ICONWARNING | MB_OK);
        return;
    }
    cfg->btnIndex = bidx;

    cfg->isDouble      = (SendMessage(GetDlgItem(hwnd, IDC_TYPE_COMBO),
                                      CB_GETCURSEL, 0, 0) == 1);
    cfg->repeatForever = IsDlgButtonChecked(hwnd, IDC_RPT_RADIO2);
    cfg->repeatCount   = GetEditInt(hwnd, IDC_RPT_COUNT);
    if (cfg->repeatCount <= 0) cfg->repeatCount = 1;

    cfg->useCurLoc = IsDlgButtonChecked(hwnd, IDC_CUR_RADIO);
    cfg->x         = GetEditInt(hwnd, IDC_X_EDIT);
    cfg->y         = GetEditInt(hwnd, IDC_Y_EDIT);

    g_running = TRUE;
    EnableWindow(GetDlgItem(hwnd, IDC_START), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_STOP),  TRUE);

    g_thread = CreateThread(NULL, 0, ClickThread, cfg, 0, NULL);
}

static void StopClicking(HWND hwnd)
{
    g_running = FALSE;
    EnableWindow(GetDlgItem(hwnd, IDC_START), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_STOP),  FALSE);
}

/* ─────────────────────────────────────────────
   Window Procedure
   ───────────────────────────────────────────── */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
                                 WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    /* ── CREATE ── */
    case WM_CREATE:
    {
        g_hwnd = hwnd;
        srand((unsigned)time(NULL));

        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

#define MK(cls,txt,style,x,y,w,h,id) \
    { HWND _h = CreateWindowA(cls,txt,WS_CHILD|WS_VISIBLE|(style),x,y,w,h,hwnd,(HMENU)(UINT_PTR)(id),NULL,NULL); \
      SendMessage(_h, WM_SETFONT,(WPARAM)hFont,TRUE); }

        /* ── Group 1: Click interval ── */
        MK("BUTTON","Click interval", BS_GROUPBOX,          8,  4, 430, 82, 0)
        MK("EDIT",  "0",  WS_BORDER|ES_NUMBER|ES_CENTER,   18, 24,  48, 22, IDC_HOURS)
        MK("STATIC","hours",          0,                   70, 27,  38, 16, 0)
        MK("EDIT",  "0",  WS_BORDER|ES_NUMBER|ES_CENTER,  112, 24,  48, 22, IDC_MINS)
        MK("STATIC","mins",           0,                  164, 27,  30, 16, 0)
        MK("EDIT",  "0",  WS_BORDER|ES_NUMBER|ES_CENTER,  198, 24,  48, 22, IDC_SECS)
        MK("STATIC","secs",           0,                  250, 27,  30, 16, 0)
        MK("EDIT",  "100",WS_BORDER|ES_NUMBER|ES_CENTER,  284, 24,  64, 22, IDC_MS)
        MK("STATIC","milliseconds",   0,                  352, 27,  80, 16, 0)

        MK("BUTTON","Random offset +-", BS_AUTOCHECKBOX,   18, 52, 128, 20, IDC_RAND_CHECK)
        MK("EDIT",  "40", WS_BORDER|ES_NUMBER|ES_CENTER,  152, 51,  60, 22, IDC_RAND_MS)
        MK("STATIC","milliseconds",   0,                  218, 54,  80, 16, 0)

        /* ── Group 2: Click options ── */
        MK("BUTTON","Click options",  BS_GROUPBOX,          8, 90, 218,100, 0)
        MK("STATIC","Button:",        0,                   18,112,  46, 18, 0)

        {   /* Button combo with separators */
            HWND hC = CreateWindowA("COMBOBOX", NULL,
                WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,
                68, 108, 150, 220, hwnd, (HMENU)IDC_BTN_COMBO, NULL, NULL);
            SendMessage(hC, WM_SETFONT, (WPARAM)hFont, TRUE);

#define ADD_ITEM(str, bidx) { \
    int _i = (int)SendMessageA(hC, CB_ADDSTRING, 0, (LPARAM)(str)); \
    SendMessageA(hC, CB_SETITEMDATA, _i, (LPARAM)(bidx)); }

            ADD_ITEM("--- Mouse ---", -1)
            for (int i = 0; i < MOUSE_COUNT; i++)
                ADD_ITEM(g_buttons[i].name, i)

            ADD_ITEM("--- Keyboard ---", -1)
            for (int i = MOUSE_COUNT; i < BUTTON_COUNT; i++)
                ADD_ITEM(g_buttons[i].name, i)

#undef ADD_ITEM
            SendMessage(hC, CB_SETCURSEL, 1, 0); /* Left Button */
        }

        MK("STATIC","Click type:",    0,                   18,142,  62, 18, 0)
        {
            HWND hC = CreateWindowA("COMBOBOX", NULL,
                WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST,
                84, 138, 134, 80, hwnd, (HMENU)IDC_TYPE_COMBO, NULL, NULL);
            SendMessage(hC, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessageA(hC, CB_ADDSTRING, 0, (LPARAM)"Single");
            SendMessageA(hC, CB_ADDSTRING, 0, (LPARAM)"Double");
            SendMessage(hC, CB_SETCURSEL, 0, 0);
        }

        /* ── Group 3: Click repeat ── */
        MK("BUTTON","Click repeat",   BS_GROUPBOX,        232, 90, 206,100, 0)
        MK("BUTTON","Repeat",         BS_AUTORADIOBUTTON|WS_GROUP,
                                                          242,112,  58, 20, IDC_RPT_RADIO1)
        MK("EDIT",  "1",  WS_BORDER|ES_NUMBER|ES_CENTER, 306,110,  52, 22, IDC_RPT_COUNT)
        MK("STATIC","times",          0,                  362,113,  36, 18, 0)
        MK("BUTTON","Repeat until stopped",
                          BS_AUTORADIOBUTTON,             242,140, 150, 20, IDC_RPT_RADIO2)
        CheckRadioButton(hwnd, IDC_RPT_RADIO1, IDC_RPT_RADIO2, IDC_RPT_RADIO2);

        /* ── Group 4: Cursor position ── */
        MK("BUTTON","Cursor position",BS_GROUPBOX,          8,195, 430, 62, 0)
        MK("BUTTON","Current location",
                          BS_AUTORADIOBUTTON|WS_GROUP,    18,216, 120, 20, IDC_CUR_RADIO)
        MK("BUTTON","",   BS_AUTORADIOBUTTON,             146,216,  16, 20, IDC_PICK_RADIO)
        MK("BUTTON","Pick location", BS_PUSHBUTTON,       164,213,  92, 24, IDC_PICK_BTN)
        MK("STATIC","X",              0,                  264,217,  12, 18, 0)
        MK("EDIT",  "0",  WS_BORDER|ES_NUMBER|ES_CENTER, 278,214,  52, 22, IDC_X_EDIT)
        MK("STATIC","Y",              0,                  338,217,  12, 18, 0)
        MK("EDIT",  "0",  WS_BORDER|ES_NUMBER|ES_CENTER, 352,214,  52, 22, IDC_Y_EDIT)
        CheckRadioButton(hwnd, IDC_CUR_RADIO, IDC_PICK_RADIO, IDC_CUR_RADIO);

        /* ── Start / Stop ── */
        MK("BUTTON","Start (F6)",     BS_PUSHBUTTON,        8,265, 210, 36, IDC_START)
        {
            HWND hStop = CreateWindowA("BUTTON","Stop (F6)",
                WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                226,265,212,36, hwnd,(HMENU)IDC_STOP,NULL,NULL);
            SendMessage(hStop, WM_SETFONT,(WPARAM)hFont,TRUE);
            EnableWindow(hStop, FALSE);
        }

#undef MK

        /* Install global keyboard hook for F6 */
        g_kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyHookProc,
                                    GetModuleHandle(NULL), 0);
        break;
    }

    /* ── COMMAND ── */
    case WM_COMMAND:
    {
        int id   = LOWORD(wParam);
        int code = HIWORD(wParam);

        if (id == IDC_START) {
            StartClicking(hwnd);
        }
        else if (id == IDC_STOP) {
            StopClicking(hwnd);
        }
        else if (id == IDC_PICK_BTN) {
            CheckRadioButton(hwnd, IDC_CUR_RADIO, IDC_PICK_RADIO, IDC_PICK_RADIO);
            g_pickMode   = TRUE;
            g_mouseHook  = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc,
                                            GetModuleHandle(NULL), 0);
            ShowWindow(hwnd, SW_MINIMIZE);
        }
        else if (id == IDC_BTN_COMBO && code == CBN_SELCHANGE) {
            /* Skip separator items */
            HWND  hC  = GetDlgItem(hwnd, IDC_BTN_COMBO);
            int   sel = (int)SendMessage(hC, CB_GETCURSEL, 0, 0);
            int   dat = (int)SendMessage(hC, CB_GETITEMDATA, sel, 0);
            if (dat < 0) {
                /* Move to next real item */
                int next = sel + 1;
                int cnt  = (int)SendMessage(hC, CB_GETCOUNT, 0, 0);
                if (next < cnt)
                    SendMessage(hC, CB_SETCURSEL, next, 0);
                else if (sel > 0)
                    SendMessage(hC, CB_SETCURSEL, sel - 1, 0);
            }
        }
        break;
    }

    /* ── Thread finished ── */
    case WM_STOP_CLICK:
        StopClicking(hwnd);
        break;

    /* ── DESTROY ── */
    case WM_DESTROY:
        g_running = FALSE;
        if (g_kbHook)    { UnhookWindowsHookEx(g_kbHook);    g_kbHook    = NULL; }
        if (g_mouseHook) { UnhookWindowsHookEx(g_mouseHook); g_mouseHook = NULL; }
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* ─────────────────────────────────────────────
   WinMain
   ───────────────────────────────────────────── */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev,
                   LPSTR cmdLine, int nShow)
{
    (void)hPrev; (void)cmdLine;

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSA wc      = {0};
    wc.lpfnWndProc    = WndProc;
    wc.hInstance      = hInst;
    wc.lpszClassName  = "ACbyEustachy";
    wc.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowA(
        "ACbyEustachy",
        "Auto Clicker by 22eustachy",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        456, 340,
        NULL, NULL, hInst, NULL);

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}