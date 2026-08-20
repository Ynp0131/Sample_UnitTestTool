#pragma once

#include "ToolModels.h"
#include <functional>
#include <string>
#include <vector>

struct CommandResult {
    bool handled;
    int commandId;
    std::wstring title;
    std::wstring message;
    std::vector<std::wstring> todoItems;
};

using CommandCallback = std::function<void(const CommandResult&)>;

class AppController {
private:
    TodoTool todoTool;
    MemoTool memoTool;
    LogTool logTool;
    MailContentTool mailContentTool;
    FileTool fileTool;
    int lastPressedId = 0;

public:
    AppController();
    ~AppController();

    void handleCommand(int commandId, CommandCallback callback);
    void addTodo(const std::wstring& todoText, CommandCallback callback);
    void deleteTodo(int key, std::size_t index, CommandCallback callback);
    void editTodo(int key, std::size_t index, const std::wstring& todoText, CommandCallback callback);
    void addMemo(const std::wstring& title, const std::wstring& body, CommandCallback callback);
    void updateMemo(std::size_t index, const std::wstring& title, const std::wstring& body, CommandCallback callback);
    void deleteMemo(std::size_t index, CommandCallback callback);
    bool getMemo(std::size_t index, std::wstring& title, std::wstring& body) const;
    void addLog(const std::wstring& logText, CommandCallback callback);
    void deleteLog(std::size_t index, CommandCallback callback);
    void generateMailContent(const std::wstring& inputText, CommandCallback callback);

    void startMailInputMonitoring();
    void stopMailInputMonitoring();
    void startFileMonitoring();
    void stopFileMonitoring();
    void organizeDownloadsFiles();
    void organizeDesktopFiles();

    bool isFileMonitoringRunning() const;
    std::vector<std::wstring> getFileMonitorLog() const;
    int getLastPressedId() const;
};
