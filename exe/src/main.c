#ifdef _WIN32
#include <Windows.h>
#include <util.h>
#include <stdio.h> // printf
#include <stdlib.h> // malloc
#include <string.h>
#include <commctrl.h>

#define PROGRESS_DIALOG 1001
#define PROGRESS_SLIDER 10001

static LPWSTR* wargv;
static int argc;
static boolean initialized = NO;
static boolean firstShow = NO;

static void InitCommandLineArgs();

void InitCommandLineArgs() {
  if (!initialized) {
    argc = 0;
    wargv = NULL;
    wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    initialized = YES;
  }
}

static DWORD WINAPI main_thread(LPVOID lpParameter) {
  int code;
  HWND hwnd;
  WCHAR display[20];

  hwnd = (HWND)lpParameter;
  memset(display, 0, 20 * sizeof(WCHAR));
  code = start(argc, wargv);

  ShowWindow(hwnd, SW_HIDE);
  if (code != 0) {
    swprintf(display, 20, L"Update Failed：%d", code);
    MessageBoxW(hwnd, display, L"updater", MB_OK);
  }
  SendMessageW(hwnd, WM_CLOSE, (WPARAM)NULL, (LPARAM)NULL);
  return code;
}

INT_PTR WINAPI dlgproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
  InitCommandLineArgs();
  
  DialogBoxParamW(hInstance, MAKEINTRESOURCEW(PROGRESS_DIALOG), NULL, dlgproc, (LPARAM)NULL);
  return 0;
}

INT_PTR WINAPI dlgproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
  case WM_SHOWWINDOW:
    if (wParam == TRUE && firstShow == FALSE) {
      firstShow = TRUE;
      CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)main_thread,
        hwnd,
        0,
        0
      );
    }
    return 0;
  case WM_INITDIALOG:
    SendDlgItemMessageW(hwnd, PROGRESS_SLIDER, PBM_SETMARQUEE, 1, 0);

    RECT rect;
    GetWindowRect(hwnd, &rect);

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    GetWindowRect(GetDesktopWindow(), &rect);

    SetWindowPos(
      hwnd,
      HWND_TOPMOST,
      rect.right / 2 - width / 2,
      rect.bottom / 2 - height / 2,
      width,
      height,
      0);

    ShutdownBlockReasonCreate(hwnd, L"Application is updating...");
    return 0;
  case WM_CLOSE:
    PostQuitMessage(0);
    return 1;
  case WM_DESTROY:
    ShutdownBlockReasonDestroy(hwnd);
    return 0;
  default:
    return 0;
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
