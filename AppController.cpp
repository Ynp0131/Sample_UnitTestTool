#include "AppController.h"
#include <algorithm>
#include <Windows.h>

namespace {
    std::string wideToUtf8(const std::wstring& text) {
        if (text.empty()) {
            return {};
        }

        int size = WideCharToMultiByte(
            CP_UTF8,
            0,
            text.c_str(),
            static_cast<int>(text.size()),
            nullptr,
            0,
            nullptr,
            nullptr
        );
        std::string result(size, '\0');
        WideCharToMultiByte(
            CP_UTF8,
            0,
            text.c_str(),
            static_cast<int>(text.size()),
            result.data(),
            size,
            nullptr,
            nullptr
        );
        return result;
    }

    std::wstring utf8ToWide(const std::string& text) {
        if (text.empty()) {
            return {};
        }

        int size = MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text.c_str(),
            static_cast<int>(text.size()),
            nullptr,
            0
        );
        if (size == 0) {
            return L"文字列を表示できません";
        }

        std::wstring result(size, L'\0');
        MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text.c_str(),
            static_cast<int>(text.size()),
            result.data(),
            size
        );
        return result;
    }

    std::vector<std::wstring> createTodoItems(const TodoTool& todoTool) {
        std::vector<std::wstring> items;
        for (const auto& [key, todos] : todoTool.getAllTodos()) {
            for (const std::string& todo : todos) {
                items.push_back(L"[" + std::to_wstring(key) + L"] " + utf8ToWide(todo));
            }
        }
        std::sort(items.begin(), items.end());
        return items;
    }

    std::vector<std::wstring> createMemoItems(const MemoTool& memoTool) {
        std::vector<std::wstring> items;
        const auto& memos = memoTool.getAllMemos();
        items.reserve(memos.size());
        for (std::size_t i = 0; i < memos.size(); ++i) {
            items.push_back(L"[" + std::to_wstring(i + 1) + L"] " + utf8ToWide(memos[i].title));
        }
        return items;
    }

    std::vector<std::wstring> createLogItems(const LogTool& logTool) {
        std::vector<std::wstring> items;
        const auto& logs = logTool.getAllLogs();
        items.reserve(logs.size());
        for (const auto& log : logs) {
            items.push_back(utf8ToWide(log));
        }
        return items;
    }
}

AppController::AppController() {
    mailContentTool.setLogCallback([this](const std::string& message) {
        logTool.addLog(message);
    });
    fileTool.setLogCallback([this](const std::string& message) {
        logTool.addLog(message);
    });
}

AppController::~AppController() {
    stopFileMonitoring();
    stopMailInputMonitoring();
}

void AppController::handleCommand(int commandId, CommandCallback callback) {
    if (commandId < 1 || commandId > 5) {
        callback({false, 0, L"", L"", {}});
        return;
    }

    lastPressedId = commandId;

    switch (commandId) {
        case 1:
            todoTool.execute();
            logTool.addLog("TodoTool opened");
            callback({true, commandId, L"TODO Manager", L"TodoToolの処理を実行しました。", createTodoItems(todoTool)});
            return;
        case 2:
            memoTool.execute();
            logTool.addLog("MemoTool opened");
            callback({true, commandId, L"Memo Manager", L"Memoを管理できます。", createMemoItems(memoTool)});
            return;
        case 3:
            logTool.execute();
            logTool.addLog("LogTool opened");
            callback({true, commandId, L"Log Viewer", L"Logを確認・追記できます。", createLogItems(logTool)});
            return;
        case 4:
            fileTool.organizeDesktopNow();
            logTool.addLog("Desktop folders organized");
            callback({true, commandId, L"Desktop Organizer", L"デスクトップフォルダの整理を実行しました。", fileTool.getRecentEvents()});
            return;
        case 5:
            mailContentTool.execute();
            logTool.addLog("Mail generator opened");
            callback({true, commandId, L"メール内容生成ツール", L"報告内容を入力してメール文面を生成できます。", {}});
            return;
        default:
            callback({false, 0, L"", L"", {}});
            return;
    }
}

void AppController::addTodo(const std::wstring& todoText, CommandCallback callback) {
    if (todoText.empty()) {
        logTool.addLog("Todo add rejected: empty input");
        callback({false, 1, L"TODO Manager", L"Todoを入力してください。", {}});
        return;
    }

    todoTool.addTodo(wideToUtf8(todoText));
    logTool.addLog("Todo added");
    callback({true, 1, L"TODO Manager", L"Todoを追加しました。", createTodoItems(todoTool)});
}

void AppController::deleteTodo(int key, std::size_t index, CommandCallback callback) {
    const auto* todos = todoTool.getTodos(key);
    if (todos == nullptr || index >= todos->size()) {
        logTool.addLog("Todo delete failed: target not found");
        callback({false, 1, L"TODO Manager", L"削除対象のTodoが見つかりませんでした。", createTodoItems(todoTool)});
        return;
    }

    if (!todoTool.removeTodo(key, index)) {
        logTool.addLog("Todo delete failed");
        callback({false, 1, L"TODO Manager", L"Todoの削除に失敗しました。", createTodoItems(todoTool)});
        return;
    }

    logTool.addLog("Todo deleted");
    callback({true, 1, L"TODO Manager", L"Todoを削除しました。", createTodoItems(todoTool)});
}

