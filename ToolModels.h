#pragma once

#define NOMINMAX

#include "Tool.h"
#include "LocalAi.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <sstream>
#include <set>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <ctime>
#include <unordered_map>
#include <vector>
#include <windows.h>
#include <shlobj.h>

// Model螻､: TodoTool縺梧桶縺・後く繝ｼ縺斐→縺ｮTodo荳隕ｧ縲阪ｒ邂｡逅・☆繧九・
class TodoTool : public Tool {
private:
    std::unordered_map<int, std::vector<std::string>> todosByKey;
    std::string toolName = "TodoTool";
    int position = 0;
    const char* fileName = "todos.json";

    static void skipWhitespace(const std::string& json, std::size_t& index) {
        while (index < json.size() && std::isspace(static_cast<unsigned char>(json[index]))) {
            ++index;
        }
    }

    static bool readJsonString(const std::string& json, std::size_t& index, std::string& value) {
        if (index >= json.size() || json[index] != '"') {
            return false;
        }

        ++index;
        value.clear();
        while (index < json.size()) {
            char character = json[index++];
            if (character == '"') {
                return true;
            }
            if (character == '\\' && index < json.size()) {
                char escaped = json[index++];
                switch (escaped) {
                    case '"': value += '"'; break;
                    case '\\': value += '\\'; break;
                    case 'n': value += '\n'; break;
                    case 'r': value += '\r'; break;
                    case 't': value += '\t'; break;
                    default: return false;
                }
            } else {
                value += character;
            }
        }
        return false;
    }

    static std::string escapeJsonString(const std::string& value) {
        std::string escaped;
        for (char character : value) {
            switch (character) {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += character; break;
            }
        }
        return escaped;
    }

    void loadFromJson() {
        std::ifstream file(fileName, std::ios::binary);
        if (!file) {
            return;
        }

        std::string json(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );
        std::size_t index = 0;
        skipWhitespace(json, index);
        if (index >= json.size() || json[index++] != '{') {
            return;
        }

        while (index < json.size()) {
            skipWhitespace(json, index);
            if (index < json.size() && json[index] == '}') {
                break;
            }

            std::string keyText;
            if (!readJsonString(json, index, keyText)) {
                return;
            }
            skipWhitespace(json, index);
            if (index >= json.size() || json[index++] != ':') {
                return;
            }
            skipWhitespace(json, index);
            if (index >= json.size() || json[index++] != '[') {
                return;
            }

            int key = 0;
            try {
                key = std::stoi(keyText);
            } catch (...) {
                return;
            }

            while (index < json.size()) {
                skipWhitespace(json, index);
                if (index < json.size() && json[index] == ']') {
                    ++index;
                    break;
                }

                std::string todo;
                if (!readJsonString(json, index, todo)) {
                    return;
                }
                todosByKey[key].push_back(todo);
                skipWhitespace(json, index);
                if (index < json.size() && json[index] == ',') {
                    ++index;
                }
            }

            position = std::max(position, key + 1);
            skipWhitespace(json, index);
            if (index < json.size() && json[index] == ',') {
                ++index;
            }
        }

    }

    void saveToJson() const {
        std::ofstream file(fileName, std::ios::binary | std::ios::trunc);
        if (!file) {
            return;
        }

        std::vector<int> keys;
        for (const auto& [key, todos] : todosByKey) {
            keys.push_back(key);
        }
        std::sort(keys.begin(), keys.end());

        file << "{\n";
        for (std::size_t keyIndex = 0; keyIndex < keys.size(); ++keyIndex) {
            int key = keys[keyIndex];
            file << "  \"" << key << "\": [";
            const auto& todos = todosByKey.at(key);
            for (std::size_t todoIndex = 0; todoIndex < todos.size(); ++todoIndex) {
                file << "\"" << escapeJsonString(todos[todoIndex]) << "\"";
                if (todoIndex + 1 < todos.size()) {
                    file << ", ";
                }
            }
            file << "]";
            if (keyIndex + 1 < keys.size()) {
                file << ",";
            }
            file << "\n";
        }
        file << "}\n";
    }

public:
    TodoTool() {
        loadFromJson();
    }

    void execute() override {
    }

    int getPosition() const {
        return position;
    }

    const std::string& getToolName() const {
        return toolName;
    }

    // 譁ｰ縺励＞Todo繧堤樟蝨ｨ縺ｮ繧ｭ繝ｼ縺ｫ霑ｽ蜉縺励∵ｬ｡縺ｮ繧ｭ繝ｼ縺ｸ騾ｲ繧√ｋ縲・
    void addTodo(const std::string& todo) {
        todosByKey[position].push_back(todo);
        ++position;
        saveToJson();
    }

