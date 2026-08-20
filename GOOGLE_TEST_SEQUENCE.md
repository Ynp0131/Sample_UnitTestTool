# Googleテスト用シーケンス

## 1. 起動と監視開始

```mermaid
sequenceDiagram
    actor User as ユーザー
    participant Main as main.cpp
    participant View as MainWindow
    participant Controller as AppController
    participant File as FileTool
    participant Mail as MailContentTool
    participant Log as LogTool

    User->>Main: mvc_app.exeを起動
    Main->>Controller: AppControllerを生成
    Controller->>Log: logs.jsonを読込
    Controller->>File: ファイル監視を開始
    Controller->>Mail: input監視を開始
    Controller->>Log: 監視開始を記録
    Main->>View: メインウィンドウを作成
    View->>View: 監視小窓を非表示で作成
```

## 2. Ctrl+Alt+Sで小窓を表示

```mermaid
sequenceDiagram
    actor User as ユーザー
    participant OS as Windows Message Loop
    participant View as MainWindow

    User->>OS: Ctrl+Alt+Sを入力
    OS->>View: WM_HOTKEY
    View->>View: 監視小窓の表示状態を切替
    View-->>User: 小窓を表示または非表示
```

## 3. Todoの追加・編集・削除

```mermaid
sequenceDiagram
    actor User as ユーザー
    participant View as MainWindow
    participant Controller as AppController
    participant Todo as TodoTool
    participant Log as LogTool

    User->>View: Todoを入力して追加/編集/削除
    View->>Controller: addTodo / editTodo / deleteTodo
    Controller->>Todo: データを変更
    Todo->>Todo: todos.jsonへ保存
    Controller->>Log: 操作結果を追加
    Log->>Log: logs.jsonへ保存
    Log->>Log: 200件超なら最古ログを削除
    Controller-->>View: 更新済みTodo一覧
    View-->>User: 一覧を更新
```

## 4. Memoの登録・選択・更新・削除

```mermaid
sequenceDiagram
    actor User as ユーザー
    participant View as MainWindow
    participant Controller as AppController
    participant Memo as MemoTool
    participant Log as LogTool

    User->>View: タイトルと本文を入力
    View->>Controller: addMemo
    Controller->>Memo: MemoEntryを追加
    Memo->>Memo: memos.jsonへ保存
    Controller->>Log: Memo追加を記録
    User->>View: Memoタイトルを選択
    View->>Controller: getMemo
    Controller->>Memo: タイトルと本文を取得
    Memo-->>View: MemoEntry
    View-->>User: 本文を表示
    User->>View: Memoを更新または削除
    View->>Controller: updateMemo / deleteMemo
    Controller->>Memo: データを変更
    Controller->>Log: 操作結果を記録
```

## 5. Downloads自動監視・整理

```mermaid
sequenceDiagram
    participant FileSystem as Downloadsフォルダ
    participant File as FileTool
    participant Desktop as Desktop分類フォルダ
    participant Log as LogTool

    loop 約2秒ごと
        File->>FileSystem: ファイル・フォルダ一覧を確認
        FileSystem-->>File: 現在の一覧
        File->>File: 前回一覧との差分を判定
    end
    File->>FileSystem: 新規項目を検出
    File->>File: 拡張子またはフォルダ内容から分類
    File->>Desktop: 分類フォルダへ移動
    File->>Log: 整理成功/失敗を通知
    Log->>Log: logs.jsonへ保存
```

## 6. Desktop手動整理

```mermaid
sequenceDiagram
    actor User as ユーザー
    participant View as MainWindow
    participant Controller as AppController
    participant File as FileTool
    participant Desktop as Desktop
    participant Log as LogTool

    User->>View: Desktop整理ボタンを押す
    View->>Controller: organizeDesktopFiles
    Controller->>File: organizeDesktopNow
    File->>Desktop: フォルダをdesktop_foldersへ整理
    Controller->>Log: Desktop整理を記録
    Controller-->>View: 整理結果
    View-->>User: 結果を表示
```

## 7. 通常メール生成

```mermaid
sequenceDiagram
    actor User as ユーザー
    participant View as MainWindow
    participant Controller as AppController
    participant Mail as MailContentTool
    participant Input as input/input_*.txt
    participant AI as Ollama localhost
    participant Output as output/generated_mail.txt
    participant Log as LogTool

    User->>View: メール内容を入力
    View->>Controller: generateMailContent
    Controller->>Mail: 入力を渡す
    Mail->>Input: UTF-8で保存
    Mail->>AI: localhost:11434へ生成依頼
    AI-->>Mail: 件名と自然な本文
    Mail->>Output: 最新結果を保存
    Controller->>Log: メール生成成功を記録
    Controller-->>View: 保存先と本文
    View-->>User: 本文を表示

    alt Ollama未起動または応答失敗
        AI-->>Mail: 接続エラー
        Mail->>Output: 日本語エラーを保存
        Controller->>Log: メール生成失敗を記録
        Controller-->>View: エラーを表示
    end
```

## 8. 日報生成

```mermaid
sequenceDiagram
    actor User as ユーザー
    participant Mail as MailContentTool
    participant Input as input/input_*.txt
    participant AI as Ollama localhost
    participant Output as output/generated_mail.txt
    participant Log as LogTool

    User->>Input: 先頭に種別:日報を含む内容を保存
    Mail->>Input: 最新ファイルを検出
    Mail->>Mail: 日報モードを判定
    Mail->>AI: 日報生成を依頼
    AI-->>Mail: 実施内容・課題・明日の予定を自然文で返却
    Mail->>Output: 日報を保存
    Mail->>Log: 日報生成成功を記録
```

## 9. ログバッファ

```mermaid
sequenceDiagram
    participant Feature as 各機能
    participant Log as LogTool
    participant File as logs.json

    Feature->>Log: 操作結果を通知
    Log->>Log: タイムスタンプを付加
    Log->>Log: ログを末尾へ追加
    alt 200件を超えた
        Log->>Log: 先頭の古いログを削除
    end
    Log->>File: JSON配列として保存
```

## 10. Googleテスト実施順

1. アプリをビルドする。
2. `logs.json` をバックアップする。
3. アプリを起動し、監視小窓が自動表示されないことを確認する。
4. `Ctrl+Alt+S` で小窓が切り替わることを確認する。
5. Todo、Memo、Logの追加・編集・削除を確認する。
6. Downloadsにテスト用ファイルとフォルダを追加し、自動整理を確認する。
7. Desktop整理をボタンから実行する。
8. 通常メールを生成する。
9. `種別:日報` を含む入力で日報を生成する。
10. Ollama停止時にエラー処理とログ記録を確認する。
11. `logs.json` が200件を超えないことを確認する。
12. テスト終了後、テスト用の入力・出力ファイルを手動で削除する。
