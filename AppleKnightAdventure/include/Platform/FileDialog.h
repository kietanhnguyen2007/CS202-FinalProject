#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include <string>

class FileDialog {
public:
    static std::string OpenFile(const char* filter);
};

#endif // FILE_DIALOG_H
