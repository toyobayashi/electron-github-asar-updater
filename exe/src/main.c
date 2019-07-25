#ifdef _WIN32
#include <Windows.h>
#include <util.h>
#include <stdio.h> // printf
#include <stdlib.h> // malloc
#include <string.h>
#include <commctrl.h>

static LPWSTR* wargv;
static int argc;
static boolean initialized = NO;

// 窗口过程回调
static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
// 设置客户区大小
static void SetClientSize(HWND hWnd, int width, int height);
// 居中显示窗口
static void CenterWindow(HWND hWnd);
// 初始化获取命令行参数
static void InitCommandLineArgs();

static DWORD WINAPI main_thread(LPVOID lpParameter) {
  int code;
  HWND hwnd;
  WCHAR display[10];

  hwnd = (HWND)lpParameter;
  memset(display, 0, 10);
  code = start(argc, wargv);

  ShowWindow(hwnd, SW_HIDE);
  if (code != 0) {
    swprintf(display, 10, L"Failed Code：%d", code);
    MessageBoxW(hwnd, display, L"updater", MB_OK);
  }
  SendMessageW(hwnd, WM_CLOSE, (WPARAM)NULL, (LPARAM)NULL);
  // MessageBoxW(NULL, _itow(code, display, 10), L"参数", MB_OK);
  return code;
}

