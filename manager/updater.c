#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
#include <wchar.h>
#include <stdio.h>

static const wchar_t *arg_value(int argc, wchar_t **argv, const wchar_t *name) {
    for (int i = 1; i + 1 < argc; ++i) if (wcscmp(argv[i], name) == 0) return argv[i + 1];
    return NULL;
}

static int recognized(const wchar_t *root) {
    wchar_t manager[32768], version[32768], payload[32768];
    swprintf_s(manager, 32768, L"%s\\ZanikManager.exe", root);
    swprintf_s(version, 32768, L"%s\\VERSION", root);
    swprintf_s(payload, 32768, L"%s\\payload\\payloads.json", root);
    return GetFileAttributesW(manager) != INVALID_FILE_ATTRIBUTES &&
           GetFileAttributesW(version) != INVALID_FILE_ATTRIBUTES &&
           GetFileAttributesW(payload) != INVALID_FILE_ATTRIBUTES;
}

static void journal_state(const wchar_t *path, const wchar_t *state) {
    wchar_t status_path[32768];
    swprintf_s(status_path, 32768, L"%s.status", path);
    FILE *file = NULL;
    if (_wfopen_s(&file, status_path, L"w, ccs=UTF-8") == 0 && file) {
        fwprintf(file, L"{\"schemaVersion\":1,\"state\":\"%s\"}\n", state);
        fclose(file);
    }
}

static int updater_main(int argc, wchar_t **argv) {
    const wchar_t *pid_text = arg_value(argc, argv, L"--pid");
    const wchar_t *source = arg_value(argc, argv, L"--source");
    const wchar_t *target = arg_value(argc, argv, L"--target");
    const wchar_t *backup = arg_value(argc, argv, L"--backup");
    const wchar_t *handshake = arg_value(argc, argv, L"--handshake");
    const wchar_t *journal = arg_value(argc, argv, L"--journal");
    int smoke = arg_value(argc, argv, L"--smoke-test") != NULL;
    if (!pid_text || !source || !target || !backup || !handshake || !journal ||
        !recognized(source) || !recognized(target) || _wcsicmp(source, target) == 0) return 2;

    HANDLE old_process = OpenProcess(SYNCHRONIZE, FALSE, wcstoul(pid_text, NULL, 10));
    if (old_process) {
        if (WaitForSingleObject(old_process, 60000) != WAIT_OBJECT_0) { CloseHandle(old_process); return 3; }
        CloseHandle(old_process);
    }
    DeleteFileW(handshake);
    if (GetFileAttributesW(backup) != INVALID_FILE_ATTRIBUTES) return 4;
    journal_state(journal, L"swapping");
    if (!MoveFileExW(target, backup, MOVEFILE_WRITE_THROUGH)) { journal_state(journal, L"rollback-needed"); return 5; }
    if (!MoveFileExW(source, target, MOVEFILE_WRITE_THROUGH)) {
        MoveFileExW(backup, target, MOVEFILE_WRITE_THROUGH);
        journal_state(journal, L"rolled-back"); return 6;
    }

    wchar_t exe[32768], command[65536];
    swprintf_s(exe, 32768, L"%s\\ZanikManager.exe", target);
    swprintf_s(command, 65536, L"\"%s\" --update-handshake \"%s\"%s", exe, handshake,
               smoke ? L" --smoke-test" : L"");
    STARTUPINFOW startup = {sizeof(startup)};
    PROCESS_INFORMATION process = {0};
    if (!CreateProcessW(exe, command, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, NULL, target, &startup, &process)) {
        MoveFileExW(target, source, MOVEFILE_WRITE_THROUGH);
        MoveFileExW(backup, target, MOVEFILE_WRITE_THROUGH);
        journal_state(journal, L"rolled-back"); return 7;
    }
    CloseHandle(process.hThread);
    int ready = 0;
    for (int i = 0; i < 300; ++i) {
        if (GetFileAttributesW(handshake) != INVALID_FILE_ATTRIBUTES) { ready = 1; break; }
        if (WaitForSingleObject(process.hProcess, 100) == WAIT_OBJECT_0) break;
    }
    if (!ready) {
        TerminateProcess(process.hProcess, 1);
        WaitForSingleObject(process.hProcess, 5000);
        MoveFileExW(target, source, MOVEFILE_WRITE_THROUGH);
        MoveFileExW(backup, target, MOVEFILE_WRITE_THROUGH);
        journal_state(journal, L"rolled-back");
        CloseHandle(process.hProcess); return 8;
    }
    CloseHandle(process.hProcess);
    journal_state(journal, L"complete");
    return 0;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command_line, int show) {
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return 1;
    int result = updater_main(argc, argv);
    int smoke = arg_value(argc, argv, L"--smoke-test") != NULL;
    LocalFree(argv);
    if (result != 0 && !smoke) {
        MessageBoxW(NULL, L"Zanik Manager could not complete the update. The previous portable installation was preserved or restored. See the update journal in LocalAppData for details.",
                    L"Zanik Manager update", MB_ICONERROR);
    }
    return result;
}