    bool removeTodo(int key, std::size_t index) {
        auto todos = todosByKey.find(key);
        if (todos == todosByKey.end() || index >= todos->second.size()) {
            return false;
        }

        todos->second.erase(todos->second.begin() + index);
        if (todos->second.empty()) {
            todosByKey.erase(todos);
        }
        saveToJson();
        return true;
    }

    bool updateTodo(int key, std::size_t index, const std::string& todo) {
        auto todos = todosByKey.find(key);
        if (todos == todosByKey.end() || index >= todos->second.size() || todo.empty()) {
            return false;
        }

        todos->second[index] = todo;
        saveToJson();
        return true;
    }

    const std::vector<std::string>* getTodos(int key) const {
        auto todos = todosByKey.find(key);
        if (todos == todosByKey.end()) {
            return nullptr;
        }

        return &todos->second;
    }

    const std::unordered_map<int, std::vector<std::string>>& getAllTodos() const {
        return todosByKey;
    }
};

// 莉雁ｾ後◎繧後◇繧後・讖溯・繧貞ｮ溯｣・☆繧貴odel螻､縺ｮ繧ｯ繝ｩ繧ｹ縲・
class MemoTool : public Tool {
public:
    struct MemoEntry {
        std::string title;
        std::string body;
    };

private:
    std::vector<MemoEntry> memos;
    const char* fileName = "memos.json";

    static void skipWhitespace(const std::string& json, std::size_t& index) {
        while (index < json.size() && std::isspace(static_cast<unsigned char>(json[index]))) {
            ++index;
        }
    }

    static bool readJsonString(const std::string& json, std::size_t& index, std::string& value) {
        if (index >= json.size() || json[index] != '"') {
            return false;
        }

        ++index;
        value.clear();
        while (index < json.size()) {
            char character = json[index++];
            if (character == '"') {
                return true;
            }
            if (character == '\\' && index < json.size()) {
                char escaped = json[index++];
                switch (escaped) {
                    case '"': value += '"'; break;
                    case '\\': value += '\\'; break;
                    case 'n': value += '\n'; break;
                    case 'r': value += '\r'; break;
                    case 't': value += '\t'; break;
                    default: return false;
                }
            } else {
                value += character;
            }
        }
        return false;
    }

    static std::string escapeJsonString(const std::string& value) {
        std::string escaped;
        for (char character : value) {
            switch (character) {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += character; break;
            }
        }
        return escaped;
    }

    void loadFromJson() {
        std::ifstream file(fileName, std::ios::binary);
        if (!file) {
            return;
        }

        std::string json(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        std::size_t index = 0;
        skipWhitespace(json, index);
        if (index >= json.size() || json[index++] != '[') {
            return;
        }

        while (index < json.size()) {
            skipWhitespace(json, index);
            if (index < json.size() && json[index] == ']') {
                break;
            }

            MemoEntry entry;
            if (index < json.size() && json[index] == '{') {
                ++index;
                while (index < json.size()) {
                    skipWhitespace(json, index);
                    if (index < json.size() && json[index] == '}') {
                        ++index;
                        break;
                    }

                    std::string key;
                    std::string value;
                    if (!readJsonString(json, index, key)) {
                        return;
                    }
                    skipWhitespace(json, index);
                    if (index >= json.size() || json[index++] != ':') {
                        return;
                    }
                    skipWhitespace(json, index);
                    if (!readJsonString(json, index, value)) {
                        return;
                    }

                    if (key == "title") {
                        entry.title = value;
                    } else if (key == "body") {
                        entry.body = value;
                    }

                    skipWhitespace(json, index);
                    if (index < json.size() && json[index] == ',') {
                        ++index;
                    }
                }
            } else {
                // 譌ｧ蠖｢蠑・譁・ｭ怜・驟榊・)縺ｨ縺ｮ莠呈鋤隱ｭ縺ｿ霎ｼ縺ｿ
                std::string memoBody;
                if (!readJsonString(json, index, memoBody)) {
                    return;
                }
                entry.body = memoBody;
                if (!memoBody.empty()) {
                    const std::size_t lineEnd = memoBody.find('\n');
                    entry.title = memoBody.substr(0, std::min<std::size_t>(lineEnd == std::string::npos ? memoBody.size() : lineEnd, 30));
                }
            }

            if (!entry.title.empty() || !entry.body.empty()) {
                if (entry.title.empty()) {
                    entry.title = "(無題)";
                }
                memos.push_back(entry);
            }

            skipWhitespace(json, index);
            if (index < json.size() && json[index] == ',') {
                ++index;
            }
        }

    }

    void saveToJson() const {
        std::ofstream file(fileName, std::ios::binary | std::ios::trunc);
        if (!file) {
            return;
        }

        file << "[\n";
        for (std::size_t i = 0; i < memos.size(); ++i) {
            file << "  {\"title\": \"" << escapeJsonString(memos[i].title)
                 << "\", \"body\": \"" << escapeJsonString(memos[i].body) << "\"}";
            if (i + 1 < memos.size()) {
                file << ",";
            }
            file << "\n";
        }
        file << "]\n";
    }

public:
    MemoTool() {
        loadFromJson();
    }

    void execute() override {
    }

    void addMemo(const std::string& title, const std::string& body) {
        if (title.empty() && body.empty()) {
            return;
        }
        memos.push_back({title.empty() ? "(無題)" : title, body});
        saveToJson();
    }

    bool updateMemo(std::size_t index, const std::string& title, const std::string& body) {
        if (index >= memos.size()) {
            return false;
        }

        memos[index].title = title.empty() ? "(無題)" : title;
        memos[index].body = body;
        saveToJson();
        return true;
    }

    bool removeMemo(std::size_t index) {
        if (index >= memos.size()) {
            return false;
        }
        memos.erase(memos.begin() + static_cast<std::ptrdiff_t>(index));
        saveToJson();
        return true;
    }

    bool getMemo(std::size_t index, MemoEntry& entry) const {
        if (index >= memos.size()) {
            return false;
        }
        entry = memos[index];
        return true;
    }

    const std::vector<MemoEntry>& getAllMemos() const {
        return memos;
    }
};

class LogTool : public Tool {
private:
    static constexpr std::size_t MaxLogEntries = 200;
    std::vector<std::string> logs;
    const char* fileName = "logs.json";

