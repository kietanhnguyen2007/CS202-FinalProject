#include "Platform/FileDialog.h"
#include <windows.h>
#include <commdlg.h>
#include <string>

std::string FileDialog::OpenFile(const char* filter) {
    OPENFILENAMEA ofn;
    CHAR szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    
    // We don't have the HWND directly unless we include glfw3 native or raylib native.
    // Setting hwndOwner to NULL is fine for a file dialog (it will just be a standalone window)
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    
    // Default to the game's assets/levels directory
    char currentDir[256];
    GetCurrentDirectoryA(256, currentDir);
    std::string initDir = std::string(currentDir) + "\\assets\\levels";
    ofn.lpstrInitialDir = initDir.c_str();
    
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }
    return std::string();
}