void AppController::editTodo(int key, std::size_t index, const std::wstring& todoText, CommandCallback callback) {
    const auto* todos = todoTool.getTodos(key);
    if (todos == nullptr || index >= todos->size()) {
        logTool.addLog("Todo update failed: target not found");
        callback({false, 1, L"TODO Manager", L"編集対象のTodoが見つかりませんでした。", createTodoItems(todoTool)});
        return;
    }

    if (todoText.empty()) {
        logTool.addLog("Todo update rejected: empty input");
        callback({false, 1, L"TODO Manager", L"Todoを入力してください。", createTodoItems(todoTool)});
        return;
    }

    if (!todoTool.updateTodo(key, index, wideToUtf8(todoText))) {
        logTool.addLog("Todo update failed");
        callback({false, 1, L"TODO Manager", L"Todoの更新に失敗しました。", createTodoItems(todoTool)});
        return;
    }

    logTool.addLog("Todo updated");
    callback({true, 1, L"TODO Manager", L"Todoを更新しました。", createTodoItems(todoTool)});
}

void AppController::addMemo(const std::wstring& title, const std::wstring& body, CommandCallback callback) {
    if (title.empty() && body.empty()) {
        logTool.addLog("Memo add rejected: empty input");
        callback({false, 2, L"Memo Manager", L"タイトルまたは本文を入力してください。", createMemoItems(memoTool)});
        return;
    }

    memoTool.addMemo(wideToUtf8(title), wideToUtf8(body));
    logTool.addLog("Memo added");
    callback({true, 2, L"Memo Manager", L"メモを追加しました。", createMemoItems(memoTool)});
}

void AppController::updateMemo(std::size_t index, const std::wstring& title, const std::wstring& body, CommandCallback callback) {
    if (title.empty() && body.empty()) {
        logTool.addLog("Memo update rejected: empty input");
        callback({false, 2, L"Memo Manager", L"タイトルまたは本文を入力してください。", createMemoItems(memoTool)});
        return;
    }

    if (!memoTool.updateMemo(index, wideToUtf8(title), wideToUtf8(body))) {
        logTool.addLog("Memo update failed: target not found");
        callback({false, 2, L"Memo Manager", L"更新対象のメモが見つかりませんでした。", createMemoItems(memoTool)});
        return;
    }

    logTool.addLog("Memo updated");
    callback({true, 2, L"Memo Manager", L"メモを更新しました。", createMemoItems(memoTool)});
}

void AppController::deleteMemo(std::size_t index, CommandCallback callback) {
    if (!memoTool.removeMemo(index)) {
        logTool.addLog("Memo delete failed: target not found");
        callback({false, 2, L"Memo Manager", L"削除対象のメモが見つかりませんでした。", createMemoItems(memoTool)});
        return;
    }

    logTool.addLog("Memo deleted");
    callback({true, 2, L"Memo Manager", L"メモを削除しました。", createMemoItems(memoTool)});
}

bool AppController::getMemo(std::size_t index, std::wstring& title, std::wstring& body) const {
    MemoTool::MemoEntry memo;
    if (!memoTool.getMemo(index, memo)) {
        return false;
    }

    title = utf8ToWide(memo.title);
    body = utf8ToWide(memo.body);
    return true;
}

void AppController::addLog(const std::wstring& logText, CommandCallback callback) {
    if (logText.empty()) {
        logTool.addLog("Log add rejected: empty input");
        callback({false, 3, L"Log Viewer", L"ログ内容を入力してください。", createLogItems(logTool)});
        return;
    }

    logTool.addLog(wideToUtf8(logText));
    callback({true, 3, L"Log Viewer", L"ログを追記しました。", createLogItems(logTool)});
}

void AppController::deleteLog(std::size_t index, CommandCallback callback) {
    if (!logTool.removeLog(index)) {
        logTool.addLog("Log delete failed: target not found");
        callback({false, 3, L"Log Viewer", L"削除対象のログが見つかりませんでした。", createLogItems(logTool)});
        return;
    }

    logTool.addLog("Log deleted");
    callback({true, 3, L"Log Viewer", L"ログを削除しました。", createLogItems(logTool)});
}

void AppController::generateMailContent(const std::wstring& inputText, CommandCallback callback) {
    if (inputText.empty()) {
        logTool.addLog("Mail generation rejected: empty input");
        callback({false, 5, L"メール内容生成ツール", L"本文を入力してください。", {}});
        return;
    }

    std::string generated;
    std::string savedPath;
    std::string error;
    if (!mailContentTool.generateMailFromInputPipeline(wideToUtf8(inputText), generated, savedPath, error)) {
        logTool.addLog("Mail generation failed");
        callback({false, 5, L"メール内容生成ツール", utf8ToWide(error), {}});
        return;
    }

    logTool.addLog("Mail generated");
    const std::wstring message =
        L"保存先: " + utf8ToWide(savedPath) +
        L"\r\n\r\n" +
        utf8ToWide(generated);

    callback({true, 5, L"メール内容生成ツール", message, {}});
}

void AppController::startMailInputMonitoring() {
    mailContentTool.startInputMonitoring();
    logTool.addLog("Mail input monitoring started");
}

void AppController::stopMailInputMonitoring() {
    mailContentTool.stopInputMonitoring();
    logTool.addLog("Mail input monitoring stopped");
}

void AppController::startFileMonitoring() {
    fileTool.startMonitoring();
    logTool.addLog("File monitoring started");
}

void AppController::stopFileMonitoring() {
    fileTool.stopMonitoring();
    logTool.addLog("File monitoring stopped");
}

void AppController::organizeDownloadsFiles() {
    fileTool.organizeDownloadsNow();
    logTool.addLog("Downloads organized");
}

void AppController::organizeDesktopFiles() {
    fileTool.organizeDesktopNow();
    logTool.addLog("Desktop organized");
}

bool AppController::isFileMonitoringRunning() const {
    return fileTool.isMonitoring();
}

std::vector<std::wstring> AppController::getFileMonitorLog() const {
    return fileTool.getRecentEvents();
}

int AppController::getLastPressedId() const {
    return lastPressedId;
}
