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