#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
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

static void journal_status(const wchar_t *path, const wchar_t *state,
                           const wchar_t *operation, DWORD error) {
    wchar_t status_path[32768];
    swprintf_s(status_path, 32768, L"%s.status", path);
    FILE *file = NULL;
    if (_wfopen_s(&file, status_path, L"w, ccs=UTF-8") == 0 && file) {
        fwprintf(file, L"{\"schemaVersion\":1,\"state\":\"%ls\",\"operation\":\"%ls\",\"windowsError\":%lu}\n",
                 state, operation ? operation : L"", error);
        fclose(file);
    }
}

static int move_with_retry(const wchar_t *source, const wchar_t *target, DWORD *last_error) {
    for (int attempt = 0; attempt < 50; ++attempt) {
        if (MoveFileExW(source, target, MOVEFILE_WRITE_THROUGH)) return 1;
        *last_error = GetLastError();
        if (*last_error != ERROR_ACCESS_DENIED && *last_error != ERROR_SHARING_VIOLATION &&
            *last_error != ERROR_LOCK_VIOLATION) return 0;
        Sleep(100);
    }
    return 0;
}

static int path_is_within(const wchar_t *path, const wchar_t *root) {
    size_t length = wcslen(root);
    while (length && (root[length - 1] == L'\\' || root[length - 1] == L'/')) --length;
    if (!length || _wcsnicmp(path, root, length) != 0) return 0;
    return path[length] == L'\0' || path[length] == L'\\' || path[length] == L'/';
}

static DWORD running_install_processes(const wchar_t *target, DWORD requested_pid) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 1;
    PROCESSENTRY32W entry = {0}; entry.dwSize = sizeof(entry);
    DWORD count = 0, self = GetCurrentProcessId();
    if (Process32FirstW(snapshot, &entry)) do {
        if (!entry.th32ProcessID || entry.th32ProcessID == self) continue;
        HANDLE process = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION,
                                     FALSE, entry.th32ProcessID);
        if (!process) {
            if (entry.th32ProcessID == requested_pid) ++count;
            continue;
        }
        int running = WaitForSingleObject(process, 0) == WAIT_TIMEOUT;
        wchar_t image[32768]; DWORD image_length = 32768;
        int inside = running && QueryFullProcessImageNameW(process, 0, image, &image_length) &&
                     path_is_within(image, target);
        if (running && (entry.th32ProcessID == requested_pid || inside)) ++count;
        CloseHandle(process);
    } while (Process32NextW(snapshot, &entry));
    CloseHandle(snapshot);
    return count;
}

static int wait_for_install_processes(const wchar_t *target, DWORD requested_pid, DWORD timeout_ms) {
    int clear_scans = 0;
    for (DWORD elapsed = 0; elapsed < timeout_ms; elapsed += 100) {
        if (running_install_processes(target, requested_pid) == 0) {
            if (++clear_scans >= 2) return 1;
        } else clear_scans = 0;
        Sleep(100);
    }
    return 0;
}

static int updater_main(int argc, wchar_t **argv) {
    const wchar_t *pid_text = arg_value(argc, argv, L"--pid");
    const wchar_t *source = arg_value(argc, argv, L"--source");
    const wchar_t *target = arg_value(argc, argv, L"--target");
    const wchar_t *backup = arg_value(argc, argv, L"--backup");
    const wchar_t *handshake = arg_value(argc, argv, L"--handshake");
    const wchar_t *journal = arg_value(argc, argv, L"--journal");
    int smoke = arg_value(argc, argv, L"--smoke-test") != NULL;
    const wchar_t *timeout_text = arg_value(argc, argv, L"--lock-timeout-ms");
    if (!pid_text || !source || !target || !backup || !handshake || !journal ||
        !recognized(source) || !recognized(target) || _wcsicmp(source, target) == 0) return 2;

    wchar_t updater_path[32768];
    DWORD updater_length = GetModuleFileNameW(NULL, updater_path, 32768);
    wchar_t *separator = updater_length ? wcsrchr(updater_path, L'\\') : NULL;
    if (!separator) {
        journal_status(journal, L"rollback-needed", L"resolve-updater-directory", GetLastError());
        return 10;
    }
    *separator = L'\0';
    if (!SetCurrentDirectoryW(updater_path)) {
        journal_status(journal, L"rollback-needed", L"leave-install-directory", GetLastError());
        return 10;
    }

    DWORD lock_timeout_ms = 60000;
    if (timeout_text) {
        lock_timeout_ms = wcstoul(timeout_text, NULL, 10);
        if (!smoke || lock_timeout_ms < 100 || lock_timeout_ms > 60000) return 2;
    }
    DWORD requested_pid = wcstoul(pid_text, NULL, 10);
    if (!requested_pid || !wait_for_install_processes(target, requested_pid, lock_timeout_ms)) {
        journal_status(journal, L"rollback-needed", L"wait-for-portable-processes", WAIT_TIMEOUT);
        return 3;
    }
    DeleteFileW(handshake);
    if (GetFileAttributesW(backup) != INVALID_FILE_ATTRIBUTES) return 4;
    DWORD move_error = ERROR_SUCCESS;
    journal_status(journal, L"swapping", L"move-current-to-backup", ERROR_SUCCESS);
    if (!move_with_retry(target, backup, &move_error)) {
        journal_status(journal, L"rollback-needed", L"move-current-to-backup", move_error); return 5;
    }
    if (!move_with_retry(source, target, &move_error)) {
        DWORD install_error = move_error;
        if (!move_with_retry(backup, target, &move_error)) {
            journal_status(journal, L"rollback-failed", L"restore-current-after-install-move", move_error); return 9;
        }
        journal_status(journal, L"rolled-back", L"move-staged-to-current", install_error); return 6;
    }

    wchar_t python[32768], command[65536];
    swprintf_s(python, 32768, L"%s\\runtime\\pythonw.exe", target);
    swprintf_s(command, 65536, L"\"%s\" -m zanik_manager.app --update-handshake \"%s\"%s", python, handshake,
               smoke ? L" --smoke-test" : L"");
    STARTUPINFOW startup = {sizeof(startup)};
    PROCESS_INFORMATION process = {0};
    if (!CreateProcessW(python, command, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, NULL, target, &startup, &process)) {
        DWORD launch_error = GetLastError();
        if (!move_with_retry(target, source, &move_error) ||
            !move_with_retry(backup, target, &move_error)) {
            journal_status(journal, L"rollback-failed", L"restore-current-after-launch", move_error); return 9;
        }
        journal_status(journal, L"rolled-back", L"launch-new-manager", launch_error); return 7;
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
        if (!move_with_retry(target, source, &move_error) ||
            !move_with_retry(backup, target, &move_error)) {
            journal_status(journal, L"rollback-failed", L"restore-current-after-handshake", move_error);
            CloseHandle(process.hProcess); return 9;
        }
        journal_status(journal, L"rolled-back", L"wait-for-startup-handshake", WAIT_TIMEOUT);
        CloseHandle(process.hProcess); return 8;
    }
    CloseHandle(process.hProcess);
    journal_status(journal, L"complete", L"update-and-relaunch", ERROR_SUCCESS);
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
