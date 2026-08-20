#include <gtest/gtest.h>
#include <filesystem>
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

// Case No.1
TEST_F(LogToolTest, AddLogAppendsTimestampedMessage) {
    LogTool tool;
    tool.addLog("message");

    ASSERT_EQ(tool.getAllLogs().size(), 1u);
    EXPECT_NE(tool.getAllLogs()[0].find("message"), std::string::npos);
    EXPECT_EQ(tool.getAllLogs()[0].front(), '[');
}

// Case No.2
TEST_F(LogToolTest, AddLogIgnoresEmptyMessage) {
    LogTool tool;
    tool.addLog("");
    EXPECT_EQ(tool.getAllLogs().size(), 0u);
}

namespace {
bool endsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() &&
        text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}
}

// Case No.3
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

// Case No.4
TEST_F(LogToolTest, RemoveLogDeletesEntryAtValidIndex) {
    LogTool tool;
    tool.addLog("first");
    tool.addLog("second");

    EXPECT_TRUE(tool.removeLog(0));
    ASSERT_EQ(tool.getAllLogs().size(), 1u);
    EXPECT_NE(tool.getAllLogs()[0].find("second"), std::string::npos);
}

// Case No.5
TEST_F(LogToolTest, RemoveLogReturnsFalseWhenIndexOutOfRange) {
    LogTool tool;
    tool.addLog("only entry");
    EXPECT_FALSE(tool.removeLog(5));
}
