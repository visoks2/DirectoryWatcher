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
}

void App::run()
{
    using namespace std;

    if (m_options.showLog) {
        m_db.select(m_options.showLog_pathRegex, m_options.showLog_dateFrom, m_options.showLog_dateTo);
        return;
    }

    error_code err;
    filesystem::create_directory(m_options.backupFolderPath, err);
    if (err) {
        throw runtime_error(err.message().c_str());
    }

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
        changeBackupPath = changeBackupPath.lexically_normal();

        // if (!filesystem::is_directory(it.path))
        error_code err;
        switch (it.event) {
        case DirectoryChangeEvent::Created:
            if (filesystem::is_directory(it.path)) {
                filesystem::create_directories(changeBackupPath, err);
            } else {
                changeBackupPath += BACKUP_FILE_SUFFIX;
                filesystem::copy(it.path, changeBackupPath, filesystem::copy_options::update_existing, err);
            }

            if (err)
                changesToProcessLater.push_back(move(it));
            else
                m_db.insert({it.event, changeBackupPath});
            break;
        case DirectoryChangeEvent::Deleted:
            if (filesystem::is_directory(changeBackupPath))
                filesystem::remove_all(changeBackupPath, err);
            else {
                changeBackupPath += BACKUP_FILE_SUFFIX;
                filesystem::remove_all(changeBackupPath, err);
            }

            m_db.insert({it.event, changeBackupPath});
            break;
        case DirectoryChangeEvent::Modified:
            if (!filesystem::is_directory(it.path)) {
                changeBackupPath += BACKUP_FILE_SUFFIX;
                filesystem::copy(it.path, changeBackupPath, filesystem::copy_options::update_existing, err);

                if (err) //!!  err 32, The process cannot access the file because it is being used by another process.
                    changesToProcessLater.push_back(move(it));
                else
                    m_db.insert({it.event, changeBackupPath});
            }
            break;
        case DirectoryChangeEvent::MovedTo:
            if (!filesystem::is_directory(it.path))
                changeBackupPath += BACKUP_FILE_SUFFIX;

            if (it.path.filename().generic_string().rfind("delete_", 0) == 0) {
                filesystem::remove_all(lastMove, err);
                if (!err)
                    filesystem::remove_all(it.path, err);
                if (!err) {
                    m_db.insert({DirectoryChangeEvent::Deleted, lastMove});
                    m_db.insert({DirectoryChangeEvent::Deleted, it.path});
                }
            } else {
                filesystem::rename(lastMove, changeBackupPath, err);
            }

            if (err) { //!!  err 32, The process cannot access the file because it is being used by another process.
                changesToProcessLater.push_back({DirectoryChangeEvent::MovedFrom, lastMove});
                changesToProcessLater.push_back(move(it));
            } else {
                m_db.insert({DirectoryChangeEvent::MovedFrom, lastMove});
                m_db.insert({it.event, changeBackupPath});
            }
            detectedMoveStart = false;
            break;
        case DirectoryChangeEvent::MovedFrom:
            if (!filesystem::is_directory(changeBackupPath))
                changeBackupPath += BACKUP_FILE_SUFFIX;
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
