#pragma once

#include <Windows.h>
#include <string>
#include <vector>

class AppController;
struct CommandResult;
struct TodoWindowData;

// View層: ウィンドウとボタンを作成し、ユーザー操作をControllerへ渡す。
class MainWindow {
private:
    HINSTANCE instance;
    AppController& controller;
    HWND windowHandle = nullptr;
    HWND statusLabel = nullptr;
    HWND monitorWindow = nullptr;
    HWND monitorText = nullptr;
    HWND monitorToggleButton = nullptr;
    HWND monitorCloseButton = nullptr;

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK toolWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK monitorWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void createButtons();
    void createStatusLabel();
    void createMonitorWindow();
    void updateDisplayForCommand(int commandId);
    void openToolWindow(const CommandResult& result);
    void updateTodoList(HWND list, const std::vector<std::wstring>& todoItems);
    void updateMonitorWindow();
    void updateMonitorToggleButton();
    void toggleMonitorWindow();

public:
    MainWindow(HINSTANCE instance, AppController& controller);
    bool create();
    void show(int commandShow);
    HWND getHandle() const;
};
