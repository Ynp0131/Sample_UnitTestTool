# 単体テスト結果レポート: TodoTool / LogTool (ToolModels.h)

> 生成日時: 2026-08-24 / 対象ブランチ・入力形式: GitHub `false1`ブランチ (`https://github.com/Ynp0131/Sample_UnitTestTool.git`)。修正はfeatureブランチ`fix/l58-readjsonstring`(`false1`から分岐)上で適用し、`main`へのPRを作成した。

## 1. テスト箇所

| 項目 | 内容 |
|---|---|
| 対象ファイル | `ToolModels.h` (`class TodoTool`, `class LogTool`) |
| 対象範囲 | `TodoTool`: `addTodo` / `removeTodo` / `updateTodo` / `getTodos` / `readJsonString` / `loadFromJson`。`LogTool`: `addLog` / `removeLog` / `getAllLogs` |
| 言語・フレームワーク | C++17 / GoogleTest (CMake `FetchContent`、MinGW-w64 g++ 16.1.0でビルド・実行) |
| `main`との差分 | `ToolModels.h:58`(`TodoTool::readJsonString`内、未知のエスケープ文字を検出した際の戻り値): `return false;` → `return true;`。同じロジックを持つ`MemoTool::readJsonString`(:283)・`LogTool::readJsonString`(:507)は変更されておらず`return false;`のまま |

## 2. テストケース

既存テストコード: あり(`tests/TodoTool_test.cpp`, `tests/LogTool_test.cpp`。`main`ブランチ側で導入済みの15件)
既存テストで確認済みのため今回追加しなかったケース: CRUD系(追加・削除・更新・取得)の正常系・境界値・異常系は既存15件でカバー済みのため対象外。今回の差分(`ToolModels.h:58`)はJSON読込(`loadFromJson`/`readJsonString`)側のロジックであり、**既存15件はいずれも`loadFromJson`の解析処理そのものを一度も実行していない**(下記「3. 網羅性チェック」参照)ため、そこに新規ケース(No.16)を1件追加した。

**LogTool**

| テストID | テスト観点 | 入力値 | 実行条件 | 期待結果 | 網羅対象コード | 結果 |
|----------|------------|--------|----------|----------|----------------|:---:|
| TC-001 | 正常系 | `addLog("message")` | 特になし | サイズ1、タイムスタンプ付きで本文を含む | `ToolModels.h:598 addLog` | ✅ |
| TC-002 | 異常系(Null/Empty分析) | `addLog("")` | 特になし | 追加されない(サイズ変化なし) | `ToolModels.h:598 addLog` | ✅ |
| TC-003 | 境界値 | 200件登録済みの状態でさらに1件`addLog` | 事前に200件登録 | サイズは200件のまま、最古の1件が削除される | `ToolModels.h:598 addLog` | ✅ |
| TC-004 | 正常系 | 有効なindexで`removeLog` | 2件登録済み | `true`、該当ログが削除される | `ToolModels.h:610 removeLog` | ✅ |
| TC-005 | 異常系(境界値) | 範囲外indexで`removeLog` | 1件登録済み | `false` | `ToolModels.h:610 removeLog` | ✅ |

**TodoTool**

| テストID | テスト観点 | 入力値 | 実行条件 | 期待結果 | 網羅対象コード | 結果 |
|----------|------------|--------|----------|----------|----------------|:---:|
| TC-006 | 正常系(同値分割) | 空の状態で`addTodo("牛乳を買う")` | 特になし | `getTodos(0)`が1件を返し`getPosition()`が1になる | `ToolModels.h:200 addTodo` | ✅ |
| TC-007 | 正常系(同値-複数key) | `addTodo`を2回呼ぶ | 特になし | key=0とkey=1にそれぞれ1件ずつ登録される | `ToolModels.h:200 addTodo` | ✅ |
| TC-008 | 境界値 | 空の状態で`removeTodo(0, 0)` | 特になし | `false` | `ToolModels.h:206 removeTodo` | ✅ |
| TC-009 | 異常系 | 存在するkeyだが範囲外indexで`removeTodo` | 1件登録済み | `false` | `ToolModels.h:206 removeTodo` | ✅ |
| TC-010 | 正常系 | 1件だけのkeyに対し`removeTodo` | 1件登録済み | `true`、かつキーごと削除され`getTodos`が`nullptr` | `ToolModels.h:206 removeTodo` | ✅ |
| TC-011 | 異常系(Null/Empty分析) | 空文字列で`updateTodo` | 1件登録済み | `false`、内容は変更されない | `ToolModels.h:220 updateTodo` | ✅ |
| TC-012 | 異常系 | 存在しないkeyで`updateTodo` | 特になし | `false` | `ToolModels.h:220 updateTodo` | ✅ |
| TC-013 | 異常系(境界値) | 範囲外indexで`updateTodo` | 1件登録済み | `false` | `ToolModels.h:220 updateTodo` | ✅ |
| TC-014 | 正常系 | 有効なkey/indexで`updateTodo` | 1件登録済み | `true`、内容が更新される | `ToolModels.h:220 updateTodo` | ✅ |
| TC-015 | 境界値 | 存在しないkeyで`getTodos` | 特になし | `nullptr` | `ToolModels.h:231 getTodos` | ✅ |
| **TC-016(新規)** | **異常系/例外経路分析** | `todos.json`に未知のエスケープ`\q`を含む`{"0": ["abc\qdef"]}`を事前配置 | 起動時読込(`TodoTool()`のコンストラクタ) | 読込を中断し、確定していない不完全な文字列を登録してはならない → `getTodos(0)`は`nullptr` | `ToolModels.h:58, 82 readJsonString / loadFromJson` | ❌ |

