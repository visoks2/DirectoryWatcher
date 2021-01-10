#pragma once
#include "CommandLineOptions.h"
#include "fs/Database.hpp"
#include "fs/Watcher.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <string>
#include <thread>

// TODO: remove
constexpr bool NoFileSystemChanges = false;

class App
{
  public:
    App(const CommandLineOptions &options)
        : m_options(options)
        , m_db()
    {
        std::error_code err;
        std::filesystem::create_directory(m_options.backupFolderPath, err);
        if (err) {
            throw std::exception(err.message().c_str());
        }
    }
    void run()
    {
        using namespace std;
        using Watcher = fs::Watcher;
        using DirectoryChangeList = fs::Watcher::DirectoryChangeList;
        using DirectoryChange = fs::Watcher::DirectoryChange;

        Watcher watcher(m_options.watchFolderPath);

        DirectoryChangeList changesToProcessLater{};
        while (true) {
            DirectoryChangeList dirChanges;
            watcher.getChanges(dirChanges);
            auto a = unique(dirChanges.begin(), dirChanges.end(), [](const DirectoryChange &a, const DirectoryChange &b) {
                // TODO: think about moving/renaming files. it's ok?
                return a.event == b.event &&
                       a.path == b.path;
            });
            dirChanges.erase(a, dirChanges.end());

            DirectoryChangeList tempNotProcessedChanges{};
            processChanges(dirChanges, tempNotProcessedChanges);
            processChanges(tempNotProcessedChanges, changesToProcessLater);

            this_thread::sleep_for(1s);
        }
    }

  private:
    void processChanges(fs::Watcher::DirectoryChangeList &changes, fs::Watcher::DirectoryChangeList &changesToProcessLater)
    {
        using namespace std;
        using DirectoryChangeEvent = fs::Watcher::DirectoryChangeEvent;

        changesToProcessLater.clear();
        for (const auto &it : changes) {
            filesystem::path changeBackupPath{m_options.backupFolderPath / filesystem::relative(it.path, m_options.watchFolderPath)}; // TODO:!!! careful, need asserts
            error_code err;

            cout << "changed: '" << it.path << "' ";
            if (filesystem::is_directory(it.path))
                cout << "isDir ";

            switch (it.event) {
            case DirectoryChangeEvent::Created:
                cout << "created ";
                if constexpr (!NoFileSystemChanges) {
                    filesystem::copy(it.path, changeBackupPath, filesystem::copy_options::update_existing, err);
                    if (!err)
                        changesToProcessLater.push_back(move(it));
                }
                break;
            case DirectoryChangeEvent::Deleted:
                cout << "deleted ";
                if constexpr (!NoFileSystemChanges) {
                    filesystem::remove_all(changeBackupPath);
                }
                break;
            case DirectoryChangeEvent::Modified:
                cout << "modified ";
                if constexpr (!NoFileSystemChanges) {
                    filesystem::copy(it.path, changeBackupPath, filesystem::copy_options::update_existing, err);
                    if (err.value() == 32) //!!  err 32, The process cannot access the file because it is being used by another process.
                        changesToProcessLater.push_back(move(it));
                }
                break;
            case DirectoryChangeEvent::MovedTo:
                cout << "moved_to \n";
                if constexpr (!NoFileSystemChanges) {
                    filesystem::rename(lastMove, changeBackupPath, err);
                }
                detectedMoveStart = false;
                break;
            case DirectoryChangeEvent::MovedFrom:
                cout << "moved_from ";
                lastMove = changeBackupPath;
                detectedMoveStart = true;
                break;
            default:
                break;
            }
            cout << endl;
            if (err)
                cout << err.message() << endl;
            else
                m_db.insert(it);
        }
    }
    const CommandLineOptions &m_options;
    fs::Database m_db;
    // WIP
    std::filesystem::path lastMove;
    bool detectedMoveStart;
};
