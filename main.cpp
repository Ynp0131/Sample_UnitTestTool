#include <Windows.h>

#include "AppController.h"
#include "MainWindow.h"

// アプリケーションの起動処理だけを担当するエントリーポイント。
// 具体的な画面処理はView、ボタンの処理分岐はController、データはModelに置く。
int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int commandShow) {
    AppController controller;
    MainWindow window(instance, controller);

    if (!window.create()) {
        return 0;
    }

    window.show(commandShow);

    MSG message = {};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}
