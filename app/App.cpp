#include "App.h"
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <string>
#include <thread>

App::App(const CommandLineOptions &options)
    : m_options(options)
    , m_db()
{

    std::error_code err;
    std::filesystem::create_directory(m_options.backupFolderPath, err);
    if (err) {
        throw std::runtime_error(err.message().c_str());
    }
}

void App::run()
{
    using namespace std;

    Watcher watcher(m_options.watchFolderPath);

    DirectoryChangeList changesToProcessLater{};
    while (true) {
        DirectoryChangeList dirChanges;
        watcher.getChanges(dirChanges);
        removeDuplicates(dirChanges);

        DirectoryChangeList tempNotProcessedChanges{};
        processChanges(dirChanges, tempNotProcessedChanges);
        processChanges(tempNotProcessedChanges, changesToProcessLater);

        this_thread::sleep_for(1s);
    }
}

void App::removeDuplicates(fs::Watcher::DirectoryChangeList &changes)
{
    auto a = unique(changes.begin(), changes.end(), [](const fs::Watcher::DirectoryChange &a, const fs::Watcher::DirectoryChange &b) {
        // TODO: think about moving/renaming files. it's ok?
        return a.event == b.event &&
               a.path == b.path;
    });
    changes.erase(a, changes.end());
}

void App::processChanges(fs::Watcher::DirectoryChangeList &changes, fs::Watcher::DirectoryChangeList &changesToProcessLater)
{
    using namespace std;

    changesToProcessLater.clear();
    for (const auto &it : changes) {
        filesystem::path changeBackupPath{m_options.backupFolderPath / filesystem::relative(it.path, m_options.watchFolderPath)}; // TODO:!!! careful, need asserts

        // if (!filesystem::is_directory(it.path))
        //     changeBackupPath.append(".bak");

        error_code err;
        switch (it.event) {
        case DirectoryChangeEvent::Created:
            filesystem::copy(it.path, changeBackupPath, filesystem::copy_options::update_existing, err);
            if (err)
                changesToProcessLater.push_back(move(it));
            else
                m_db.insert({it.event, changeBackupPath});
            break;
        case DirectoryChangeEvent::Deleted:
            filesystem::remove_all(changeBackupPath);
            m_db.insert({it.event, changeBackupPath});
            break;
        case DirectoryChangeEvent::Modified:
            filesystem::copy(it.path, changeBackupPath, filesystem::copy_options::update_existing, err);
            if (err.value() == 32) //!!  err 32, The process cannot access the file because it is being used by another process.
                changesToProcessLater.push_back(move(it));
            else
                m_db.insert({it.event, changeBackupPath});
            break;
        case DirectoryChangeEvent::MovedTo:
            filesystem::rename(lastMove, changeBackupPath, err);
            if (err.value() == 32) {                                                          //!!  err 32, The process cannot access the file because it is being used by another process.
                changesToProcessLater.push_back({DirectoryChangeEvent::MovedFrom, lastMove}); // nooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
                changesToProcessLater.push_back(move(it));
            } else {
                m_db.insert({DirectoryChangeEvent::MovedFrom, lastMove});
                m_db.insert({it.event, changeBackupPath});
            }
            detectedMoveStart = false;
            break;
        case DirectoryChangeEvent::MovedFrom:
            lastMove = changeBackupPath;
            detectedMoveStart = true;
            break;
        default:
            break;
        }
        if (err)
            cout << err.message() << '\n';
    }
}
