#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "ToolModels.h"

namespace fs = std::filesystem;

// logs.json はカレントディレクトリ固定でハードコードされているため、
// 実際のプロジェクトファイルを汚さないよう一時ディレクトリで実行する。
class LogToolTest : public ::testing::Test {
protected:
    fs::path originalDir;
    fs::path tempDir;

    void SetUp() override {
        originalDir = fs::current_path();
        tempDir = fs::temp_directory_path() / fs::path("log_tool_test_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()));
        fs::create_directories(tempDir);
        fs::current_path(tempDir);
    }

    void TearDown() override {
        fs::current_path(originalDir);
        std::error_code ec;
        fs::remove_all(tempDir, ec);
    }
};

namespace {
bool endsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() &&
        text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}
}

// TC-L01
TEST_F(LogToolTest, AddLogAppendsTimestampedMessage) {
    LogTool tool;
    tool.addLog("message");

    ASSERT_EQ(tool.getAllLogs().size(), 1u);
    EXPECT_NE(tool.getAllLogs()[0].find("message"), std::string::npos);
    EXPECT_EQ(tool.getAllLogs()[0].front(), '[');
}

// TC-L02
TEST_F(LogToolTest, AddLogIgnoresEmptyMessage) {
    LogTool tool;
    tool.addLog("");
    EXPECT_EQ(tool.getAllLogs().size(), 0u);
}

// TC-L03
TEST_F(LogToolTest, AddLogTrimsOldestEntryBeyondMaxOf200) {
    LogTool tool;
    for (int i = 0; i < 200; ++i) {
        tool.addLog("entry " + std::to_string(i));
    }
    ASSERT_EQ(tool.getAllLogs().size(), 200u);
    EXPECT_TRUE(endsWith(tool.getAllLogs().front(), "entry 0"));

    tool.addLog("entry 200");

    ASSERT_EQ(tool.getAllLogs().size(), 200u);
    EXPECT_FALSE(endsWith(tool.getAllLogs().front(), "entry 0"));
    EXPECT_TRUE(endsWith(tool.getAllLogs().front(), "entry 1"));
    EXPECT_TRUE(endsWith(tool.getAllLogs().back(), "entry 200"));
}

// TC-L04
TEST_F(LogToolTest, AddLogKeepsAllEntriesAtExactlyMaxOf200) {
    LogTool tool;
    for (int i = 0; i < 200; ++i) {
        tool.addLog("entry " + std::to_string(i));
    }

    ASSERT_EQ(tool.getAllLogs().size(), 200u);
    EXPECT_TRUE(endsWith(tool.getAllLogs().front(), "entry 0"));
    EXPECT_TRUE(endsWith(tool.getAllLogs().back(), "entry 199"));
}

// TC-L05
TEST_F(LogToolTest, RemoveLogSucceedsAtLastValidIndex) {
    LogTool tool;
    tool.addLog("only entry");
    EXPECT_TRUE(tool.removeLog(0));
}

// TC-L06
TEST_F(LogToolTest, RemoveLogFailsJustPastLastValidIndex) {
    LogTool tool;
    tool.addLog("only entry");
    EXPECT_FALSE(tool.removeLog(1));
}

// TC-L07
TEST_F(LogToolTest, RoundTripsSpecialCharactersThroughJsonPersistence) {
    const std::string specialText = "quote\" backslash\\ newline\n tab\t";
    {
        LogTool tool;
        tool.addLog(specialText);
    }

    // 別インスタンスで再構築し、logs.jsonからの再読込を強制する
    LogTool reloaded;
    ASSERT_EQ(reloaded.getAllLogs().size(), 1u);
    EXPECT_NE(reloaded.getAllLogs()[0].find(specialText), std::string::npos);
}

// TC-L08
TEST_F(LogToolTest, RejectsFileWithUnrecognizedEscapeSequence) {
    // \z はLogTool内部のreadJsonStringが認識しないエスケープ文字。
    {
        std::ofstream file("logs.json", std::ios::binary | std::ios::trunc);
        file << "[\"ab\\zcd\"]";
    }

    LogTool tool;
    EXPECT_EQ(tool.getAllLogs().size(), 0u);
}