    static std::string escapeJsonString(const std::string& value) {
        std::string escaped;
        for (char character : value) {
            switch (character) {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += character; break;
            }
        }
        return escaped;
    }

    static void skipWhitespace(const std::string& json, std::size_t& index) {
        while (index < json.size() && std::isspace(static_cast<unsigned char>(json[index]))) {
            ++index;
        }
    }

    static bool readJsonString(const std::string& json, std::size_t& index, std::string& value) {
        if (index >= json.size() || json[index] != '"') {
            return false;
        }

        ++index;
        value.clear();
        while (index < json.size()) {
            char character = json[index++];
            if (character == '"') {
                return true;
            }
            if (character == '\\' && index < json.size()) {
                char escaped = json[index++];
                switch (escaped) {
                    case '"': value += '"'; break;
                    case '\\': value += '\\'; break;
                    case 'n': value += '\n'; break;
                    case 'r': value += '\r'; break;
                    case 't': value += '\t'; break;
                    default: return false;
                }
            } else {
                value += character;
            }
        }
        return false;
    }

    void loadFromJson() {
        std::ifstream file(fileName, std::ios::binary);
        if (!file) {
            return;
        }

        std::string json(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        std::size_t index = 0;
        skipWhitespace(json, index);
        if (index >= json.size() || json[index++] != '[') {
            return;
        }

        while (index < json.size()) {
            skipWhitespace(json, index);
            if (index < json.size() && json[index] == ']') {
                break;
            }

            std::string entry;
            if (!readJsonString(json, index, entry)) {
                return;
            }

            logs.push_back(entry);

            skipWhitespace(json, index);
            if (index < json.size() && json[index] == ',') {
                ++index;
            }
        }

        if (logs.size() > MaxLogEntries) {
            logs.erase(logs.begin(), logs.begin() + static_cast<std::ptrdiff_t>(logs.size() - MaxLogEntries));
            saveToJson();
        }
    }

    void saveToJson() const {
        std::ofstream file(fileName, std::ios::binary | std::ios::trunc);
        if (!file) {
            return;
        }

        file << "[\n";
        for (std::size_t i = 0; i < logs.size(); ++i) {
            file << "  \"" << escapeJsonString(logs[i]) << "\"";
            if (i + 1 < logs.size()) {
                file << ",";
            }
            file << "\n";
        }
        file << "]\n";
    }

    static std::string createTimestamp() {
        std::time_t now = std::time(nullptr);
        std::tm localTime = {};
#if defined(_WIN32)
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif

        std::ostringstream stream;
        stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
        return stream.str();
    }

public:
    LogTool() {
        loadFromJson();
    }

    void execute() override {
        addLog("LogTool executed");
    }

    void addLog(const std::string& message) {
        if (message.empty()) {
            return;
        }

        logs.push_back("[" + createTimestamp() + "] " + message);
        if (logs.size() > MaxLogEntries) {
            logs.erase(logs.begin(), logs.begin() + static_cast<std::ptrdiff_t>(logs.size() - MaxLogEntries));
        }
        saveToJson();
    }

    bool removeLog(std::size_t index) {
        if (index >= logs.size()) {
            return false;
        }
        logs.erase(logs.begin() + static_cast<std::ptrdiff_t>(index));
        saveToJson();
        return true;
    }

    const std::vector<std::string>& getAllLogs() const {
        return logs;
    }
};

class MailContentTool : public Tool {
private:
    std::atomic<bool> monitoring = false;
    std::thread monitorThread;
    std::filesystem::file_time_type lastProcessedWriteTime = std::filesystem::file_time_type::min();
    mutable std::mutex ioMutex;
    std::function<void(const std::string&)> logCallback;

