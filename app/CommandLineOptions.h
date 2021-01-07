#pragma once
#include <filesystem>

struct CommandLineOptions {
    std::filesystem::path watchFolderPath;
    std::filesystem::path backupFolderPath;
};
