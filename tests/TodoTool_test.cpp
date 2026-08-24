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

// TC-T01
TEST_F(TodoToolTest, AddsFirstTodoToFreshKey) {
    TodoTool tool;
    tool.addTodo("\xe7\x89\x9b\xe4\xb9\xb3\xe3\x82\x92\xe8\xb2\xb7\xe3\x81\x86"); // "牛乳を買う"

    const auto* todos = tool.getTodos(0);
    ASSERT_NE(todos, nullptr);
    ASSERT_EQ(todos->size(), 1u);
    EXPECT_EQ(tool.getPosition(), 1);
}

// TC-T02
TEST_F(TodoToolTest, AddsSecondTodoUnderNewKey) {
    TodoTool tool;
    tool.addTodo("first");
    tool.addTodo("second");

    ASSERT_NE(tool.getTodos(0), nullptr);
    ASSERT_NE(tool.getTodos(1), nullptr);
    EXPECT_EQ(tool.getTodos(0)->at(0), "first");
    EXPECT_EQ(tool.getTodos(1)->at(0), "second");
}

// TC-T03
TEST_F(TodoToolTest, AddTodoAllowsEmptyString) {
    TodoTool tool;
    tool.addTodo("");

    ASSERT_NE(tool.getTodos(0), nullptr);
    ASSERT_EQ(tool.getTodos(0)->size(), 1u);
    EXPECT_EQ(tool.getTodos(0)->at(0), "");
}

// TC-T04
TEST_F(TodoToolTest, RemoveTodoSucceedsAtLastValidIndex) {
    TodoTool tool;
    tool.addTodo("only item");
    EXPECT_TRUE(tool.removeTodo(0, 0));
}

// TC-T05
TEST_F(TodoToolTest, RemoveTodoFailsJustPastLastValidIndex) {
    TodoTool tool;
    tool.addTodo("only item");
    EXPECT_FALSE(tool.removeTodo(0, 1));
}

// TC-T06
TEST_F(TodoToolTest, RemoveTodoReturnsFalseWhenKeyMissing) {
    TodoTool tool;
    EXPECT_FALSE(tool.removeTodo(0, 0));
}

// TC-T07
TEST_F(TodoToolTest, RemoveTodoErasesKeyWhenListBecomesEmpty) {
    TodoTool tool;
    tool.addTodo("only item");
    EXPECT_TRUE(tool.removeTodo(0, 0));
    EXPECT_EQ(tool.getTodos(0), nullptr);
}

// TC-T08
TEST_F(TodoToolTest, UpdateTodoReturnsFalseForEmptyText) {
    TodoTool tool;
    tool.addTodo("original");
    EXPECT_FALSE(tool.updateTodo(0, 0, ""));
    EXPECT_EQ(tool.getTodos(0)->at(0), "original");
}

// TC-T09
TEST_F(TodoToolTest, UpdateTodoReturnsFalseWhenKeyMissing) {
    TodoTool tool;
    EXPECT_FALSE(tool.updateTodo(0, 0, "new text"));
}

// TC-T10
TEST_F(TodoToolTest, UpdateTodoFailsJustPastLastValidIndex) {
    TodoTool tool;
    tool.addTodo("original");
    EXPECT_FALSE(tool.updateTodo(0, 1, "new text"));
}

// TC-T11
TEST_F(TodoToolTest, UpdateTodoAppliesNewTextWhenValid) {
    TodoTool tool;
    tool.addTodo("original");
    EXPECT_TRUE(tool.updateTodo(0, 0, "updated"));
    EXPECT_EQ(tool.getTodos(0)->at(0), "updated");
}

// TC-T12
TEST_F(TodoToolTest, GetTodosReturnsNullptrForMissingKey) {
    TodoTool tool;
    EXPECT_EQ(tool.getTodos(42), nullptr);
}

// TC-T13
TEST_F(TodoToolTest, PositionIsNotReusedAfterRemoval) {
    TodoTool tool;
    tool.addTodo("first");           // key 0
    EXPECT_TRUE(tool.removeTodo(0, 0));
    tool.addTodo("second");          // 削除後も position は増え続けるはず

    EXPECT_EQ(tool.getTodos(0), nullptr);
    ASSERT_NE(tool.getTodos(1), nullptr);
    EXPECT_EQ(tool.getTodos(1)->at(0), "second");
}

// TC-T14
TEST_F(TodoToolTest, RoundTripsSpecialCharactersThroughJsonPersistence) {
    const std::string specialText = "quote\" backslash\\ newline\n tab\t";
    {
        TodoTool tool;
        tool.addTodo(specialText);
    }

    // 別インスタンスで再構築し、todos.jsonからの再読込を強制する
    TodoTool reloaded;
    ASSERT_NE(reloaded.getTodos(0), nullptr);
    ASSERT_EQ(reloaded.getTodos(0)->size(), 1u);
    EXPECT_EQ(reloaded.getTodos(0)->at(0), specialText);
}

// TC-T15
TEST_F(TodoToolTest, RejectsFileWithUnrecognizedEscapeSequence) {
    // \z はTodoTool内部のreadJsonStringが認識しないエスケープ文字。
    // このようなtodos.jsonを読み込んだ場合、パース失敗として扱われ、
    // 該当エントリは読み込まれないのが仕様(ToolModels.h:58 readJsonStringのdefault節)。
    {
        std::ofstream file("todos.json", std::ios::binary | std::ios::trunc);
        file << "{\"0\": [\"ab\\zcd\"]}";
    }

    TodoTool tool;
    EXPECT_EQ(tool.getTodos(0), nullptr);
}