    static std::string trim(const std::string& text) {
        std::size_t begin = 0;
        while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
            ++begin;
        }
        std::size_t end = text.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
            --end;
        }
        return text.substr(begin, end - begin);
    }

    static std::string createTimestampForFileName() {
        std::time_t now = std::time(nullptr);
        std::tm localTime = {};
#if defined(_WIN32)
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif

        std::ostringstream stream;
        stream << std::put_time(&localTime, "%Y%m%d_%H%M%S");
        return stream.str();
    }

    static std::filesystem::path ensureInputDirectory() {
        std::filesystem::path inputDir = std::filesystem::current_path() / "input";
        std::filesystem::create_directories(inputDir);
        return inputDir;
    }

    static std::filesystem::path ensureOutputDirectory() {
        std::filesystem::path outputDir = std::filesystem::current_path() / "output";
        std::filesystem::create_directories(outputDir);
        return outputDir;
    }

    static std::filesystem::path getOutputFilePath() {
        return ensureOutputDirectory() / "generated_mail.txt";
    }

    static std::filesystem::path createUniqueInputPath(const std::filesystem::path& inputDir) {
        const std::string baseName = "input_" + createTimestampForFileName();
        std::filesystem::path candidate = inputDir / (baseName + ".txt");

        int suffix = 1;
        while (std::filesystem::exists(candidate)) {
            candidate = inputDir / (baseName + "_" + std::to_string(suffix) + ".txt");
            ++suffix;
        }

        return candidate;
    }

    static bool saveTextFile(const std::filesystem::path& filePath, const std::string& content) {
        std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
        if (!file) {
            return false;
        }
        file << content;
        return static_cast<bool>(file);
    }

