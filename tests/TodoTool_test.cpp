#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "ToolModels.h"

namespace fs = std::filesystem;

// todos.json はカレントディレクトリ固定でハードコードされているため、
// 実際のプロジェクトファイルを汚さないよう一時ディレクトリで実行する。
class TodoToolTest : public ::testing::Test {
protected:
    fs::path originalDir;
    fs::path tempDir;

    void SetUp() override {
        originalDir = fs::current_path();
        tempDir = fs::temp_directory_path() / fs::path("todo_tool_test_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()));
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
TEST_F(TodoToolTest, AddsFirstTodoToFreshKey) {
    TodoTool tool;
    tool.addTodo("\xe7\x89\x9b\xe4\xb9\xb3\xe3\x82\x92\xe8\xb2\xb7\xe3\x81\x86"); // "牛乳を買う"

    const auto* todos = tool.getTodos(0);
    ASSERT_NE(todos, nullptr);
    ASSERT_EQ(todos->size(), 1u);
    EXPECT_EQ(tool.getPosition(), 1);
}

// Case No.2
TEST_F(TodoToolTest, AddsSecondTodoUnderNewKey) {
    TodoTool tool;
    tool.addTodo("first");
    tool.addTodo("second");

    ASSERT_NE(tool.getTodos(0), nullptr);
    ASSERT_NE(tool.getTodos(1), nullptr);
    EXPECT_EQ(tool.getTodos(0)->at(0), "first");
    EXPECT_EQ(tool.getTodos(1)->at(0), "second");
}

// Case No.3
TEST_F(TodoToolTest, RemoveTodoReturnsFalseWhenKeyMissing) {
    TodoTool tool;
    EXPECT_FALSE(tool.removeTodo(0, 0));
}

// Case No.4
TEST_F(TodoToolTest, RemoveTodoReturnsFalseWhenIndexOutOfRange) {
    TodoTool tool;
    tool.addTodo("only item");
    EXPECT_FALSE(tool.removeTodo(0, 5));
}

// Case No.5
TEST_F(TodoToolTest, RemoveTodoErasesKeyWhenListBecomesEmpty) {
    TodoTool tool;
    tool.addTodo("only item");
    EXPECT_TRUE(tool.removeTodo(0, 0));
    EXPECT_EQ(tool.getTodos(0), nullptr);
}

// Case No.6
TEST_F(TodoToolTest, UpdateTodoReturnsFalseForEmptyText) {
    TodoTool tool;
    tool.addTodo("original");
    EXPECT_FALSE(tool.updateTodo(0, 0, ""));
    EXPECT_EQ(tool.getTodos(0)->at(0), "original");
}

// Case No.7
TEST_F(TodoToolTest, UpdateTodoReturnsFalseWhenKeyMissing) {
    TodoTool tool;
    EXPECT_FALSE(tool.updateTodo(0, 0, "new text"));
}

// Case No.8
TEST_F(TodoToolTest, UpdateTodoReturnsFalseWhenIndexOutOfRange) {
    TodoTool tool;
    tool.addTodo("original");
    EXPECT_FALSE(tool.updateTodo(0, 3, "new text"));
}

// Case No.9
TEST_F(TodoToolTest, UpdateTodoAppliesNewTextWhenValid) {
    TodoTool tool;
    tool.addTodo("original");
    EXPECT_TRUE(tool.updateTodo(0, 0, "updated"));
    EXPECT_EQ(tool.getTodos(0)->at(0), "updated");
}

// Case No.10
TEST_F(TodoToolTest, GetTodosReturnsNullptrForMissingKey) {
    TodoTool tool;
    EXPECT_EQ(tool.getTodos(42), nullptr);
}

// Case No.11 (異常系/例外経路分析: todos.json内に未知のエスケープシーケンスがある場合)
// MemoTool・LogToolの同等ロジック(ToolModels.h:283, :507)は未知のエスケープで
// 読込を中断する(false)。TodoTool側もその方針に揃えるべきであり、
// 途中まで組み立てた不完全な文字列("abc")を確定させてはならない。
TEST_F(TodoToolTest, LoadFromJsonDiscardsPartialEntryWhenEscapeSequenceIsInvalid) {
    {
        std::ofstream file("todos.json", std::ios::binary | std::ios::trunc);
        file << "{\n  \"0\": [\"abc\\qdef\"]\n}\n";
    }

    TodoTool tool;

    EXPECT_EQ(tool.getTodos(0), nullptr);
}