条件網羅分析: `updateTodo`/`removeTodo`の複合条件(`key未存在 \|\| index範囲外 \|\| (text空)`)は各分岐が真になるケース(TC-008/009/012/013/011)で個別に確認済み。
性能・並行性分析: マルチスレッド/非同期処理は無く、対象データ量も小規模のため該当なし。
セキュリティ分析: `TodoTool`/`LogTool`は外部入力(SQL/パストラバーサル相当の入力経路)を受け付けないため該当なし。
データ整合性分析: 排他制御は無いが、対象がシングルスレッドからのみ操作される前提のため該当なし(複数プロセスからの同時書き込みは今回のスコープ外)。

**実行結果サマリ(修正前・`false1`ブランチ):** 全16件中 15件成功 / 1件失敗

```
[==========] Running 16 tests from 2 test suites.
[----------] 5 tests from LogToolTest
[       OK ] LogToolTest.AddLogAppendsTimestampedMessage (128 ms)
[       OK ] LogToolTest.AddLogIgnoresEmptyMessage (25 ms)
[       OK ] LogToolTest.AddLogTrimsOldestEntryBeyondMaxOf200 (3816 ms)
[       OK ] LogToolTest.RemoveLogDeletesEntryAtValidIndex (94 ms)
[       OK ] LogToolTest.RemoveLogReturnsFalseWhenIndexOutOfRange (56 ms)
[----------] 11 tests from TodoToolTest
[       OK ] TodoToolTest.AddsFirstTodoToFreshKey (54 ms)
[       OK ] TodoToolTest.AddsSecondTodoUnderNewKey (75 ms)
[       OK ] TodoToolTest.RemoveTodoReturnsFalseWhenKeyMissing (20 ms)
[       OK ] TodoToolTest.RemoveTodoReturnsFalseWhenIndexOutOfRange (50 ms)
[       OK ] TodoToolTest.RemoveTodoErasesKeyWhenListBecomesEmpty (73 ms)
[       OK ] TodoToolTest.UpdateTodoReturnsFalseForEmptyText (53 ms)
[       OK ] TodoToolTest.UpdateTodoReturnsFalseWhenKeyMissing (22 ms)
[       OK ] TodoToolTest.UpdateTodoReturnsFalseWhenIndexOutOfRange (53 ms)
[       OK ] TodoToolTest.UpdateTodoAppliesNewTextWhenValid (68 ms)
[       OK ] TodoToolTest.GetTodosReturnsNullptrForMissingKey (16 ms)
[  FAILED  ] TodoToolTest.LoadFromJsonDiscardsPartialEntryWhenEscapeSequenceIsInvalid (52 ms)
[==========] 16 tests from 2 test suites ran. (4667 ms total)
[  PASSED  ] 15 tests.
[  FAILED  ] 1 test.
```

**実行結果サマリ(修正後・`fix/l58-readjsonstring`ブランチ):** 全16件中 **16件成功 / 0件失敗**

```
[==========] Running 16 tests from 2 test suites.
...
[       OK ] TodoToolTest.LoadFromJsonDiscardsPartialEntryWhenEscapeSequenceIsInvalid (44 ms)
[----------] 11 tests from TodoToolTest (444 ms total)
[==========] 16 tests from 2 test suites ran. (3740 ms total)
[  PASSED  ] 16 tests.
```