    static bool readTextFile(const std::filesystem::path& filePath, std::string& content) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            return false;
        }
        content.assign(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        );
        return true;
    }

    static std::vector<std::string> toMeaningfulLines(const std::string& rawInput) {
        std::vector<std::string> lines;
        std::string current;
        for (char ch : rawInput) {
            if (ch == '\r') {
                continue;
            }
            if (ch == '\n') {
                const std::string line = trim(current);
                if (!line.empty()) {
                    lines.push_back(line);
                }
                current.clear();
            } else {
                current.push_back(ch);
            }
        }

        const std::string last = trim(current);
        if (!last.empty()) {
            lines.push_back(last);
        }
        return lines;
    }

    static std::string stripBulletPrefix(const std::string& line) {
        std::string normalized = trim(line);
        const std::vector<std::string> prefixes = {"-", "*", "・", "●", "○", "◆", "■", "□", "※", "→", "↑"};

        bool changed = true;
        while (!normalized.empty() && changed) {
            changed = false;
            for (const auto& prefix : prefixes) {
                if (normalized.rfind(prefix, 0) == 0) {
                    normalized.erase(0, prefix.size());
                    normalized = trim(normalized);
                    changed = true;
                    break;
                }
            }
            if (normalized.size() >= 2 && std::isdigit(static_cast<unsigned char>(normalized[0])) && (normalized[1] == '.' || normalized[1] == ')')) {
                normalized.erase(0, 2);
                normalized = trim(normalized);
                changed = true;
            }
        }
        return normalized;
    }

    static bool hasSentenceEnding(const std::string& text) {
        if (text.empty()) {
            return false;
        }
        const std::vector<std::string> endings = {"。", "!", "！", "?", "？", "."};
        for (const auto& ending : endings) {
            if (text.size() >= ending.size() && text.compare(text.size() - ending.size(), ending.size(), ending) == 0) {
                return true;
            }
        }
        return false;
    }

    static std::string toSentence(const std::string& text) {
        const std::string normalized = stripBulletPrefix(text);
        if (normalized.empty()) {
            return {};
        }
        if (hasSentenceEnding(normalized)) {
            return normalized;
        }
        return normalized + "。";
    }

    static std::string joinSentences(const std::vector<std::string>& lines) {
        std::ostringstream out;
        bool first = true;
        for (const auto& line : lines) {
            const std::string sentence = toSentence(line);
            if (sentence.empty()) {
                continue;
            }
            if (!first) {
                out << " ";
            }
            out << sentence;
            first = false;
        }
        return out.str();
    }

    static bool shouldGenerateDailyReport(const std::vector<std::string>& lines) {
        if (lines.empty()) {
            return false;
        }

        const std::string first = lines.front();
        if (first.find("種別:日報") != std::string::npos || first.find("mode:daily") != std::string::npos || first.find("日報") != std::string::npos) {
            return true;
        }
        return false;
    }

    std::string generateDailyReport(const std::string& rawInput) const {
        std::vector<std::string> done;
        std::vector<std::string> issues;
        std::vector<std::string> next;
        const std::vector<std::string> lines = toMeaningfulLines(rawInput);
        for (const auto& original : lines) {
            const std::string line = stripBulletPrefix(original);
            if (line.empty() || line.find("種別:") == 0 || line.find("mode:") == 0) continue;
            if (line.find("課題:") == 0 || line.find("懸念:") == 0) issues.push_back(line.substr(line.find(':') + 1));
            else if (line.find("明日:") == 0 || line.find("予定:") == 0) next.push_back(line.substr(line.find(':') + 1));
            else done.push_back(line);
        }
        std::ostringstream out;
        out << "件名: 日報\n\n";
        out << "お疲れ様です。\n";
        out << "本日の日報をご報告します。\n\n";

        out << "【本日の実施内容】\n";
        if (done.empty()) {
            out << "本日は共有すべき実施事項はありませんでした。\n";
        } else {
            out << joinSentences(done) << "\n";
        }

        out << "\n【課題・懸念】\n";
        if (issues.empty()) {
            out << "現時点で大きな課題・懸念はありません。\n";
        } else {
            out << joinSentences(issues) << "\n";
        }

        out << "\n【明日の予定】\n";
        if (next.empty()) {
            out << "明日は本日の対応内容を継続して進める予定です。\n";
        } else {
            out << joinSentences(next) << "\n";
        }

        out << "\n以上、よろしくお願いいたします。\n";
        return out.str();
    }

    std::filesystem::path getLatestInputFile(bool& found) const {
        found = false;
        const std::filesystem::path inputDir = ensureInputDirectory();
        std::filesystem::path latestPath;
        std::filesystem::file_time_type latestTime = std::filesystem::file_time_type::min();

        for (const auto& entry : std::filesystem::directory_iterator(inputDir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const std::wstring fileName = entry.path().filename().wstring();
            if (fileName.rfind(L"input_", 0) != 0 || entry.path().extension() != L".txt") {
                continue;
            }

            const auto writeTime = entry.last_write_time();
            if (!found || writeTime > latestTime) {
                latestTime = writeTime;
                latestPath = entry.path();
                found = true;
            }
        }

        return latestPath;
    }

    bool processLatestInputFileInternal(bool onlyWhenUpdated, std::string& outputText) {
        bool found = false;
        const std::filesystem::path latestPath = getLatestInputFile(found);
        if (!found) {
            outputText = "入力ファイルが見つかりません。";
            return false;
        }

        const auto latestWriteTime = std::filesystem::last_write_time(latestPath);
        if (onlyWhenUpdated && latestWriteTime <= lastProcessedWriteTime) {
            return false;
        }

        std::string rawInput;
        if (!readTextFile(latestPath, rawInput)) {
            outputText = "最新inputファイルの読み込みに失敗しました。";
            return false;
        }

        const std::vector<std::string> lines = toMeaningfulLines(rawInput);
        const bool daily = shouldGenerateDailyReport(lines);
        std::string errorMessage;
        if (!LocalAiClient::generate(rawInput, daily, outputText, errorMessage)) {
            outputText = errorMessage;
            if (logCallback) {
                logCallback("Mail input processing failed");
            }
            return false;
        }

        if (!saveTextFile(getOutputFilePath(), outputText)) {
            outputText = "output/generated_mail.txt への保存に失敗しました。";
            if (logCallback) {
                logCallback("Mail output save failed");
            }
            return false;
        }

        lastProcessedWriteTime = latestWriteTime;
        if (logCallback) {
            logCallback(daily ? "Daily report generated from input" : "Mail generated from input");
        }
        return true;
    }

public:
    void setLogCallback(std::function<void(const std::string&)> callback) {
        logCallback = std::move(callback);
    }

    ~MailContentTool() {
        stopInputMonitoring();
    }

    void execute() override {
        startInputMonitoring();
    }

    void startInputMonitoring() {
        if (monitoring) {
            return;
        }

        monitoring = true;
        monitorThread = std::thread([this]() {
            while (monitoring) {
                {
                    std::lock_guard<std::mutex> lock(ioMutex);
                    std::string generated;
                    processLatestInputFileInternal(true, generated);
                }
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });
    }

    void stopInputMonitoring() {
        monitoring = false;
        if (monitorThread.joinable()) {
            monitorThread.join();
        }
    }

    bool generateMailFromInputPipeline(
        const std::string& rawInput,
        std::string& generatedMail,
        std::string& savedFilePath,
        std::string& errorMessage
    ) {
        try {
            const std::filesystem::path inputDir = ensureInputDirectory();
            const std::filesystem::path inputFilePath = createUniqueInputPath(inputDir);

            if (!saveTextFile(inputFilePath, rawInput)) {
                errorMessage = "inputファイルの保存に失敗しました。";
                return false;
            }

            {
                std::lock_guard<std::mutex> lock(ioMutex);
                std::string generated;
                processLatestInputFileInternal(false, generated);
            }

            if (!readTextFile(getOutputFilePath(), generatedMail)) {
                errorMessage = "output/generated_mail.txt の読み込みに失敗しました。";
                return false;
            }

            savedFilePath = inputFilePath.string();
            return true;
        } catch (const std::filesystem::filesystem_error&) {
            errorMessage = "input/output フォルダ処理でエラーが発生しました。";
            return false;
        }
    }

    std::string generateMail(const std::string& rawInput) const {
        const std::vector<std::string> cleaned = toMeaningfulLines(rawInput);
        std::vector<std::string> reportLines;
        std::vector<std::string> requestLines;

        for (const auto& line : cleaned) {
            const std::string normalized = stripBulletPrefix(line);
            if (normalized.empty()) {
                continue;
            }

            if (normalized.rfind("種別:", 0) == 0 || normalized.rfind("mode:", 0) == 0) {
                continue;
            }
            if (normalized.rfind("依頼:", 0) == 0 || normalized.rfind("確認:", 0) == 0) {
                const std::size_t pos = normalized.find(':');
                requestLines.push_back(trim(normalized.substr(pos + 1)));
                continue;
            }

            reportLines.push_back(normalized);
        }

        std::ostringstream out;
        out << "件名: ご報告\n\n";
        out << "お疲れ様です。\n";
        out << "いつもお世話になっております。\n\n";
        out << "本件についてご報告いたします。\n";

        if (reportLines.empty()) {
            out << "現時点で共有すべき追加事項はございません。\n";
        } else {
            out << joinSentences(reportLines) << "\n";
        }

        out << "\n";
        if (requestLines.empty()) {
            out << "ご確認いただきたい点がございましたら、ご指摘いただけますと幸いです。\n";
        } else {
            out << "あわせて、" << joinSentences(requestLines) << "\n";
            out << "お手数をおかけしますが、ご確認のほどよろしくお願いいたします。\n";
        }

        out << "\n以上、よろしくお願いいたします。\n";
        return out.str();
    }
};

class FileTool : public Tool {
private:
    std::vector<std::wstring> recentEvents;
    bool monitoring = false;
    std::thread monitorThread;
    std::function<void(const std::string&)> logCallback;

    static std::wstring utf8ToWide(const std::string& text) {
        if (text.empty()) return {};
        int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
        if (size <= 0) return {};
        std::wstring result(size, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), result.data(), size);
        return result;
    }

    static std::string wideToUtf8(const std::wstring& text) {
        if (text.empty()) return {};
        int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
        if (size <= 0) return {};
        std::string result(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), result.data(), size, nullptr, nullptr);
        return result;
    }

    static std::wstring getDesktopPath() {
        wchar_t path[MAX_PATH] = {};
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, path))) {
            return std::wstring(path);
        }
        return L".";
    }

    static std::wstring getDownloadsPath() {
        wchar_t path[MAX_PATH] = {};
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, 0, path))) {
            std::filesystem::path profilePath(path);
            std::filesystem::path downloadsPath = profilePath / L"Downloads";
            if (std::filesystem::exists(downloadsPath)) {
                return downloadsPath.wstring();
            }
        }

        const wchar_t* userProfile = _wgetenv(L"USERPROFILE");
        if (userProfile != nullptr) {
            std::filesystem::path downloadsPath = std::filesystem::path(userProfile) / L"Downloads";
            if (std::filesystem::exists(downloadsPath)) {
                return downloadsPath.wstring();
            }
        }

        return L"C:/Users/Downloads";
    }

    static std::wstring determineCategory(const std::wstring& fileName) {
        const std::wstring lower = [&fileName]() {
            std::wstring tmp = fileName;
            std::transform(tmp.begin(), tmp.end(), tmp.begin(), [](wchar_t ch) {
                return static_cast<wchar_t>(std::towlower(ch));
            });
            return tmp;
        }();

        const std::vector<std::pair<std::wstring, std::vector<std::wstring>>> categories = {
            {L"image", {L".jpg", L".jpeg", L".png", L".gif", L".bmp", L".webp", L".svg"}},
            {L"video", {L".mp4", L".mov", L".avi", L".mkv", L".wmv", L".flv"}},
            {L"audio", {L".mp3", L".wav", L".aac", L".flac", L".m4a"}},
            {L"zip", {L".zip", L".rar", L".7z", L".tar", L".gz", L".tgz"}},
            {L"document", {L".pdf", L".doc", L".docx", L".xls", L".xlsx", L".ppt", L".pptx", L".txt"}},
            {L"code", {L".cpp", L".h", L".hpp", L".cs", L".py", L".js", L".ts", L".java", L".json", L".xml"}},
            {L"exe", {L".exe", L".msi", L".bat", L".cmd"}},
            {L"other", {}} 
        };

        for (const auto& [category, extensions] : categories) {
            if (extensions.empty()) {
                continue;
            }
            for (const auto& ext : extensions) {
                if (lower.size() >= ext.size() && lower.compare(lower.size() - ext.size(), ext.size(), ext) == 0) {
                    return category;
                }
            }
        }

        return L"other";
    }

    static std::wstring determineFolderCategory(const std::filesystem::path& folderPath) {
        std::vector<std::pair<std::wstring, std::size_t>> scores = {
            {L"image", 0},
            {L"video", 0},
            {L"audio", 0},
            {L"zip", 0},
            {L"document", 0},
            {L"code", 0},
            {L"exe", 0}
        };

        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(folderPath)) {
                if (!entry.is_regular_file()) {
                    continue;
                }

                const std::wstring category = determineCategory(entry.path().filename().wstring());
                for (auto& score : scores) {
                    if (score.first == category) {
                        ++score.second;
                        break;
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error&) {
            return L"other";
        }

        const auto best = std::max_element(
            scores.begin(),
            scores.end(),
            [](const auto& left, const auto& right) {
                return left.second < right.second;
            }
        );

        if (best != scores.end() && best->second > 0) {
            return best->first;
        }

        return L"other";
    }

    void trimRecentEvents() {
        if (recentEvents.size() > 20) {
            recentEvents.erase(recentEvents.begin(), recentEvents.begin() + (recentEvents.size() - 20));
        }
    }

    void moveDownloadEntry(const std::filesystem::path& entryPath, const std::wstring& prefix) {
        if (!std::filesystem::exists(entryPath)) {
            return;
        }

        const std::wstring name = entryPath.filename().wstring();
        if (name.empty() || name == L".") {
            return;
        }

        std::wstring category = L"other";
        if (std::filesystem::is_directory(entryPath)) {
            category = determineFolderCategory(entryPath);
        } else if (entryPath.has_extension()) {
            category = determineCategory(name);
        }

        std::filesystem::path targetDir = std::filesystem::path(getDesktopPath()) / category;
        std::filesystem::create_directories(targetDir);
        std::filesystem::path targetPath = targetDir / entryPath.filename();

        if (entryPath == targetPath) {
            return;
        }

        if (!std::filesystem::exists(targetPath)) {
            try {
                std::filesystem::rename(entryPath, targetPath);
                recentEvents.push_back(prefix + name + L" -> " + category);
                if (logCallback) {
                    logCallback("File organized: " + wideToUtf8(name) + " -> " + wideToUtf8(category));
                }
            } catch (const std::filesystem::filesystem_error&) {
                recentEvents.push_back(prefix + L"skip " + name);
                if (logCallback) {
                    logCallback("File organization skipped: " + wideToUtf8(name));
                }
            }
        }
        trimRecentEvents();
    }

    static std::set<std::wstring> collectSeenFiles(const std::wstring& dirPath) {
        std::set<std::wstring> seen;
        if (!std::filesystem::exists(dirPath)) {
            return seen;
        }

        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            seen.insert(entry.path().filename().wstring());
        }
        return seen;
    }

    void organizeByIdentifier(std::filesystem::path downloadDir) {
        if (!std::filesystem::exists(downloadDir)) {
            return;
        }

        std::set<std::wstring> seen = collectSeenFiles(downloadDir.wstring());
        for (const auto& entry : std::filesystem::directory_iterator(downloadDir)) {
            const std::wstring filename = entry.path().filename().wstring();
            if (filename.empty() || filename == L".") {
                continue;
            }

            const std::wstring category = determineCategory(filename);
            std::filesystem::path targetDir = std::filesystem::path(getDesktopPath()) / category;
            std::filesystem::create_directories(targetDir);
            std::filesystem::path targetPath = targetDir / entry.path().filename();
            if (!std::filesystem::exists(targetPath)) {
                std::filesystem::rename(entry.path(), targetPath);
                recentEvents.push_back(L"[scan] " + filename + L" -> " + category);
            }
        }

        for (const auto& name : seen) {
            std::wstring eventText = L"[seen] " + name;
            if (std::find(recentEvents.begin(), recentEvents.end(), eventText) == recentEvents.end()) {
                recentEvents.push_back(eventText);
            }
        }

        if (recentEvents.size() > 20) {
            recentEvents.erase(recentEvents.begin(), recentEvents.begin() + (recentEvents.size() - 20));
        }
    }

