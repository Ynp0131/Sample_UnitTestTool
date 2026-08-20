# 単体テスト結果レポート: TodoTool / LogTool (ToolModels.h)

> 生成日時: 2026-08-20 / 対象ブランチ・入力形式: GitHub `main`ブランチ (`https://github.com/Ynp0131/Sample_UnitTestTool.git`)

## 1. テスト箇所

| 項目 | 内容 |
|---|---|
| 対象ファイル | `ToolModels.h` (`class TodoTool`, `class LogTool`) |
| 対象範囲 | `TodoTool`: `addTodo` / `removeTodo` / `updateTodo` / `getTodos`。`LogTool`: `addLog` / `removeLog` / `getAllLogs` |
| 言語・フレームワーク | C++17 / GoogleTest (新規導入) |
| 選定理由 | プロジェクトにはまだ自動テストが1件も存在せず(`GOOGLE_TEST_SEQUENCE.md`は手動テスト手順書のみ)、`AppController`が利用する5つのModelクラスのうち、GUI・ネットワーク(Ollama)・ファイル監視スレッドに依存しない`TodoTool`と`LogTool`を、境界値が明確なロジックとして最初の対象に選定した |

## 2. テストケース

**TodoTool**

| No | 観点 | 入力 | 期待される出力/挙動 | 結果 |
|----|------|------|----------------------|:---:|
| 1 | 正常系 | 空の状態で`addTodo("牛乳を買う")` | `getTodos(0)`が1件を返し`getPosition()`が1になる | ✅ |
| 2 | 正常系(同値-複数key) | `addTodo`を2回呼ぶ | key=0とkey=1にそれぞれ1件ずつ登録される | ✅ |
| 3 | 境界値 | 空の状態で`removeTodo(0, 0)` | `false` | ✅ |
| 4 | 異常系 | 存在するkeyだが範囲外indexで`removeTodo` | `false` | ✅ |
| 5 | 正常系 | 1件だけのkeyに対し`removeTodo` | `true`、かつキーごと削除され`getTodos`が`nullptr` | ✅ |
| 6 | 異常系 | 空文字列で`updateTodo` | `false`、内容は変更されない | ✅ |
| 7 | 異常系 | 存在しないkeyで`updateTodo` | `false` | ✅ |
| 8 | 異常系 | 範囲外indexで`updateTodo` | `false` | ✅ |
| 9 | 正常系 | 有効なkey/indexで`updateTodo` | `true`、内容が更新される | ✅ |
| 10 | 境界値 | 存在しないkeyで`getTodos` | `nullptr` | ✅ |

**LogTool**

| No | 観点 | 入力 | 期待される出力/挙動 | 結果 |
|----|------|------|----------------------|:---:|
| 1 | 正常系 | `addLog("message")` | サイズ1、タイムスタンプ付きで本文を含む | ✅ |
| 2 | 異常系 | `addLog("")` | 追加されない(サイズ変化なし) | ✅ |
| 3 | 境界値 | 200件登録済みの状態でさらに1件`addLog` | サイズは200件のまま、最古の1件が削除される | ✅ |
| 4 | 正常系 | 有効なindexで`removeLog` | `true`、該当ログが削除される | ✅ |
| 5 | 異常系 | 範囲外indexで`removeLog` | `false` | ✅ |

**実行結果サマリ:** 全15件中 15件成功 / 0件失敗