## 3. 網羅性チェック

| 網羅の種類 | 網羅率 | 備考 |
|---|:---:|---|
| ステートメント網羅率 | 約80%(設計時点の見立て) | `TodoTool`の`addTodo`/`removeTodo`/`updateTodo`/`getTodos`は網羅。`saveToJson`は各操作経由で実行されるが出力内容の直接検証は無い |
| ブランチ網羅率 | 約70%(設計時点の見立て) | `loadFromJson`/`readJsonString`について、**既存15件はいずれも`todos.json`が存在しない状態でしか`TodoTool`を生成しておらず、`loadFromJson`の解析コード(`ToolModels.h:82-149`)を一度も通過していなかった**。TC-016追加により「未知のエスケープで読込を中断する分岐」を初めて実行できたが、キーの区切り記号(`:`, `[`, `]`, `,`)欠落時の異常系や、`std::stoi`が失敗するケース(非数値キー)は依然未検証 |
| 条件網羅率 | 約85%(設計時点の見立て) | `updateTodo`/`removeTodo`の複合条件は各要素の真偽をTC-008/009/011/012/013で確認済み |
| 境界値網羅率 | 100%(設計時点の見立て) | `LogTool`の200件境界(内側/外側)、`TodoTool`のindex境界(範囲内/範囲外)を確認済み |
| 業務ルール網羅率 | 一部不足 | 要件定義書には`todos.json`のファイル形式異常時の挙動についての記載が無いため、`MemoTool`/`LogTool`側の実装(`return false;`で読込中断)を業務上の「暗黙の期待仕様」として採用した |

**不足している観点:**
- `loadFromJson`の構造異常系(先頭`{`が無い、キーの後の`:`が無い、配列の`[`/`]`が無い、キーが数値に変換できない等)は今回未着手。TC-016と同じ手法(一時ディレクトリに`todos.json`を事前配置)で同様に追加できるが、今回はブランチ`false1`固有の差分(l:58)の検証を優先したため対象外とした
- 実ファイルI/O異常(読込専用ファイル、ロック中のファイルなど)はOS依存のため単体テストの対象外とした

## 4. 最終判定

