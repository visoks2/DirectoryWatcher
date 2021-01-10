#pragma once
#include <filesystem>
struct CommandLineOptions {
    std::filesystem::path watchFolderPath;
    std::filesystem::path backupFolderPath;
    bool showLog;
    std::string showLog_pathRegex;
    std::string showLog_dateFrom;
    std::string showLog_dateTo;
};