```
[==========] Running 15 tests from 2 test suites.
[----------] 5 tests from LogToolTest
[       OK ] LogToolTest.AddLogAppendsTimestampedMessage (44 ms)
[       OK ] LogToolTest.AddLogIgnoresEmptyMessage (11 ms)
[       OK ] LogToolTest.AddLogTrimsOldestEntryBeyondMaxOf200 (2406 ms)
[       OK ] LogToolTest.RemoveLogDeletesEntryAtValidIndex (55 ms)
[       OK ] LogToolTest.RemoveLogReturnsFalseWhenIndexOutOfRange (30 ms)
[----------] 10 tests from TodoToolTest
[       OK ] TodoToolTest.AddsFirstTodoToFreshKey (26 ms)
[       OK ] TodoToolTest.AddsSecondTodoUnderNewKey (37 ms)
[       OK ] TodoToolTest.RemoveTodoReturnsFalseWhenKeyMissing (9 ms)
[       OK ] TodoToolTest.RemoveTodoReturnsFalseWhenIndexOutOfRange (25 ms)
[       OK ] TodoToolTest.RemoveTodoErasesKeyWhenListBecomesEmpty (37 ms)
[       OK ] TodoToolTest.UpdateTodoReturnsFalseForEmptyText (27 ms)
[       OK ] TodoToolTest.UpdateTodoReturnsFalseWhenKeyMissing (6 ms)
[       OK ] TodoToolTest.UpdateTodoReturnsFalseWhenIndexOutOfRange (26 ms)
[       OK ] TodoToolTest.UpdateTodoAppliesNewTextWhenValid (40 ms)
[       OK ] TodoToolTest.GetTodosReturnsNullptrForMissingKey (7 ms)
[==========] 15 tests from 2 test suites ran. (2806 ms total)
[  PASSED  ] 15 tests.
```

## 3. 失敗ケースの詳細

失敗ケースなし。既存ロジックはテーブルの15ケースすべてで設計どおりに動作していた(バグは検出されなかった)。

## 4. 修正の優先順位

失敗ケースが無いため優先順位付けは不要。ただし下記「5. 総評」の改善提案は、依存関係なく個別に対応可能。

## 5. 総評

- **バグは見つからなかった**: `TodoTool`・`LogTool`とも、追加/削除/更新の正常系・境界値・異常系のいずれも仕様どおりに動作していた。特に`GOOGLE_TEST_SEQUENCE.md`の手動確認項目11「`logs.json`が200件を超えない」は、今回`LogToolTest.AddLogTrimsOldestEntryBeyondMaxOf200`として自動化できた。
- **設計上の弱点(テスト容易性)**: `ToolModels.h:30`と`ToolModels.h:464`で`fileName`(`"todos.json"` / `"logs.json"`)がハードコードされ、コンストラクタで自動読込・各操作で自動保存される。このため単体テストは実行ディレクトリを一時フォルダに切り替えるフィクスチャで実データを回避する必要があった。コンストラクタでファイルパスを注入できるようにする(例: `TodoTool(std::string fileName = "todos.json")`)と、実データに触れず、かつテストごとに独立したテストダブルを使えるようになる。
  - 改善案(`ToolModels.h:30`, `ToolModels.h:183-186`):
    ```cpp
    class TodoTool : public Tool {
    private:
        std::string fileName; // ハードコード文字列から変更
    public:
        explicit TodoTool(std::string path = "todos.json") : fileName(std::move(path)) {
            loadFromJson();
        }
        ...
    ```
    なぜこの修正が必要か: テストが実ファイルを書き換える副作用を持たなくなり、`SetUp`/`TearDown`でのディレクトリ切替(カレントディレクトリの変更)というテスト側の回避策が不要になるため。
- **カバレッジが薄い部分**: 今回は`AppController`が利用する5つのModelクラスのうち`TodoTool`・`LogTool`のみを対象にした。以下は未着手であり、次のテスト対象の候補:
  - `MemoTool`(`ToolModels.h:246`〜): `TodoTool`と同様のCRUDロジックで、同じ観点のテストケースがそのまま適用できる
  - `LogTool`/`TodoTool`いずれも`loadFromJson`のJSON手書きパーサ自体は今回未検証(不正なJSON・壊れたファイルを読んだ場合の挙動)
  - `MailContentTool`・`FileTool`(`ToolModels.h:624`〜, `1043`〜): Ollamaへのネットワーク呼び出しやファイル監視スレッドを含むため、モック化(`GoogleMock`でインターフェース越しに`IService`相当を切り出す)が必要
  - `AppController.cpp`: 上記Modelクラスを束ねるコントローラ層。Modelのテストが揃った後にコールバック経由の結合的な挙動を確認する価値がある
- **結論**: 今回対象とした範囲に限れば、修正が必要なバグは無い。次の一手は、`MemoTool`への同様のテスト追加と、ファイルパスのハードコード解消(テスト容易性の改善)。