```
網羅済み:
- 正常系・異常系・境界値・同値分割・Null/Empty分析・条件網羅分析(TodoTool/LogToolのCRUD操作)
- 例外経路分析(readJsonStringの未知エスケープ検出パス。今回のTC-016で新規に実行された)

不足:
- loadFromJsonの構造異常系(区切り記号欠落、非数値キー)
- MemoToolへの同種テストの追加(スコープ外)

追加推奨ケース:
- todos.jsonの構造が壊れている場合(例: 先頭`{`欠落、`:`欠落)の異常系テスト
- addTodo→再生成(saveToJson→loadFromJsonの往復)で内容が保持されることを確認する正常系テスト(現状は保存はするが再読込での検証が無い)
```

## 5. 失敗ケースの詳細(エラー箇所・原因・改善案・影響範囲)

### ❌ TC-016: LoadFromJsonDiscardsPartialEntryWhenEscapeSequenceIsInvalid

**エラー箇所**
`tests/TodoTool_test.cpp:120`

```
C:\work\Sample\repo_false1\tests\TodoTool_test.cpp:120: Failure
Expected equality of these values:
  tool.getTodos(0)
    Which is: 0x1c8d7102430
  nullptr
    Which is: (nullptr)
```

**原因**
対象コードのバグ。`ToolModels.h:58`(`TodoTool::readJsonString`)で、JSON文字列中に未知のエスケープ文字(`\q`など、`"`/`\\`/`n`/`r`/`t`以外)が現れた場合の戻り値が`false`(解析失敗)から`true`(解析成功)に変更されている。

`return true;`はエスケープ文字を`value`へ追加せず、かつ閉じクォート`"`まで読み進めずに関数を抜けるため、以下の2つの問題が同時に起こる。
1. 直前まで組み立てた不完全な文字列(例では`"abc"`。本来あるべき`def`や閉じクォート以降の内容が失われている)を「解析成功」として呼び出し元に返してしまう
2. `index`がJSON文字列の途中(閉じクォートの手前)を指した状態で処理が継続するため、その後の`loadFromJson`(`ToolModels.h:82`)側のパースが位置ずれを起こし、後続の要素が正しく認識できなくなる

これは`readJsonString`を呼び出す`loadFromJson`(`ToolModels.h:132`: `if (!readJsonString(json, index, todo)) return;`)が「解析失敗時は即座に読込全体を中断する」設計になっていることと矛盾する。同じロジックを持つ`MemoTool::readJsonString`(`ToolModels.h:283`)・`LogTool::readJsonString`(`ToolModels.h:507`)はどちらも`default: return false;`のままであり、`TodoTool`だけがこの設計から外れている。

テストコード側のミスではなく、**対象コード(`ToolModels.h:58`)のバグ**と判定する。

**改善案**
`ToolModels.h:58`
```cpp
                    case 't': value += '\t'; break;
                    default: return false;   // true → false に戻す(MemoTool/LogToolと同じ挙動に統一)
```
なぜこの修正が必要か: `readJsonString`の呼び出し元は戻り値`false`を「文字列として解析できなかった」というシグナルとして扱い、その時点で`loadFromJson`全体を中断する設計になっている。`true`を返すと、未完成の値を「正常な結果」として上位に伝えてしまい、`todos.json`が万一(手動編集や将来的な破損などで)想定外のエスケープ文字を含んだ場合に、エラーにならず一部のTodoの内容が黒魔術的に欠落・破損した状態でアプリに読み込まれる。ユーザーには何の警告もなく表示され、原因の特定が難しくなる。

**適用結果**: `fix/l58-readjsonstring`ブランチ上で上記1行を適用し、`unit_tests.exe`を再実行した結果、TC-016を含む全16件が成功した(「実行結果サマリ(修正後)」参照)。

**影響範囲**

| 呼び出し元ファイル:行 | 現在のコード | 対応要否 | 呼び出し元側の修正案 |
|---|---|:---:|---|
| `AppController.h:20` | `TodoTool todoTool;`(起動時に一度だけ構築され、`todos.json`を読込む) | 不要 | `ToolModels.h:58`側の修正のみで解消。呼び出し元コードの変更は不要 |
| `AppController.cpp:64-73` | `createTodoItems`が`todoTool.getAllTodos()`をそのまま画面表示用リストに変換している | 不要 | 同上。`ToolModels.h:58`が修正されれば、破損データが生成されなくなるため画面表示側の対応は不要 |
| `AppController.cpp:120-123` | `case 1:`(TODO Managerボタン押下時)で`createTodoItems(todoTool)`を呼び出し、結果をそのままコールバック経由でView(`MainWindow`)へ渡す | 不要 | 同上 |

検索コマンド: `grep -rn "TodoTool" --include="*.cpp" --include="*.h" .`(取得できたリポジトリ全範囲を検索。GitHubから取得したソース以外の呼び出し元は無い)

現状の`todos.json`(リポジトリに含まれるもの)には未知のエスケープ文字は含まれておらず、通常のアプリ操作(`addTodo`/`updateTodo`は`escapeJsonString`で正しくエスケープしてから保存する)でも壊れたエスケープは発生しない。そのため実際に問題が表面化するのは、ユーザーが`todos.json`を手動編集する場合や、ファイルが外部要因で破損した場合に限られる。**発生頻度は低いが、発生時はユーザーへの通知が一切無く、データが静かに欠落する**点で修正の優先度は妥当である。

## 6. 修正の優先順位

検出されたバグは1件のみのため、優先順位付けは不要。`ToolModels.h:58`を`false`に戻す修正のみで解消する。

## 7. 総評

- `LogTool`および`TodoTool`のCRUD系ロジック(追加・削除・更新・取得)は、`main`ブランチから変更されておらず、15件すべて成功した。
- `false1`ブランチで加えられた唯一の変更(`ToolModels.h:58`: `readJsonString`のデフォルトケースを`false`→`true`)は、既存テストでは検出できない箇所だった。理由は、既存の`TodoToolTest`フィクスチャがいずれも「`todos.json`が存在しない状態」でしか`TodoTool`を生成しておらず、`loadFromJson`の解析ロジック自体が一度も実行されていなかったため。
- 今回追加したTC-016(未知のエスケープ文字を含む`todos.json`を事前配置して起動)により、この変更が「JSON解析失敗時に不完全な文字列を確定させてしまう」データ破損バグであることを実際に検出できた。
- 修正は`ToolModels.h:58`の1行を`return false;`に戻すだけで完了し、呼び出し元(`AppController`)への影響はない。あわせて、`loadFromJson`系のテストが今回まで実質0件だった点を踏まえ、構造異常系のテスト追加を今後の課題として推奨する。
