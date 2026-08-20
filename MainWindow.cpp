#include "MainWindow.h"
#include "AppController.h"

namespace {
    constexpr wchar_t WindowClassName[] = L"My Assistant Sayaka";
    constexpr int ButtonWidth = 120;
    constexpr int ButtonHeight = 30;
    constexpr int ButtonLeft = 20;
    constexpr int ButtonTop = 20;
    constexpr int ButtonGap = 10;
    constexpr wchar_t ToolWindowClassName[] = L"My Assistant Tool Window";
    constexpr wchar_t MonitorWindowClassName[] = L"My Assistant Monitor Window";
    constexpr int TodoAddButtonId = 101;
    constexpr int TodoDeleteButtonId = 102;
    constexpr int TodoEditButtonId = 103;
    constexpr int TodoSaveButtonId = 104;
    constexpr int MemoAddButtonId = 111;
    constexpr int MemoDeleteButtonId = 112;
    constexpr int MemoSaveButtonId = 113;
    constexpr int LogAddButtonId = 121;
    constexpr int LogDeleteButtonId = 122;
    constexpr int LogSaveButtonId = 123;
    constexpr int MemoUpdateButtonId = 124;
    constexpr int ToolListControlId = 300;
    constexpr int MemoTitleEditControlId = 301;
    constexpr int MemoBodyEditControlId = 302;
    constexpr int EmailInputEditControlId = 320;
    constexpr int EmailOutputEditControlId = 321;
    constexpr int EmailGenerateButtonId = 322;
    constexpr int MonitorTodoButtonId = 210;
    constexpr int MonitorMemoButtonId = 211;
    constexpr int MonitorLogButtonId = 212;
    constexpr int MonitorDesktopButtonId = 213;
    constexpr int MonitorMailButtonId = 214;
    constexpr int MonitorToggleButtonId = 200;
    constexpr int MonitorCloseButtonId = 202;

    struct TodoItemRef {
        int key;
        std::size_t index;
    };

    std::vector<TodoItemRef> parseTodoItemRefs(const std::vector<std::wstring>& todoItems) {
        std::vector<TodoItemRef> refs;
        std::unordered_map<int, std::size_t> seenCount;

        for (const std::wstring& todoItem : todoItems) {
            if (todoItem.empty() || todoItem.front() != L'[') {
                continue;
            }

            const std::size_t closeIndex = todoItem.find(L']');
            if (closeIndex == std::wstring::npos) {
                continue;
            }

            const std::wstring keyText = todoItem.substr(1, closeIndex - 1);
            int key = 0;
            try {
                key = std::stoi(keyText);
            } catch (...) {
                continue;
            }

            refs.push_back({key, seenCount[key]++});
        }

        return refs;
    }

    std::wstring extractTodoText(const std::wstring& fullText) {
        const std::size_t closeIndex = fullText.find(L']');
        if (closeIndex == std::wstring::npos) {
            return fullText;
        }

        std::size_t textStart = closeIndex + 1;
        while (textStart < fullText.size() && fullText[textStart] == L' ') {
            ++textStart;
        }

        return fullText.substr(textStart);
    }
}

struct TodoWindowData {
    MainWindow* owner;
    int commandId = 0;
    HWND list = nullptr;
    HWND input = nullptr;
    HWND inputSecondary = nullptr;
    HWND output = nullptr;
    HWND saveButton = nullptr;
    HWND updateButton = nullptr;
    bool editing = false;
    int editKey = -1;
    std::size_t editIndex = 0;
    std::vector<TodoItemRef> todoEntries;
};

MainWindow::MainWindow(HINSTANCE instance, AppController& controller)
    : instance(instance), controller(controller) {
}

