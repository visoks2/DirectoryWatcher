#pragma once
#include "CommandLineOptions.h"
#include "fs/Database.h"
#include "fs/Watcher.h"
#include <filesystem>

class App
{
  public:
    App(const CommandLineOptions &options);
    void run();

  private:
    static constexpr auto *BACKUP_FILE_SUFFIX = ".bak";
    using Watcher = fs::Watcher;
    using DirectoryChangeList = fs::Watcher::DirectoryChangeList;
    using DirectoryChange = fs::Watcher::DirectoryChange;
    using DirectoryChangeEvent = fs::Watcher::DirectoryChangeEvent;

    static void removeDuplicates(fs::Watcher::DirectoryChangeList &changes);
    void processChanges(fs::Watcher::DirectoryChangeList &changes, fs::Watcher::DirectoryChangeList &changesToProcessLater);
    // std::error_code onFileCreated()
    // {
    // }
    // std::error_code onFileModified()
    // {
    // }
    // std::error_code onFileCreated()
    // {
    // }

    const CommandLineOptions &m_options;
    fs::Database m_db;
    // WIP
    std::filesystem::path lastMove;
    bool detectedMoveStart;
};