int APIENTRY wWinMain(
  _In_ HINSTANCE hInstance,
  _In_opt_ HINSTANCE hPrevInstance,
  _In_ LPWSTR lpCmdLine,
  _In_ int nCmdShow) {
  InitCommandLineArgs();
  HWND hwnd; // 主窗口句柄在 CreateWindow 中赋值
  MSG msg; // 消息变量在 GetMessage 中使用
  WNDCLASS wndclass; // 声明窗口类
  WCHAR szAppName[] = L"updater"; // 类名和窗口名
  // wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); // 窗口背景
  wndclass.hbrBackground = CreateSolidBrush(RGB(240, 240, 240));
  wndclass.hCursor = LoadCursorW(NULL, IDC_ARROW); // 鼠标
  wndclass.hIcon = LoadIconW(NULL, IDI_APPLICATION); // 图标
  wndclass.lpszClassName = szAppName; // 类名
  wndclass.cbClsExtra = 0; // 类的额外参数
  wndclass.cbWndExtra = 0; // 窗口的额外参数 用于基于同一窗口类的窗口各自区分.

  wndclass.lpszMenuName = NULL; // 菜单名 可以用作子窗口的id
  wndclass.style = CS_HREDRAW | CS_VREDRAW; // 窗口风格
  wndclass.lpfnWndProc = WndProc; // 窗口过程
  wndclass.hInstance = hInstance; // 包含窗口过程的实例句柄

  if (!RegisterClassW(&wndclass)) return 0; // 注册窗口类

  hwnd = CreateWindowW(
    szAppName, // 窗口类名
    szAppName, // 窗口标题
    (WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME) & ~WS_MAXIMIZEBOX, // 窗口风格
    0, // 初始的x坐标
    0, // 初始的y坐标
    0, // 初始的宽度
    0, // 初始的高度
    NULL, // 父窗口
    NULL, // 菜单
    hInstance, // 和窗口相关的实例句柄
    NULL // 额外参数
  );

  SetClientSize(hwnd, 280, 90);
  CenterWindow(hwnd);
  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);

  CreateThread(
    NULL,
    0,
    (LPTHREAD_START_ROUTINE)main_thread,
    hwnd,
    0,
    0
  );

  while (GetMessageW(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return msg.wParam;
}

void SetClientSize(HWND hWnd, int width, int height) {
  if (!hWnd) {
    return;
  }
  RECT rectProgram, rectClient;
  GetWindowRect(hWnd, &rectProgram); // 获得程序窗口位于屏幕坐标
  GetClientRect(hWnd, &rectClient); // 获得客户区坐标
  // 非客户区宽高
  int nWidth = rectProgram.right - rectProgram.left - (rectClient.right - rectClient.left);
  int nHeiht = rectProgram.bottom - rectProgram.top - (rectClient.bottom - rectClient.top);
  nWidth += width;
  nHeiht += height;
  rectProgram.right = nWidth;
  rectProgram.bottom = nHeiht;
  // int showToScreenx = GetSystemMetrics(SM_CXSCREEN) / 2 - nWidth / 2;
  // int showToScreeny = GetSystemMetrics(SM_CYSCREEN) / 2 - nHeiht / 2;
  // MoveWindow(hWnd, showToScreenx, showToScreeny, rectProgram.right, rectProgram.bottom, FALSE);
  MoveWindow(hWnd, rectProgram.left, rectProgram.top, rectProgram.right, rectProgram.bottom, FALSE);
}

void CenterWindow(HWND hWnd) {
  if (!hWnd) {
    return;
  }
  // 居中处理
  RECT rectProgram;
  GetWindowRect(hWnd, &rectProgram);
  int showToScreenx = GetSystemMetrics(SM_CXSCREEN) / 2 - (rectProgram.right - rectProgram.left) / 2;
  int showToScreeny = GetSystemMetrics(SM_CYSCREEN) / 2 - (rectProgram.bottom - rectProgram.top) / 2;
  MoveWindow(hWnd, showToScreenx, showToScreeny, rectProgram.right - rectProgram.left, rectProgram.bottom - rectProgram.top, FALSE);
}

void InitCommandLineArgs() {
  if (!initialized) {
    argc = 0;
    wargv = NULL;
    wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    initialized = YES;
  }
}

LRESULT WINAPI WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  static HFONT hFont;
  static HWND hwndPB;
  static HWND hStaticText;

  switch (message) {
  case WM_CREATE: {
    WCHAR statictext[60];
    // WCHAR argcString[3];
    memset(statictext, 0, 120);
    // memset(argcString, 0, 6);
    // _itow_s(argc, argcString, 3, 10);
    wcscat(statictext, L"Updating...");
    // wcscat(statictext, argcString);
    InitCommonControls(); // 注册进度条类 PROGRESS_CLASS
    hwndPB = CreateWindowW(
      PROGRESS_CLASSW,
      NULL,
      WS_CHILD | WS_VISIBLE | PBS_MARQUEE,
      0, 0, 0, 0, // 位置和大小在 WM_SIZE 中设置
      hwnd,
      (HMENU)0,
      ((LPCREATESTRUCT)lParam)->hInstance,
      NULL);
    SendMessageW(hwndPB, (UINT)PBM_SETMARQUEE, (WPARAM)1, (LPARAM)NULL);

    hStaticText = CreateWindowW(
      L"static",
      statictext,
      WS_CHILD | WS_VISIBLE,
      0, 0, 0, 0,
      hwnd,
      (HMENU)1000,
      ((LPCREATESTRUCT)lParam)->hInstance,
      NULL
    );
    hFont = CreateFontW(-12 /* 高 */, -6 /* 宽 */, 0, 0, 0 /* 700 表示粗体 */,
      FALSE /* 斜体 */, FALSE /* 下划线 */, FALSE /* 删除线 */, DEFAULT_CHARSET,
      OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY,
      FF_DONTCARE, L"Microsoft YaHei"
    );
    SendMessageW(hStaticText, WM_SETFONT, (WPARAM)hFont, (LPARAM)NULL);

    return 0;
  }
  case WM_SIZE: {
    RECT rc;
    GetClientRect(hwnd, &rc);
    MoveWindow(
      hwndPB,
      rc.left + 25,
      (rc.bottom - rc.top) / 2 - 10,
      rc.right - rc.left - 50,
      // (rc.bottom - rc.top) / 10,
      20,
      TRUE
    );
    MoveWindow(
      hStaticText,
      rc.left + 25,
      (rc.bottom - rc.top) / 2 - 30,
      rc.right - rc.left - 50,
      // (rc.bottom - rc.top) / 10,
      20,
      TRUE
    );
    return 0;
  }
  case WM_CLOSE:
    DestroyWindow(hwnd);
    return 0;
  case WM_DESTROY:
    if (wargv != NULL) {
      LocalFree(wargv);
    }
    PostQuitMessage(0);
    return 0;
  default:
    return DefWindowProcW(hwnd, message, wParam, lParam);
  }
  return 0;
}

#else

#include <util.h>
#include <stdio.h> // printf
#include <stdlib.h> // malloc
#include <string.h>
int main (int argc, char** argv) {
  int code;
  code = start(argc, argv);
  printf("exit code: %d\n", code);
  return code;
}
#endif
