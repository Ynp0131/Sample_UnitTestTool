#pragma once
#include <cstdlib>
#include <string>
#include <windows.h>
#include <winhttp.h>

class LocalAiClient {
private:
    static std::string escapeJson(const std::string& text) {
        std::string result;
        for (char character : text) {
            switch (character) {
                case '\\': result += "\\\\"; break;
                case '"': result += "\\\""; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                default: result += character; break;
            }
        }
        return result;
    }

    static std::string unescapeJson(const std::string& text) {
        std::string result;
        bool escaped = false;
        for (char character : text) {
            if (escaped) {
                if (character == 'n') result += '\n';
                else if (character == 'r') result += '\r';
                else if (character == 't') result += '\t';
                else result += character;
                escaped = false;
            } else if (character == '\\') {
                escaped = true;
            } else {
                result += character;
            }
        }
        return result;
    }

    static bool post(const std::string& body, std::string& response, std::string& error) {
        HINTERNET session = WinHttpOpen(L"MVC-Mail-Generator/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, nullptr, nullptr, 0);
        if (!session) {
            error = "ローカルAI接続の準備に失敗しました。";
            return false;
        }
        HINTERNET connection = WinHttpConnect(session, L"127.0.0.1", 11434, 0);
        HINTERNET request = connection ? WinHttpOpenRequest(connection, L"POST", L"/api/generate", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0) : nullptr;
        const std::wstring headers = L"Content-Type: application/json; charset=utf-8";
        const BOOL sent = request && WinHttpSendRequest(request, headers.c_str(), static_cast<DWORD>(headers.size()), const_cast<char*>(body.data()), static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0);
        const BOOL received = sent && WinHttpReceiveResponse(request, nullptr);
        if (!received) {
            if (request) WinHttpCloseHandle(request);
            if (connection) WinHttpCloseHandle(connection);
            WinHttpCloseHandle(session);
            error = "ローカルAIに接続できません。Ollamaを起動してください。";
            return false;
        }
        DWORD available = 0;
        while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
            std::string chunk(available, '\0');
            DWORD read = 0;
            if (!WinHttpReadData(request, chunk.data(), available, &read)) {
                error = "ローカルAIの応答読み込みに失敗しました。";
                break;
            }
            response.append(chunk.data(), read);
        }
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return !response.empty() && error.empty();
    }

public:
    static bool generate(const std::string& input, bool daily, std::string& output, std::string& error) {
        const char* configuredModel = std::getenv("OLLAMA_MODEL");
        const std::string model = configuredModel && *configuredModel ? configuredModel : "qwen2.5:3b";
        const std::string mode = daily
            ? "日報として、件名を日報にし、本日の実施内容、課題・懸念、明日の予定を自然な段落で整理してください。"
            : "上司へ送る報告メールとして、内容の意味を整理し、適切な件名と自然な段落の本文を作成してください。";
        const std::string prompt = "あなたは社内メールの編集担当です。入力をそのままコピペせず、誰が何をしたか、予定、問題を整理して文章化してください。事実にない内容や推測を追加しないでください。箇条書きは禁止です。" + mode + "出力はメール本文だけにしてください。\n\n入力:\n" + input;
        const std::string body = "{\"model\":\"" + escapeJson(model) + "\",\"prompt\":\"" + escapeJson(prompt) + "\",\"stream\":false}";
        std::string response;
        if (!post(body, response, error)) return false;
        const std::string key = "\"response\":\"";
        const std::size_t begin = response.find(key);
        if (begin == std::string::npos) {
            error = "ローカルAIの応答に本文がありません。モデル名を確認してください。";
            return false;
        }
        const std::size_t start = begin + key.size();
        std::size_t end = start;
        bool escaped = false;
        while (end < response.size()) {
            if (response[end] == '"' && !escaped) break;
            if (response[end] == '\\' && !escaped) escaped = true;
            else escaped = false;
            ++end;
        }
        output = unescapeJson(response.substr(start, end - start));
        if (output.empty()) {
            error = "ローカルAIが空の本文を返しました。";
            return false;
        }
        return true;
    }
};

