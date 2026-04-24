#include "pch.hpp"
#include <format>
#include <thread>
#include <shellapi.h>

HWND g_hMainWnd = NULL;
HWND g_hLogEdit = NULL;
constexpr UINT WM_APP_LOG = WM_APP + 1;
constexpr UINT WM_APP_TRAY = WM_APP + 2;
constexpr UINT TRAY_ICON_UID = 0x25A2;
constexpr UINT ID_TRAY_RESTORE = 40001;
constexpr UINT ID_TRAY_EXIT = 40002;

NOTIFYICONDATAA g_tray_icon = {};
bool g_tray_icon_visible = false;

void remove_tray_icon()
{
    if (!g_tray_icon_visible)
        return;

    Shell_NotifyIconA(NIM_DELETE, &g_tray_icon);
    g_tray_icon_visible = false;
}

void show_tray_icon(HWND hWnd)
{
    if (g_tray_icon_visible)
        return;

    g_tray_icon = {};
    g_tray_icon.cbSize = sizeof(g_tray_icon);
    g_tray_icon.hWnd = hWnd;
    g_tray_icon.uID = TRAY_ICON_UID;
    g_tray_icon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_tray_icon.uCallbackMessage = WM_APP_TRAY;
    g_tray_icon.hIcon = static_cast<HICON>(LoadImageA(NULL, IDI_APPLICATION, IMAGE_ICON, 16, 16, LR_SHARED));
    strcpy_s(g_tray_icon.szTip, "cs2_webradar");

    if (Shell_NotifyIconA(NIM_ADD, &g_tray_icon))
        g_tray_icon_visible = true;
}

void minimize_to_tray()
{
    if (!g_hMainWnd)
        return;

    show_tray_icon(g_hMainWnd);
    ShowWindow(g_hMainWnd, SW_HIDE);
}

void restore_from_tray()
{
    if (!g_hMainWnd)
        return;

    ShowWindow(g_hMainWnd, SW_RESTORE);
    SetForegroundWindow(g_hMainWnd);
    remove_tray_icon();
}

void show_tray_menu(HWND hWnd)
{
    HMENU menu = CreatePopupMenu();
    if (!menu)
        return;

    AppendMenuA(menu, MF_STRING, ID_TRAY_RESTORE, "Restore");
    AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(menu, MF_STRING, ID_TRAY_EXIT, "Exit");

    POINT cursor_pos;
    GetCursorPos(&cursor_pos);

    SetForegroundWindow(hWnd);
    const auto clicked_command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, cursor_pos.x, cursor_pos.y, 0, hWnd, NULL);
    DestroyMenu(menu);

    if (clicked_command != 0)
        PostMessage(hWnd, WM_COMMAND, clicked_command, 0);
}

void LogMessage(const std::string& msg, int type = 1) {
    if (!g_hMainWnd) return;

    static const char* levels[] = { "[OTHER] ", "[INFO] ", "[WARNING] ", "[ERROR] " };
    const char* prefix = (type >= 0 && type <= 3) ? levels[type] : levels[0];

    std::string fullMsg = prefix + msg;

    char* pText = new char[fullMsg.size() + 1];
    std::copy(fullMsg.begin(), fullMsg.end(), pText);
    pText[fullMsg.size()] = '\0';

    PostMessage(g_hMainWnd, WM_APP_LOG, 0, (LPARAM)pText);
}

DWORD WINAPI AppLogic(LPVOID) {
    LOG_CLEAR();

    if (!utils::is_updated()) {
        LogMessage("Radar is not updated! Check LOG for more info.", 3);
        return 0;
    }
    LogMessage("Radar is up to date.");
    LOG_INFO("Radar is up to date.");

    config_data_t config = {};
    int cfgResult = cfg::setup(config);
    if (cfgResult != 0) {
        const char* errs[] = { "", "Couldn't open config.json.", "Failed to parse config.json.", "Failed to deserialize config.json." };
        LogMessage(errs[cfgResult], 3);
        return 0;
    }
    LogMessage("Config system initialization completed.");

    if (!exc::setup()) { LogMessage("Exception setup failed!", 3); return 0; }
    LogMessage("Exception handler initialized.");

    int memStatus;
    bool waitingLog = true;
    do {
        memStatus = m_memory->setup();
        if (memStatus == 1) { LogMessage("Anti-cheat detected. Disable it.", 3); return 0; }
        if (memStatus == 3) { LogMessage("Memory init failed.", 3); return 0; }
        if (memStatus == 2) {
            if (waitingLog) {
                LogMessage("Waiting for CS2.exe...");
                waitingLog = false;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } while (memStatus == 2);

    LogMessage("Found CS2.exe, initializing...");
    LogMessage("Memory initialization completed.");

    waitingLog = true;
    while (!i::setup()) {
        if (waitingLog) {
            LogMessage("Waiting for game load...");
            waitingLog = false;
        }
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    LogMessage("Game loaded.");

    if (!schema::setup()) { LogMessage("Schema setup failed!", 3); return 0; }
    LogMessage("Schema initialized.");

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 0;

    auto ipv4 = utils::get_ipv4_address(config);
    if (ipv4.empty()) {
        ipv4 = config.m_local_ip;
        LogMessage(std::format("Failed to get auto-IP. Using config IP: '{}'", ipv4), 2);
    }

    std::string url = std::format("ws://{}:22006/cs2_webradar", ipv4);
    auto ws = easywsclient::WebSocket::from_url(url);

    while (!ws) {
        LogMessage(std::format("Connection failed ({}), retrying...", url), 3);
        ws = easywsclient::WebSocket::from_url(url);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    LogMessage("Connected to websocket.");

    auto start = std::chrono::system_clock::now();
    bool in_match = false;

    while (true) {
        auto now = std::chrono::system_clock::now();
        if ((now - start) >= std::chrono::milliseconds(45)) {
            start = now;
            sdk::update();
            in_match = f::run();
            if (!in_match) f::m_data["m_map"] = "invalid";
            ws->send(f::m_data.dump());
        }
        ws->poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 1;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        g_hLogEdit = CreateWindow("EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            10, 0, 360, 210, hWnd, (HMENU)1, NULL, NULL);
        SendMessage(g_hLogEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        return 0;

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
            minimize_to_tray();
            return 0;
        }
        break;

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MINIMIZE) {
            minimize_to_tray();
            return 0;
        }
        break;

    case WM_APP_TRAY:
        if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
            restore_from_tray();
            return 0;
        }

        if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
            show_tray_menu(hWnd);
            return 0;
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TRAY_RESTORE) {
            restore_from_tray();
            return 0;
        }

        if (LOWORD(wParam) == ID_TRAY_EXIT) {
            remove_tray_icon();
            DestroyWindow(hWnd);
            return 0;
        }
        break;

    case WM_APP_LOG: {
        char* pText = (char*)lParam;
        int len = GetWindowTextLength(g_hLogEdit);
        SendMessage(g_hLogEdit, EM_SETSEL, len, len);
        SendMessage(g_hLogEdit, EM_REPLACESEL, 0, (LPARAM)pText);
        SendMessage(g_hLogEdit, EM_REPLACESEL, 0, (LPARAM)"\r\n");
        delete[] pText;
        return 0;
    }
    case WM_DESTROY:
        remove_tray_icon();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    const char* CNAME = "WBFCS";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CNAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    g_hMainWnd = CreateWindowEx(0, CNAME, CNAME,
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 260, NULL, NULL, hInst, NULL);

    if (!g_hMainWnd) return 0;

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    CloseHandle(CreateThread(NULL, 0, AppLogic, NULL, 0, NULL));

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}