public:
    void setLogCallback(std::function<void(const std::string&)> callback) {
        logCallback = std::move(callback);
    }

    void organizeDownloadsNow() {
        const std::filesystem::path downloadsPath(getDownloadsPath());
        if (!std::filesystem::exists(downloadsPath)) {
            recentEvents.push_back(L"[downloads] ダウンロードフォルダが見つかりません");
            trimRecentEvents();
            return;
        }

        std::vector<std::filesystem::path> entries;
        for (const auto& entry : std::filesystem::directory_iterator(downloadsPath)) {
            entries.push_back(entry.path());
        }

        for (const auto& entryPath : entries) {
            moveDownloadEntry(entryPath, L"[downloads] ");
        }
    }

    void organizeDesktopNow() {
        const std::filesystem::path desktopPath(getDesktopPath());
        if (!std::filesystem::exists(desktopPath)) {
            recentEvents.push_back(L"[desktop] デスクトップが見つかりません");
            return;
        }

        std::vector<std::filesystem::path> foldersToMove;
        const std::set<std::wstring> skipFolderNames = {
            L"desktop_folders",
            L"image",
            L"video",
            L"audio",
            L"zip",
            L"document",
            L"code",
            L"exe",
            L"other"
        };

        for (const auto& entry : std::filesystem::directory_iterator(desktopPath)) {
            if (!entry.is_directory()) {
                continue;
            }

            const std::wstring folderName = entry.path().filename().wstring();
            if (folderName.empty()) {
                continue;
            }
            if (skipFolderNames.find(folderName) != skipFolderNames.end()) {
                continue;
            }

            foldersToMove.push_back(entry.path());
        }

        const std::filesystem::path folderRoot = desktopPath / L"desktop_folders";
        std::filesystem::create_directories(folderRoot);

        for (const auto& folderPath : foldersToMove) {
            const std::wstring folderName = folderPath.filename().wstring();
            const std::wstring contentCategory = determineFolderCategory(folderPath);
            const std::wstring category = L"folder_" + contentCategory;
            std::filesystem::path targetDir = folderRoot / category;
            std::filesystem::create_directories(targetDir);
            std::filesystem::path targetPath = targetDir / folderPath.filename();

            if (folderPath != targetPath && !std::filesystem::exists(targetPath)) {
                try {
                    std::filesystem::rename(folderPath, targetPath);
                    recentEvents.push_back(L"[desktop-folder] " + folderName + L" -> " + category);
                } catch (const std::filesystem::filesystem_error&) {
                    recentEvents.push_back(L"[desktop-folder] skip " + folderName);
                }
            }
        }
        trimRecentEvents();
    }

    void execute() override {
        startMonitoring();
    }

    void startMonitoring() {
        if (monitoring) {
            return;
        }

        monitoring = true;
        monitorThread = std::thread([this]() {
            const std::wstring downloadPath = getDownloadsPath();
            std::filesystem::path downloadsPath(downloadPath);
            std::set<std::wstring> lastKnownFiles;

            while (monitoring) {
                if (!std::filesystem::exists(downloadsPath)) {
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    continue;
                }

                std::set<std::wstring> currentFiles;
                for (const auto& entry : std::filesystem::directory_iterator(downloadsPath)) {
                    currentFiles.insert(entry.path().filename().wstring());
                }

                for (const auto& fileName : currentFiles) {
                    if (lastKnownFiles.find(fileName) == lastKnownFiles.end()) {
                        std::filesystem::path filePath = downloadsPath / fileName;
                        moveDownloadEntry(filePath, L"[auto] ");
                    }
                }

                lastKnownFiles = currentFiles;
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });
    }

    void stopMonitoring() {
        monitoring = false;
        if (monitorThread.joinable()) {
            monitorThread.join();
        }
    }

public:
    bool isMonitoring() const {
        return monitoring;
    }

    std::vector<std::wstring> getRecentEvents() const {
        return recentEvents;
    }

    void stopMonitoringThread() {
        stopMonitoring();
    }
};