bool MainWindow::create() {
    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = MainWindow::windowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = WindowClassName;

    if (!RegisterClassW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    windowHandle = CreateWindowExW(
        0,
        WindowClassName,
        L"My Assistant Sayaka",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        nullptr,
        nullptr,
        instance,
        this
    );

    if (windowHandle == nullptr) {
        return false;
    }

    RegisterHotKey(windowHandle, 1, MOD_CONTROL | MOD_ALT, static_cast<UINT>('S'));
    SetTimer(windowHandle, 2, 1000, nullptr);
    createButtons();
    createStatusLabel();
    createMonitorWindow();
    controller.startFileMonitoring();
    controller.startMailInputMonitoring();
    ShowWindow(monitorWindow, SW_HIDE);
    updateMonitorWindow();
    return true;
}

void MainWindow::createButtons() {
    const wchar_t* labels[] = {
        L"TodoTool",
        L"MemoTool",
        L"LogTool",
        L"Desktop整理",
        L"メール内容生成"
    };

    for (int index = 0; index < 5; ++index) {
        CreateWindowW(
            L"BUTTON",
            labels[index],
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            ButtonLeft,
            ButtonTop + index * (ButtonHeight + ButtonGap),
            ButtonWidth,
            ButtonHeight,
            windowHandle,
            reinterpret_cast<HMENU>(index + 1),
            instance,
            nullptr
        );
    }
}

void MainWindow::createStatusLabel() {
    statusLabel = CreateWindowW(
        L"STATIC",
        L"ツールを選択してください",
        WS_CHILD | WS_VISIBLE,
        180, 25, 180, 30,
        windowHandle,
        nullptr,
        instance,
        nullptr
    );
}

void MainWindow::createMonitorWindow() {
    WNDCLASSW monitorClass = {};
    monitorClass.lpfnWndProc = MainWindow::monitorWindowProc;
    monitorClass.hInstance = instance;
    monitorClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    monitorClass.lpszClassName = MonitorWindowClassName;

    if (!RegisterClassW(&monitorClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return;
    }

    monitorWindow = CreateWindowExW(
        0,
        MonitorWindowClassName,
        L"Download Monitor",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 340, 260,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (monitorWindow == nullptr) {
        return;
    }

    SetWindowLongPtrW(monitorWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    monitorText = CreateWindowW(
        L"STATIC",
        L"監視状態...",
        WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
        12, 12, 300, 54,
        monitorWindow,
        nullptr,
        instance,
        nullptr
    );

    CreateWindowW(
        L"BUTTON",
        L"TodoTool",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        12, 74, 95, 28,
        monitorWindow,
        reinterpret_cast<HMENU>(MonitorTodoButtonId),
        instance,
        nullptr
    );

    CreateWindowW(
        L"BUTTON",
        L"MemoTool",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        117, 74, 95, 28,
        monitorWindow,
        reinterpret_cast<HMENU>(MonitorMemoButtonId),
        instance,
        nullptr
    );

    CreateWindowW(
        L"BUTTON",
        L"LogTool",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        222, 74, 95, 28,
        monitorWindow,
        reinterpret_cast<HMENU>(MonitorLogButtonId),
        instance,
        nullptr
    );

    CreateWindowW(
        L"BUTTON",
        L"Desktop整理",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        12, 110, 95, 28,
        monitorWindow,
        reinterpret_cast<HMENU>(MonitorDesktopButtonId),
        instance,
        nullptr
    );

    CreateWindowW(
        L"BUTTON",
        L"メール生成",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        222, 110, 95, 28,
        monitorWindow,
        reinterpret_cast<HMENU>(MonitorMailButtonId),
        instance,
        nullptr
    );

    monitorToggleButton = CreateWindowW(
        L"BUTTON",
        L"監視を停止",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        117, 150, 200, 28,
        monitorWindow,
        reinterpret_cast<HMENU>(MonitorToggleButtonId),
        instance,
        nullptr
    );

    monitorCloseButton = CreateWindowW(
        L"BUTTON",
        L"Close",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        12, 190, 305, 28,
        monitorWindow,
        reinterpret_cast<HMENU>(MonitorCloseButtonId),
        instance,
        nullptr
    );

    ShowWindow(monitorWindow, SW_HIDE);
    UpdateWindow(monitorWindow);
}

void MainWindow::updateDisplayForCommand(int commandId) {
    const wchar_t* displayText = L"未選択";

    // View縺ｯController縺御ｿ晄戟縺吶ｋ蜈ｱ騾唔D繧定ｦ九※縲∬｡ｨ遉ｺ縺縺代ｒ蛻・ｊ譖ｿ縺医ｋ縲・
    switch (commandId) {
        case 1:
            displayText = L"選択中: TodoTool";
            break;
        case 2:
            displayText = L"選択中: MemoTool";
            break;
        case 3:
            displayText = L"選択中: LogTool";
            break;
        case 4:
            displayText = L"選択中: Desktop整理";
            break;
        case 5:
            displayText = L"選択中: メール内容生成";
            break;
        default:
            break;
    }

    if (statusLabel != nullptr) {
        SetWindowTextW(statusLabel, displayText);
    }
}

void MainWindow::updateTodoList(HWND list, const std::vector<std::wstring>& todoItems) {
    if (list == nullptr) {
        return;
    }

    SendMessageW(list, LB_RESETCONTENT, 0, 0);

    if (todoItems.empty()) {
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Todoはありません"));
        return;
    }

    for (std::size_t index = 0; index < todoItems.size(); ++index) {
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(todoItems[index].c_str()));
        SendMessageW(list, LB_SETITEMDATA, static_cast<WPARAM>(index), static_cast<LPARAM>(index));
    }
}

void MainWindow::openToolWindow(const CommandResult& result) {
    WNDCLASSW toolWindowClass = {};
    toolWindowClass.lpfnWndProc = MainWindow::toolWindowProc;
    toolWindowClass.hInstance = instance;
    toolWindowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    toolWindowClass.lpszClassName = ToolWindowClassName;

    if (!RegisterClassW(&toolWindowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return;
    }

    int windowWidth = 420;
    int windowHeight = 360;
    if (result.commandId == 2) {
        windowWidth = 760;
        windowHeight = 560;
    } else if (result.commandId == 5) {
        windowWidth = 780;
        windowHeight = 620;
    }

    HWND toolWindow = CreateWindowExW(
        0,
        ToolWindowClassName,
        result.title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (toolWindow == nullptr) {
        return;
    }

    TodoWindowData* todoWindowData = new TodoWindowData();
    todoWindowData->owner = this;
    todoWindowData->commandId = result.commandId;
    SetWindowLongPtrW(toolWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(todoWindowData));

    if (result.commandId == 1 || result.commandId == 3) {
        HWND list = CreateWindowW(
            L"LISTBOX",
            nullptr,
            WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY,
            20, 20, 360, 190,
            toolWindow,
            reinterpret_cast<HMENU>(ToolListControlId),
            instance,
            nullptr
        );
        todoWindowData->list = list;

        if (result.todoItems.empty()) {
            const wchar_t* emptyText = L"データはありません";
            if (result.commandId == 1) {
                emptyText = L"Todoはありません";
            } else if (result.commandId == 2) {
                emptyText = L"メモはありません";
            } else if (result.commandId == 3) {
                emptyText = L"ログはありません";
            }
            SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(emptyText));
        } else {
            for (std::size_t index = 0; index < result.todoItems.size(); ++index) {
                SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(result.todoItems[index].c_str()));
                SendMessageW(list, LB_SETITEMDATA, static_cast<WPARAM>(index), static_cast<LPARAM>(index));
            }
        }

        if (result.commandId == 1) {
            todoWindowData->todoEntries = parseTodoItemRefs(result.todoItems);
            CreateWindowW(
                L"BUTTON",
                L"Add",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                20, 225, 100, 30,
                toolWindow,
                reinterpret_cast<HMENU>(TodoAddButtonId),
                instance,
                nullptr
            );
            CreateWindowW(
                L"BUTTON",
                L"Delete",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                130, 225, 100, 30,
                toolWindow,
                reinterpret_cast<HMENU>(TodoDeleteButtonId),
                instance,
                nullptr
            );
            CreateWindowW(
                L"BUTTON",
                L"Edit",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                240, 225, 100, 30,
                toolWindow,
                reinterpret_cast<HMENU>(TodoEditButtonId),
                instance,
                nullptr
            );
        } else if (result.commandId == 2) {
            CreateWindowW(
                L"BUTTON",
                L"Add",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                20, 225, 100, 30,
                toolWindow,
                reinterpret_cast<HMENU>(MemoAddButtonId),
                instance,
                nullptr
            );
            CreateWindowW(
                L"BUTTON",
                L"Delete",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                130, 225, 100, 30,
                toolWindow,
                reinterpret_cast<HMENU>(MemoDeleteButtonId),
                instance,
                nullptr
            );
        } else if (result.commandId == 3) {
            CreateWindowW(
                L"BUTTON",
                L"Add",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                20, 225, 100, 30,
                toolWindow,
                reinterpret_cast<HMENU>(LogAddButtonId),
                instance,
                nullptr
            );
            CreateWindowW(
                L"BUTTON",
                L"Delete",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                130, 225, 100, 30,
                toolWindow,
                reinterpret_cast<HMENU>(LogDeleteButtonId),
                instance,
                nullptr
            );
        }
    } else if (result.commandId == 2) {
        HWND list = CreateWindowW(
            L"LISTBOX",
            nullptr,
            WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY,
            20, 20, 220, 450,
            toolWindow,
            reinterpret_cast<HMENU>(ToolListControlId),
            instance,
            nullptr
        );
        todoWindowData->list = list;

        if (result.todoItems.empty()) {
            SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"メモはありません"));
        } else {
            for (std::size_t index = 0; index < result.todoItems.size(); ++index) {
                SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(result.todoItems[index].c_str()));
                SendMessageW(list, LB_SETITEMDATA, static_cast<WPARAM>(index), static_cast<LPARAM>(index));
            }
        }

        CreateWindowW(
            L"STATIC",
            L"タイトル",
            WS_CHILD | WS_VISIBLE,
            260, 20, 100, 20,
            toolWindow,
            nullptr,
            instance,
            nullptr
        );

        todoWindowData->inputSecondary = CreateWindowW(
            L"EDIT",
            nullptr,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            260, 44, 460, 28,
            toolWindow,
            reinterpret_cast<HMENU>(MemoTitleEditControlId),
            instance,
            nullptr
        );

        CreateWindowW(
            L"STATIC",
            L"本文",
            WS_CHILD | WS_VISIBLE,
            260, 82, 100, 20,
            toolWindow,
            nullptr,
            instance,
            nullptr
        );

        todoWindowData->input = CreateWindowW(
            L"EDIT",
            nullptr,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL,
            260, 106, 460, 280,
            toolWindow,
            reinterpret_cast<HMENU>(MemoBodyEditControlId),
            instance,
            nullptr
        );

        CreateWindowW(
            L"BUTTON",
            L"Add",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            260, 405, 100, 30,
            toolWindow,
            reinterpret_cast<HMENU>(MemoAddButtonId),
            instance,
            nullptr
        );

        todoWindowData->updateButton = CreateWindowW(
            L"BUTTON",
            L"Update",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            370, 405, 100, 30,
            toolWindow,
            reinterpret_cast<HMENU>(MemoUpdateButtonId),
            instance,
            nullptr
        );

        CreateWindowW(
            L"BUTTON",
            L"Delete",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            480, 405, 100, 30,
            toolWindow,
            reinterpret_cast<HMENU>(MemoDeleteButtonId),
            instance,
            nullptr
        );
    } else if (result.commandId == 5) {
        CreateWindowW(
            L"STATIC",
            L"起こったこと・報告したいことを入力 (日報は先頭に 種別:日報)",
            WS_CHILD | WS_VISIBLE,
            20, 20, 500, 20,
            toolWindow,
            nullptr,
            instance,
            nullptr
        );

        todoWindowData->input = CreateWindowW(
            L"EDIT",
            nullptr,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL,
            20, 48, 720, 220,
            toolWindow,
            reinterpret_cast<HMENU>(EmailInputEditControlId),
            instance,
            nullptr
        );

        CreateWindowW(
            L"BUTTON",
            L"input保存してメール生成",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            20, 280, 200, 30,
            toolWindow,
            reinterpret_cast<HMENU>(EmailGenerateButtonId),
            instance,
            nullptr
        );

        CreateWindowW(
            L"STATIC",
            L"生成結果",
            WS_CHILD | WS_VISIBLE,
            20, 322, 500, 20,
            toolWindow,
            nullptr,
            instance,
            nullptr
        );

        todoWindowData->output = CreateWindowW(
            L"EDIT",
            nullptr,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL | ES_READONLY,
            20, 350, 720, 210,
            toolWindow,
            reinterpret_cast<HMENU>(EmailOutputEditControlId),
            instance,
            nullptr
        );
    } else {
        CreateWindowW(
            L"STATIC",
            result.message.c_str(),
            WS_CHILD | WS_VISIBLE,
            20, 30, 360, 30,
            toolWindow,
            nullptr,
            instance,
            nullptr
        );
    }

    ShowWindow(toolWindow, SW_SHOW);
    UpdateWindow(toolWindow);
}

