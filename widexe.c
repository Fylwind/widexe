#include <stdio.h>
#include <stdlib.h>
#ifndef UNICODE
#  define UNICODE
#endif
#include <windows.h>
#if defined(__MINGW32__) && !defined(IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION)
#  define IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION 16
#endif
#define XFER_BUFFER_SIZE 2048
#ifndef IMAGE_SIZEOF_NT_OPTIONAL_HEADER
#  define IMAGE_SIZEOF_NT_OPTIONAL32_HEADER 224
#  define IMAGE_SIZEOF_NT_OPTIONAL64_HEADER 240
#  ifdef _WIN64
#    define IMAGE_SIZEOF_NT_OPTIONAL_HEADER IMAGE_SIZEOF_NT_OPTIONAL64_HEADER
#  else
#    define IMAGE_SIZEOF_NT_OPTIONAL_HEADER IMAGE_SIZEOF_NT_OPTIONAL32_HEADER
#  endif
#endif

int abs_seek(HANDLE hFile, LONG offset, DWORD* newOffset) {
    *newOffset = SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
    if (*newOffset == INVALID_SET_FILE_POINTER) {
        fwprintf(stderr, L"SetFilePointer failed, error %lu.\n",
                 GetLastError());
        return 0;
    }
    return 1;
}

int read_bytes(HANDLE hFile, LPVOID buffer, DWORD size) {
    DWORD bytes;
    if (!ReadFile(hFile, buffer, size, &bytes, NULL)) {
        fwprintf(stderr, L"ReadFile failed, error %lu.\n", GetLastError());
        return 0;
    } else if (size != bytes) {
        fwprintf(stderr,
                 L"Read the wrong number of bytes, expected %lu, got %lu.\n",
                 size, bytes);
        return 0;
    }
    return 1;
}

int wmain(int argc, wchar_t** argv) {
    HANDLE handle;
    DWORD coff_header_offset;
    DWORD more_dos_header[16];
    ULONG nt_signature;
    IMAGE_DOS_HEADER image_dos_header;
    IMAGE_FILE_HEADER image_file_header;
    IMAGE_OPTIONAL_HEADER image_optional_header;

    if (argc != 2) {
        fwprintf(stderr,
                 L"Usage: %s FILE\n"
                 L"Identifies the subsystem of a Windows executable.\n",
                 argv[0]);
        return EXIT_FAILURE;
    }

    /* Open the reference file. */
    handle = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (handle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND)
            fwprintf(stderr, L"Can't find: %s\n", argv[1]);
        else
            fwprintf(stderr, L"Error %lu: can't open: %s\n", error, argv[1]);
        return EXIT_FAILURE;
    }

    /*  Read the MS-DOS image header. */
    if (!read_bytes(handle, &image_dos_header, sizeof(IMAGE_DOS_HEADER)))
        return EXIT_FAILURE;

    if (image_dos_header.e_magic != IMAGE_DOS_SIGNATURE) {
        fwprintf(stderr, L"This doesn't seem to be an executable.\n");
        return EXIT_FAILURE;
    }

    /* Read more MS-DOS header. */
    if (!read_bytes(handle, more_dos_header, sizeof(more_dos_header)))
        return EXIT_FAILURE;

    /* Get actual COFF header. */
    if (!abs_seek(handle, image_dos_header.e_lfanew, &coff_header_offset))
        return EXIT_FAILURE;

    if (!read_bytes(handle, &nt_signature, sizeof(ULONG)))
        return EXIT_FAILURE;

    if (nt_signature != IMAGE_NT_SIGNATURE) {
        fwprintf(stderr, L"Missing NT signature.  Unknown file type.\n");
        return EXIT_FAILURE;
    }

    if (!read_bytes(handle,&image_file_header, IMAGE_SIZEOF_FILE_HEADER))
        return EXIT_FAILURE;

    /* Read optional header. */
    if (!read_bytes(handle, &image_optional_header,
                    IMAGE_SIZEOF_NT_OPTIONAL_HEADER))
        return EXIT_FAILURE;

    switch (image_optional_header.Subsystem) {
    case IMAGE_SUBSYSTEM_UNKNOWN:
        wprintf(L"Unknown subsystem (IMAGE_SUBSYSTEM_UNKNOWN).\n");
        break;
    case IMAGE_SUBSYSTEM_NATIVE:
        wprintf(L"No subsystem required (device drivers and native system "
                L"processes).\n");
        break;
    case IMAGE_SUBSYSTEM_WINDOWS_GUI:
        wprintf(L"Windows graphical user interface (GUI) subsystem.\n");
        break;
    case IMAGE_SUBSYSTEM_WINDOWS_CUI:
        wprintf(L"Windows character-mode user interface (CUI) subsystem.\n");
        break;
    case IMAGE_SUBSYSTEM_OS2_CUI:
        wprintf(L"OS/2 character-mode user interface (CUI) subsystem.\n");
        break;
    case IMAGE_SUBSYSTEM_POSIX_CUI:
        wprintf(L"POSIX character-mode user interface (CUI) subsystem.\n");
        break;
    case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:
        wprintf(L"Windows CE system.\n");
        break;
    case IMAGE_SUBSYSTEM_EFI_APPLICATION:
        wprintf(L"Extensible Firmware Interface (EFI) application.\n");
        break;
    case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
        wprintf(L"Extensible Firmware Interface (EFI) driver with boot "
                L"services.\n");
        break;
    case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
        wprintf(L"Extensible Firmware Interface (EFI) driver with run-time "
                L"services.\n");
        break;
    case IMAGE_SUBSYSTEM_EFI_ROM:
        wprintf(L"Extensible Firmware Interface (EFI) ROM image.\n");
        break;
    case IMAGE_SUBSYSTEM_XBOX:
        wprintf(L"Xbox system.\n");
        break;
    case IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION:
        wprintf(L"Boot application.\n");
        break;
    default:
        wprintf(L"Unrecognized subsystem: %u\n",
                image_optional_header.Subsystem);
        break;
    }
    return EXIT_SUCCESS;
}

/* Cygwin/MinGW doesn't support `wmain` so we have to improvise here */
#if defined(__MINGW32__) || defined(__CYGWIN__)
int main(int argc, char** argv) {
    (void) argv;
    wchar_t** argvw = CommandLineToArgvW(GetCommandLine(), &argc);
    if (!argv) {
        fwprintf(stderr, L"Error %lu: failed to get command-line arguments.",
                 GetLastError());
        return EXIT_FAILURE;
    }
    return wmain(argc, argvw);
}
#endif
