#include <windows.h>
#include <wchar.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command_line, int show) {
    wchar_t module[MAX_PATH], root[MAX_PATH], python[MAX_PATH], args[32768];
    if (!GetModuleFileNameW(NULL, module, MAX_PATH)) return 2;
    wcscpy_s(root, MAX_PATH, module);
    wchar_t *slash = wcsrchr(root, L'\\');
    if (!slash) return 2;
    *slash = 0;
    swprintf_s(python, MAX_PATH, L"%s\\runtime\\pythonw.exe", root);
    swprintf_s(args, 32768, L"\"%s\" -m zanik_manager.app %s", python, command_line ? command_line : L"");
    STARTUPINFOW startup = {sizeof(startup)};
    PROCESS_INFORMATION process = {0};
    if (!CreateProcessW(python, args, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, NULL, root, &startup, &process)) {
        MessageBoxW(NULL, L"The bundled Zanik Manager runtime could not be started. Re-extract the complete archive and try again.", L"Zanik Manager", MB_ICONERROR);
        return 3;
    }
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    return 0;
}
