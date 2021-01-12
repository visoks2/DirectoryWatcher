#include "App.h"
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <string>
#include <thread>
#include <vector>

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

    recheckFiles();

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
        return a.changeType == b.changeType &&
               a.path == b.path;
    });
    changes.erase(a, changes.end());
}

void App::processChanges(fs::Watcher::DirectoryChangeList &changes, fs::Watcher::DirectoryChangeList &changesToProcessLater)
{
    using namespace std;

    changesToProcessLater.clear();
    for (const auto &it : changes) {
        filesystem::path backupPath{m_options.backupFolderPath / filesystem::relative(it.path, m_options.watchFolderPath)};
        backupPath = backupPath.lexically_normal();

        error_code err;
        switch (it.changeType) {
        case ChangeType::Created:
            if (filesystem::is_directory(it.path)) {
                filesystem::create_directories(backupPath, err);
            } else {
                backupPath += BACKUP_FILE_SUFFIX;
                filesystem::copy(it.path, backupPath, filesystem::copy_options::update_existing, err);
            }

            if (err)
                changesToProcessLater.push_back(move(it));
            else
                m_db.insert({it.changeType, backupPath});
            break;
        case ChangeType::Deleted:
            if (filesystem::is_directory(backupPath))
                filesystem::remove_all(backupPath, err); // most likely contains bug :) (will not log all files)
            else {
                backupPath += BACKUP_FILE_SUFFIX;
                filesystem::remove_all(backupPath, err); // most likely contains bug :) (will not log all files)
            }

            m_db.insert({it.changeType, backupPath});
            break;
        case ChangeType::Modified:
            if (!filesystem::is_directory(it.path)) {
                backupPath += BACKUP_FILE_SUFFIX;
                filesystem::copy(it.path, backupPath, filesystem::copy_options::update_existing, err);

                if (err) //!!  err 32, The process cannot access the file because it is being used by another process.
                    changesToProcessLater.push_back(move(it));
                else
                    m_db.insert({it.changeType, backupPath});
            }
            break;
        case ChangeType::MovedTo:
            if (!filesystem::is_directory(it.path))
                backupPath += BACKUP_FILE_SUFFIX;

            if (it.path.filename().generic_string().rfind("delete_", 0) == 0) {
                filesystem::remove_all(lastMove, err);
                if (!err)
                    filesystem::remove_all(it.path, err);
                if (!err) {
                    m_db.insert({ChangeType::Deleted, lastMove});
                    m_db.insert({ChangeType::Deleted, it.path});
                }
            } else {
                filesystem::rename(lastMove, backupPath, err);
            }

            if (err) { //!!  err 32, The process cannot access the file because it is being used by another process.
                changesToProcessLater.push_back({ChangeType::MovedFrom, lastMove});
                changesToProcessLater.push_back(move(it));
            } else {
                m_db.insert({ChangeType::MovedFrom, lastMove});
                m_db.insert({it.changeType, backupPath});
            }
            detectedMoveStart = false;
            break;
        case ChangeType::MovedFrom:
            if (!filesystem::is_directory(backupPath))
                backupPath += BACKUP_FILE_SUFFIX;
            lastMove = backupPath;
            detectedMoveStart = true;
            break;
        default:
            break;
        }
        if (err)
            cerr << err.message() << '\n';
    }
}

void App::recheckFiles()
{
    using namespace std;
    // remove deleted files
    vector<filesystem::path> deletedPaths;
    for (auto backupPath : filesystem::recursive_directory_iterator(m_options.backupFolderPath)) {
        filesystem::path path{m_options.watchFolderPath / filesystem::relative(backupPath, m_options.backupFolderPath)};
        if (!filesystem::is_directory(path)) {
            const auto pathStr = path.generic_string();
            path = filesystem::path{pathStr.substr(0, pathStr.size() - strlen(BACKUP_FILE_SUFFIX))};
        }
        if (!filesystem::exists(path)) {
            deletedPaths.push_back(backupPath);
        }
    }
    for (auto &deletedPath : deletedPaths) {
        error_code err;
        filesystem::remove_all(deletedPath, err);
        if (err)
            cerr << err.message() << '\n';
        else
            m_db.insert({ChangeType::Deleted, deletedPath});
    }
    // create/update modified files
    for (auto &path : filesystem::recursive_directory_iterator(m_options.watchFolderPath)) {
        filesystem::path backupPath{m_options.backupFolderPath / filesystem::relative(path, m_options.watchFolderPath)};
        backupPath = backupPath.lexically_normal();
        if (!filesystem::is_directory(path))
            backupPath += BACKUP_FILE_SUFFIX;

        error_code err;
        if (!filesystem::exists(backupPath)) {
            filesystem::copy(path, backupPath, filesystem::copy_options::update_existing, err);
            if (err)
                cerr << err.message() << '\n';
            else
                m_db.insert({ChangeType::Created, backupPath});
        } else if ((!filesystem::is_directory(path)) && (filesystem::file_size(path) != filesystem::file_size(backupPath))) {
            filesystem::copy(path, backupPath, filesystem::copy_options::update_existing, err);

            if (err)
                cerr << err.message() << '\n';
            else
                m_db.insert({ChangeType::Modified, backupPath});
        }
    }
}