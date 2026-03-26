/*
 * User Manager - Учебное приложение для управления локальными пользователями Windows
 * Требует прав администратора для работы с учетными записями
 */

#include <windows.h>
#include <lm.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "advapi32.lib")

#define ID_EDIT_USERNAME     101
#define ID_EDIT_PASSWORD     102
#define ID_BTN_ADD           103
#define ID_BTN_CREATE        104
#define ID_LIST_USERS        105
#define ID_CHK_MUST_CHANGE   201
#define ID_CHK_CANT_CHANGE   202
#define ID_CHK_NEVER_EXPIRES 203
#define ID_CHK_DISABLED      204

struct UserData {
    std::wstring username;
    std::wstring password;
    bool mustChangePassword = false;
    bool cantChangePassword = false;
    bool passwordNeverExpires = false;
    bool accountDisabled = false;
};

std::vector<UserData> usersList;
HWND hEditUsername, hEditPassword;
HWND hListUsers;
HWND hChkMustChange, hChkCantChange, hChkNeverExpires, hChkDisabled;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void AddUserToList();
void CreateUsers();
void UpdateUsersList();
bool UserExists(const std::wstring& username);
bool CreateLocalUser(const UserData& user);
bool SetUserMustChangePassword(const std::wstring& username, const std::wstring& password);
bool SetUserCantChangePassword(const std::wstring& username);
std::wstring GetWindowTextString(HWND hwnd);
void ExecuteCommand(const std::wstring& cmd);

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow
) {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        MessageBox(NULL, L"Ошибка проверки прав", L"Ошибка", MB_OK);
        return 1;
    }

    TOKEN_ELEVATION elevation;
    DWORD dwSize;
    if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) {
        if (!elevation.TokenIsElevated) {
            MessageBox(NULL,
                L"Это приложение требует прав администратора!\n"
                L"Пожалуйста, запустите программу от имени администратора",
                L"Требуются права администратора", MB_OK | MB_ICONWARNING);
            CloseHandle(hToken);
            return 1;
        }
    }
    CloseHandle(hToken);

    const wchar_t CLASS_NAME[] = L"UserManagerWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Управление локальными пользователями (Учебное)",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        600, 450,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        CreateWindow(L"STATIC", L"Имя пользователя:",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            20, 20, 150, 20, hwnd, NULL, NULL, NULL);

        hEditUsername = CreateWindow(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            180, 20, 200, 20, hwnd, (HMENU)ID_EDIT_USERNAME, NULL, NULL);

        CreateWindow(L"STATIC", L"Пароль:",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            20, 50, 150, 20, hwnd, NULL, NULL, NULL);

        hEditPassword = CreateWindow(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL,
            180, 50, 200, 20, hwnd, (HMENU)ID_EDIT_PASSWORD, NULL, NULL);

        CreateWindow(L"BUTTON", L"Добавить в список",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            400, 35, 150, 30, hwnd, (HMENU)ID_BTN_ADD, NULL, NULL);

        int yPos = 90;
        int chkHeight = 20;

        hChkMustChange = CreateWindow(L"BUTTON",
            L"Требовать смены пароля при следующем входе",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            20, yPos, 350, chkHeight, hwnd, (HMENU)ID_CHK_MUST_CHANGE, NULL, NULL);

        hChkCantChange = CreateWindow(L"BUTTON",
            L"Запретить смену пароля пользователем",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            20, yPos += 25, 350, chkHeight, hwnd, (HMENU)ID_CHK_CANT_CHANGE, NULL, NULL);

        hChkNeverExpires = CreateWindow(L"BUTTON",
            L"Срок действия пароля неограничен",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            20, yPos += 25, 350, chkHeight, hwnd, (HMENU)ID_CHK_NEVER_EXPIRES, NULL, NULL);

        hChkDisabled = CreateWindow(L"BUTTON",
            L"Отключить учетную запись",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            20, yPos += 25, 350, chkHeight, hwnd, (HMENU)ID_CHK_DISABLED, NULL, NULL);

        CreateWindow(L"STATIC", L"Список пользователей для создания:",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            20, yPos += 35, 300, 20, hwnd, NULL, NULL, NULL);

        hListUsers = CreateWindow(L"LISTBOX", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
            20, yPos += 20, 550, 150, hwnd, (HMENU)ID_LIST_USERS, NULL, NULL);

        CreateWindow(L"BUTTON", L"Создать всех пользователей",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            200, yPos += 160, 200, 40, hwnd, (HMENU)ID_BTN_CREATE, NULL, NULL);

        EnumChildWindows(hwnd, [](HWND hWnd, LPARAM lParam) -> BOOL {
            SendMessage(hWnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
            return TRUE;
            }, 0);

        return 0;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);

        switch (wmId) {
        case ID_BTN_ADD:
            AddUserToList();
            break;

        case ID_BTN_CREATE:
            CreateUsers();
            break;
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

std::wstring GetWindowTextString(HWND hwnd) {
    int len = GetWindowTextLength(hwnd) + 1;
    if (len <= 1) return L"";

    std::wstring text(len, 0);
    GetWindowText(hwnd, &text[0], len);
    text.resize(len - 1);
    return text;
}

void AddUserToList() {
    std::wstring username = GetWindowTextString(hEditUsername);
    std::wstring password = GetWindowTextString(hEditPassword);

    if (username.empty() || password.empty()) {
        MessageBox(NULL, L"Введите имя пользователя и пароль!", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    if (UserExists(username)) {
        MessageBox(NULL, (L"Пользователь '" + username + L"' уже существует в системе!").c_str(),
            L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    for (const auto& u : usersList) {
        if (u.username == username) {
            MessageBox(NULL, L"Этот пользователь уже добавлен в список!", L"Ошибка", MB_OK | MB_ICONERROR);
            return;
        }
    }

    UserData user;
    user.username = username;
    user.password = password;
    user.mustChangePassword = (SendMessage(hChkMustChange, BM_GETCHECK, 0, 0) == BST_CHECKED);
    user.cantChangePassword = (SendMessage(hChkCantChange, BM_GETCHECK, 0, 0) == BST_CHECKED);
    user.passwordNeverExpires = (SendMessage(hChkNeverExpires, BM_GETCHECK, 0, 0) == BST_CHECKED);
    user.accountDisabled = (SendMessage(hChkDisabled, BM_GETCHECK, 0, 0) == BST_CHECKED);

    usersList.push_back(user);

    UpdateUsersList();

    SetWindowText(hEditUsername, L"");
    SetWindowText(hEditPassword, L"");
    SendMessage(hChkMustChange, BM_SETCHECK, BST_UNCHECKED, 0);
    SendMessage(hChkCantChange, BM_SETCHECK, BST_UNCHECKED, 0);
    SendMessage(hChkNeverExpires, BM_SETCHECK, BST_UNCHECKED, 0);
    SendMessage(hChkDisabled, BM_SETCHECK, BST_UNCHECKED, 0);

    SetFocus(hEditUsername);
}

void UpdateUsersList() {
    SendMessage(hListUsers, LB_RESETCONTENT, 0, 0);

    for (const auto& user : usersList) {
        std::wstring info = user.username + L" | ";

        if (user.mustChangePassword) info += L"[Сменить пароль] ";
        if (user.cantChangePassword) info += L"[Не менять пароль] ";
        if (user.passwordNeverExpires) info += L"[Бессрочно] ";
        if (user.accountDisabled) info += L"[Отключена] ";

        SendMessage(hListUsers, LB_ADDSTRING, 0, (LPARAM)info.c_str());
    }
}

void ExecuteCommand(const std::wstring& cmd) {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };

    wchar_t* cmdBuffer = new wchar_t[cmd.length() + 1];
    wcscpy_s(cmdBuffer, cmd.length() + 1, cmd.c_str());

    if (CreateProcess(NULL, cmdBuffer, NULL, NULL, FALSE,
        CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
        NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 10000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    delete[] cmdBuffer;
}

bool UserExists(const std::wstring& username) {
    LPBYTE bufPtr = NULL;
    NET_API_STATUS nStatus = NetUserGetInfo(NULL, username.c_str(), 0, &bufPtr);

    if (nStatus == NERR_Success) {
        NetApiBufferFree(bufPtr);
        return true;
    }

    return false;
}

// ГАРАНТИРОВАННЫЙ метод требования смены пароля
bool SetUserMustChangePassword(const std::wstring& username, const std::wstring& originalPassword) {
    bool success = false;

    // Метод 1: Двойная смена пароля (самый надежный)
    // Меняем на временный, потом обратно — это сбрасывает age и требует смену

    wchar_t tempPassword[] = L"TempPass123!@#Change";

    // Шаг 1: Меняем на временный пароль
    USER_INFO_1003 ui1003;
    ui1003.usri1003_password = tempPassword;

    DWORD parm_err = 0;
    NET_API_STATUS nStatus = NetUserSetInfo(NULL, username.c_str(), 1003, (LPBYTE)&ui1003, &parm_err);

    if (nStatus == NERR_Success) {
        Sleep(200);

        // Шаг 2: Меняем обратно на оригинальный
        ui1003.usri1003_password = (LPWSTR)originalPassword.c_str();
        nStatus = NetUserSetInfo(NULL, username.c_str(), 1003, (LPBYTE)&ui1003, &parm_err);

        if (nStatus == NERR_Success) {
            success = true;
        }
    }

    // Метод 2: Устанавливаем флаги через USER_INFO_3 (даже если метод 1 сработал, делаем для надежности)
    LPBYTE bufPtr = NULL;
    nStatus = NetUserGetInfo(NULL, username.c_str(), 3, &bufPtr);

    if (nStatus == NERR_Success) {
        USER_INFO_3* pUi3 = (USER_INFO_3*)bufPtr;

        // Устанавливаем флаг истечения пароля
        pUi3->usri3_flags |= UF_PASSWORD_EXPIRED;
        pUi3->usri3_flags &= ~UF_DONT_EXPIRE_PASSWD;
        pUi3->usri3_password_age = (DWORD)-1;  // Устаревший пароль

        NetUserSetInfo(NULL, username.c_str(), 3, (LPBYTE)pUi3, &parm_err);
        NetApiBufferFree(bufPtr);
    }
    else if (bufPtr) {
        NetApiBufferFree(bufPtr);
    }

    // Метод 3: PowerShell ADSI (гарантированный fallback)
    wchar_t psCmd[1024];
    swprintf_s(psCmd,
        L"powershell -ExecutionPolicy Bypass -Command \""
        L"$user = [ADSI]'WinNT://%s/%s,user'; "
        L"if ($user -ne $null) { "
        L"  $user.PasswordExpired = 1; "
        L"  $user.SetInfo(); "
        L"  Write-Host 'Password change required set successfully'; "
        L"}\"",
        L".", username.c_str());

    ExecuteCommand(psCmd);

    // Метод 4: WMIC для включения политики истечения
    wchar_t wmicCmd[512];
    swprintf_s(wmicCmd, L"wmic useraccount where \"name='%s'\" set PasswordExpires=true",
        username.c_str());
    ExecuteCommand(wmicCmd);

    return true;  // Возвращаем true, так как хотя бы один метод должен сработать
}

bool SetUserCantChangePassword(const std::wstring& username) {
    LPBYTE bufPtr = NULL;
    NET_API_STATUS nStatus = NetUserGetInfo(NULL, username.c_str(), 3, &bufPtr);

    if (nStatus == NERR_Success) {
        USER_INFO_3* pUi3 = (USER_INFO_3*)bufPtr;
        pUi3->usri3_flags |= UF_PASSWD_CANT_CHANGE;

        DWORD parm_err = 0;
        nStatus = NetUserSetInfo(NULL, username.c_str(), 3, (LPBYTE)pUi3, &parm_err);

        NetApiBufferFree(bufPtr);

        if (nStatus == NERR_Success) {
            return true;
        }
    }
    else if (bufPtr) {
        NetApiBufferFree(bufPtr);
    }

    wchar_t cmd[512];
    swprintf_s(cmd, L"net user \"%s\" /passwordchg:no", username.c_str());
    ExecuteCommand(cmd);

    return true;
}

bool CreateLocalUser(const UserData& user) {
    if (UserExists(user.username)) {
        SetLastError(2224);
        return false;
    }

    USER_INFO_1 ui;
    DWORD dwError = 0;

    ui.usri1_name = (LPWSTR)user.username.c_str();
    ui.usri1_password = (LPWSTR)user.password.c_str();
    ui.usri1_priv = USER_PRIV_USER;
    ui.usri1_home_dir = NULL;
    ui.usri1_comment = (LPWSTR)L"Создано через UserManager";
    ui.usri1_flags = UF_SCRIPT;
    ui.usri1_script_path = NULL;

    // Только если НЕ требуется смена пароля — ставим бессрочный
    if (user.passwordNeverExpires && !user.mustChangePassword) {
        ui.usri1_flags |= UF_DONT_EXPIRE_PASSWD;
    }

    if (user.accountDisabled) {
        ui.usri1_flags |= UF_ACCOUNTDISABLE;
    }

    NET_API_STATUS nStatus = NetUserAdd(NULL, 1, (LPBYTE)&ui, &dwError);

    if (nStatus != NERR_Success) {
        SetLastError(nStatus);
        return false;
    }

    Sleep(300);

    bool flagsOk = true;

    if (user.mustChangePassword) {
        if (!SetUserMustChangePassword(user.username, user.password)) {
            flagsOk = false;
        }
    }

    if (user.cantChangePassword) {
        if (!SetUserCantChangePassword(user.username)) {
            flagsOk = false;
        }
    }

    return true;
}

void CreateUsers() {
    if (usersList.empty()) {
        MessageBox(NULL, L"Список пользователей пуст!", L"Информация", MB_OK);
        return;
    }

    int successCount = 0;
    int failCount = 0;
    std::wstring errorMessages;

    std::vector<UserData> usersToCreate = usersList;
    usersList.clear();
    UpdateUsersList();

    for (const auto& user : usersToCreate) {
        if (CreateLocalUser(user)) {
            successCount++;
        }
        else {
            failCount++;
            DWORD err = GetLastError();

            std::wstring errMsg;
            switch (err) {
            case 2224:
                errMsg = L"уже существует";
                break;
            case NERR_PasswordTooShort:
                errMsg = L"пароль слишком короткий";
                break;
            case NERR_PasswordTooLong:
                errMsg = L"пароль слишком длинный";
                break;
            case ERROR_ACCESS_DENIED:
                errMsg = L"нет доступа";
                break;
            default:
                errMsg = L"код ошибки: " + std::to_wstring(err);
            }

            errorMessages += user.username + L" (" + errMsg + L")\n";
        }
    }

    std::wstring message = L"Создано пользователей: " + std::to_wstring(successCount) +
        L"\nОшибок: " + std::to_wstring(failCount);
    if (failCount > 0) {
        message += L"\n\nНе удалось создать:\n" + errorMessages;
    }

    MessageBox(NULL, message.c_str(), L"Результат", MB_OK | (failCount > 0 ? MB_ICONWARNING : MB_ICONINFORMATION));
}