void MainWindow::updateMonitorWindow() {
    if (monitorText == nullptr) {
        return;
    }

    std::wstring status = L"Download Organizer\r\n";
    status += controller.isFileMonitoringRunning() ? L"監視中" : L"停止中";
    const auto log = controller.getFileMonitorLog();
    if (!log.empty()) {
        status += L"\r\n";
        for (std::size_t index = 0; index < log.size() && index < 5; ++index) {
            if (index > 0) {
                status += L"\r\n";
            }
            status += log[index];
        }
    }

    SetWindowTextW(monitorText, status.c_str());
    updateMonitorToggleButton();
    if (monitorWindow != nullptr && IsWindowVisible(monitorWindow)) {
        RedrawWindow(monitorWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

void MainWindow::updateMonitorToggleButton() {
    if (monitorToggleButton == nullptr) {
        return;
    }

    const wchar_t* buttonText = controller.isFileMonitoringRunning() ? L"監視停止" : L"監視開始";
    SetWindowTextW(monitorToggleButton, buttonText);
}

void MainWindow::toggleMonitorWindow() {
    if (monitorWindow == nullptr) {
        return;
    }

    updateMonitorWindow();
    if (IsWindowVisible(monitorWindow)) {
        ShowWindow(monitorWindow, SW_HIDE);
    } else {
        ShowWindow(monitorWindow, SW_SHOWNORMAL);
        SetForegroundWindow(monitorWindow);
    }
}

void MainWindow::show(int commandShow) {
    ShowWindow(windowHandle, SW_HIDE);
    UpdateWindow(windowHandle);
}

HWND MainWindow::getHandle() const {
    return windowHandle;
}

LRESULT CALLBACK MainWindow::windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    MainWindow* window = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE) {
        const CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        window = static_cast<MainWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }

    switch (message) {
        case WM_COMMAND:
            if (window != nullptr) {
                // Controller縺ｮ蜃ｦ逅・ｮ御ｺ・夂衍繧団allback縺ｧ蜿励￠蜿悶ｊ縲〃iew繧呈峩譁ｰ縺吶ｋ縲・
                window->controller.handleCommand(
                    LOWORD(wParam),
                    [window](const CommandResult& result) {
                        if (!result.handled) {
                            return;
                        }

                        window->updateDisplayForCommand(result.commandId);

                        switch (result.commandId) {
                            case 1:
                                // ID=1縺ｮTodo繝・・繧ｿ繧呈眠縺励＞Window縺ｮListBox縺ｸ荳隕ｧ陦ｨ遉ｺ縺吶ｋ縲・
                                window->openToolWindow(result);
                                break;
                            case 2:
                                window->openToolWindow(result);
                                break;
                            case 3:
                                window->openToolWindow(result);
                                break;
                            case 4:
                                window->updateMonitorWindow();
                                break;
                            case 5:
                                window->openToolWindow(result);
                                break;
                            default:
                                break;
                        }
                    }
                );
                return 0;
            }
            break;
        case WM_HOTKEY:
            if (window != nullptr && wParam == 1) {
                window->toggleMonitorWindow();
                return 0;
            }
            break;
        case WM_TIMER:
            if (window != nullptr && wParam == static_cast<WPARAM>(2)) {
                if (IsWindowVisible(window->monitorWindow)) {
                    window->updateMonitorWindow();
                }
            }
            break;
        case WM_CLOSE:
            if (window != nullptr) {
                // 繝舌ャ繧ｯ繧ｰ繝ｩ繧ｦ繝ｳ繝牙ｸｸ鬧舌ｒ蜆ｪ蜈医＠縲√Γ繧､繝ｳ繧ｦ繧｣繝ｳ繝峨え縺ｯ髢峨§縺壹↓髫縺吶・
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }
            break;
        case WM_DESTROY:
            UnregisterHotKey(hwnd, 1);
            KillTimer(hwnd, 2);
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK MainWindow::toolWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    TodoWindowData* data = reinterpret_cast<TodoWindowData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE) {
        const CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        data = static_cast<TodoWindowData*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
    }

    if (message == WM_COMMAND && data != nullptr && HIWORD(wParam) == LBN_SELCHANGE) {
        if (data->commandId == 2 && LOWORD(wParam) == ToolListControlId && data->list != nullptr) {
            const int selectedIndex = static_cast<int>(SendMessageW(data->list, LB_GETCURSEL, 0, 0));
            if (selectedIndex != LB_ERR) {
                const int itemDataIndex = static_cast<int>(SendMessageW(data->list, LB_GETITEMDATA, selectedIndex, 0));
                if (itemDataIndex >= 0) {
                    std::wstring title;
                    std::wstring body;
                    if (data->owner->controller.getMemo(static_cast<std::size_t>(itemDataIndex), title, body)) {
                        if (data->inputSecondary != nullptr) {
                            SetWindowTextW(data->inputSecondary, title.c_str());
                        }
                        if (data->input != nullptr) {
                            SetWindowTextW(data->input, body.c_str());
                        }
                        data->editing = true;
                        data->editIndex = static_cast<std::size_t>(itemDataIndex);
                    }
                }
            }
            return 0;
        }
    }

    if (message == WM_COMMAND && data != nullptr && HIWORD(wParam) == BN_CLICKED) {
        auto readControlText = [](HWND control) {
            if (control == nullptr) {
                return std::wstring();
            }
            int length = GetWindowTextLengthW(control);
            std::wstring text(length + 1, L'\0');
            GetWindowTextW(control, text.data(), length + 1);
            text.resize(length);
            return text;
        };

        switch (LOWORD(wParam)) {
            case TodoAddButtonId:
                if (data->input == nullptr) {
                    data->editing = false;
                    data->editKey = -1;
                    data->editIndex = 0;
                    data->input = CreateWindowW(
                        L"EDIT",
                        nullptr,
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                        20, 265, 250, 28,
                        hwnd,
                        nullptr,
                        data->owner->instance,
                        nullptr
                    );
                    data->saveButton = CreateWindowW(
                        L"BUTTON",
                        L"Save",
                        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                        280, 265, 100, 28,
                        hwnd,
                        reinterpret_cast<HMENU>(TodoSaveButtonId),
                        data->owner->instance,
                        nullptr
                    );
                    SetFocus(data->input);
                }
                return 0;
            case TodoDeleteButtonId: {
                if (data->list == nullptr) {
                    return 0;
                }

                const int selectedIndex = static_cast<int>(SendMessageW(data->list, LB_GETCURSEL, 0, 0));
                if (selectedIndex == LB_ERR) {
                    return 0;
                }

                const int itemDataIndex = static_cast<int>(SendMessageW(data->list, LB_GETITEMDATA, selectedIndex, 0));
                if (itemDataIndex < 0 || itemDataIndex >= static_cast<int>(data->todoEntries.size())) {
                    return 0;
                }

                const TodoItemRef& entry = data->todoEntries[itemDataIndex];
                data->owner->controller.deleteTodo(
                    entry.key,
                    entry.index,
                    [data](const CommandResult& result) {
                        if (result.handled) {
                            data->todoEntries = parseTodoItemRefs(result.todoItems);
                            data->owner->updateTodoList(data->list, result.todoItems);
                        }
                    }
                );
                return 0;
            }
            case TodoEditButtonId: {
                if (data->list == nullptr) {
                    return 0;
                }

                const int selectedIndex = static_cast<int>(SendMessageW(data->list, LB_GETCURSEL, 0, 0));
                if (selectedIndex == LB_ERR) {
                    return 0;
                }

                const int itemDataIndex = static_cast<int>(SendMessageW(data->list, LB_GETITEMDATA, selectedIndex, 0));
                if (itemDataIndex < 0 || itemDataIndex >= static_cast<int>(data->todoEntries.size())) {
                    return 0;
                }

                const TodoItemRef& entry = data->todoEntries[itemDataIndex];
                wchar_t textBuffer[512] = {};
                SendMessageW(data->list, LB_GETTEXT, selectedIndex, reinterpret_cast<LPARAM>(textBuffer));
                const std::wstring selectedText = extractTodoText(textBuffer);

                if (data->input == nullptr) {
                    data->editing = true;
                    data->editKey = entry.key;
                    data->editIndex = entry.index;
                    data->input = CreateWindowW(
                        L"EDIT",
                        selectedText.c_str(),
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                        20, 265, 250, 28,
                        hwnd,
                        nullptr,
                        data->owner->instance,
                        nullptr
                    );
                    data->saveButton = CreateWindowW(
                        L"BUTTON",
                        L"Update",
                        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                        280, 265, 100, 28,
                        hwnd,
                        reinterpret_cast<HMENU>(TodoSaveButtonId),
                        data->owner->instance,
                        nullptr
                    );
                    SetFocus(data->input);
                }
                return 0;
            }
            case TodoSaveButtonId:
                if (data->input != nullptr) {
                    int length = GetWindowTextLengthW(data->input);
                    std::wstring todoText(length + 1, L'\0');
                    GetWindowTextW(data->input, todoText.data(), length + 1);
                    todoText.resize(length);

                    auto onSaved = [data](const CommandResult& result) {
                        if (result.handled) {
                            data->todoEntries = parseTodoItemRefs(result.todoItems);
                            data->owner->updateTodoList(data->list, result.todoItems);
                        }

                        if (data->input != nullptr) {
                            DestroyWindow(data->input);
                            data->input = nullptr;
                        }
                        if (data->saveButton != nullptr) {
                            DestroyWindow(data->saveButton);
                            data->saveButton = nullptr;
                        }
                        data->editing = false;
                        data->editKey = -1;
                        data->editIndex = 0;
                    };

                    if (data->editing) {
                        data->owner->controller.editTodo(
                            data->editKey,
                            data->editIndex,
                            todoText,
                            onSaved
                        );
                    } else {
                        data->owner->controller.addTodo(
                            todoText,
                            onSaved
                        );
                    }
                }
                return 0;
            case MemoAddButtonId:
                if (data->inputSecondary == nullptr || data->input == nullptr) {
                    return 0;
                }

                data->owner->controller.addMemo(
                    readControlText(data->inputSecondary),
                    readControlText(data->input),
                    [data](const CommandResult& result) {
                        if (result.handled) {
                            data->owner->updateTodoList(data->list, result.todoItems);
                            if (data->inputSecondary != nullptr) {
                                SetWindowTextW(data->inputSecondary, L"");
                            }
                            if (data->input != nullptr) {
                                SetWindowTextW(data->input, L"");
                            }
                            data->editing = false;
                        }
                    }
                );
                return 0;
            case MemoUpdateButtonId:
                if (data->inputSecondary == nullptr || data->input == nullptr || !data->editing) {
                    return 0;
                }

                data->owner->controller.updateMemo(
                    data->editIndex,
                    readControlText(data->inputSecondary),
                    readControlText(data->input),
                    [data](const CommandResult& result) {
                        if (result.handled) {
                            data->owner->updateTodoList(data->list, result.todoItems);
                        }
                    }
                );
                return 0;
            case MemoDeleteButtonId: {
                if (data->list == nullptr) {
                    return 0;
                }

                const int selectedIndex = static_cast<int>(SendMessageW(data->list, LB_GETCURSEL, 0, 0));
                if (selectedIndex == LB_ERR) {
                    return 0;
                }

                const int itemDataIndex = static_cast<int>(SendMessageW(data->list, LB_GETITEMDATA, selectedIndex, 0));
                if (itemDataIndex < 0) {
                    return 0;
                }

                data->owner->controller.deleteMemo(
                    static_cast<std::size_t>(itemDataIndex),
                    [data](const CommandResult& result) {
                        if (result.handled) {
                            data->owner->updateTodoList(data->list, result.todoItems);
                        }
                    }
                );
                return 0;
            }
            case LogAddButtonId:
                if (data->input == nullptr) {
                    data->input = CreateWindowW(
                        L"EDIT",
                        nullptr,
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                        20, 265, 250, 28,
                        hwnd,
                        nullptr,
                        data->owner->instance,
                        nullptr
                    );
                    data->saveButton = CreateWindowW(
                        L"BUTTON",
                        L"Save",
                        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                        280, 265, 100, 28,
                        hwnd,
                        reinterpret_cast<HMENU>(LogSaveButtonId),
                        data->owner->instance,
                        nullptr
                    );
                    SetFocus(data->input);
                }
                return 0;
            case LogDeleteButtonId: {
                if (data->list == nullptr) {
                    return 0;
                }

                const int selectedIndex = static_cast<int>(SendMessageW(data->list, LB_GETCURSEL, 0, 0));
                if (selectedIndex == LB_ERR) {
                    return 0;
                }

                const int itemDataIndex = static_cast<int>(SendMessageW(data->list, LB_GETITEMDATA, selectedIndex, 0));
                if (itemDataIndex < 0) {
                    return 0;
                }

                data->owner->controller.deleteLog(
                    static_cast<std::size_t>(itemDataIndex),
                    [data](const CommandResult& result) {
                        if (result.handled) {
                            data->owner->updateTodoList(data->list, result.todoItems);
                        }
                    }
                );
                return 0;
            }
            case LogSaveButtonId:
                if (data->input != nullptr) {
                    int length = GetWindowTextLengthW(data->input);
                    std::wstring logText(length + 1, L'\0');
                    GetWindowTextW(data->input, logText.data(), length + 1);
                    logText.resize(length);

                    data->owner->controller.addLog(
                        logText,
                        [data](const CommandResult& result) {
                            if (result.handled) {
                                data->owner->updateTodoList(data->list, result.todoItems);
                            }

                            if (data->input != nullptr) {
                                DestroyWindow(data->input);
                                data->input = nullptr;
                            }
                            if (data->saveButton != nullptr) {
                                DestroyWindow(data->saveButton);
                                data->saveButton = nullptr;
                            }
                        }
                    );
                }
                return 0;
            case EmailGenerateButtonId:
                if (data->input == nullptr || data->output == nullptr) {
                    return 0;
                }

                data->owner->controller.generateMailContent(
                    readControlText(data->input),
                    [data](const CommandResult& result) {
                        if (data->output != nullptr) {
                            SetWindowTextW(data->output, result.message.c_str());
                        }
                    }
                );
                return 0;
            default:
                break;
        }
    }

    if (message == WM_NCDESTROY) {
        delete data;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK MainWindow::monitorWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    MainWindow* window = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
        case WM_COMMAND:
            if (window != nullptr && LOWORD(wParam) >= MonitorTodoButtonId && LOWORD(wParam) <= MonitorMailButtonId) {
                int commandId = 0;
                switch (LOWORD(wParam)) {
                    case MonitorTodoButtonId:
                        commandId = 1;
                        break;
                    case MonitorMemoButtonId:
                        commandId = 2;
                        break;
                    case MonitorLogButtonId:
                        commandId = 3;
                        break;
                    case MonitorDesktopButtonId:
                        commandId = 4;
                        break;
                    case MonitorMailButtonId:
                        commandId = 5;
                        break;
                    default:
                        break;
                }

                if (commandId != 0) {
                    window->controller.handleCommand(
                        commandId,
                        [window](const CommandResult& result) {
                            if (!result.handled) {
                                return;
                            }

                            window->updateDisplayForCommand(result.commandId);
                            if (result.commandId == 4) {
                                window->updateMonitorWindow();
                            } else {
                                window->openToolWindow(result);
                            }
                        }
                    );
                }
                return 0;
            }
            if (window != nullptr && LOWORD(wParam) == MonitorToggleButtonId) {
                if (window->controller.isFileMonitoringRunning()) {
                    window->controller.stopFileMonitoring();
                } else {
                    window->controller.startFileMonitoring();
                }
                window->updateMonitorWindow();
                return 0;
            }
            if (LOWORD(wParam) == MonitorCloseButtonId) {
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }
            break;
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        default:
            break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}






