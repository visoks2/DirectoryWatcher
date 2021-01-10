// https://docs.microsoft.com/en-us/windows/win32/fileio/obtaining-directory-change-notifications
// https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-readdirectorychangesw
// https://docs.microsoft.com/en-us/windows/win32/fileio/change-journals
// https://docs.microsoft.com/en-us/windows/win32/backup/backup
// https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-file_notify_information

#pragma once

#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>
#include <windows.h>

namespace fs
{
class Watcher
{
  public:
    enum class DirectoryChangeEvent : uint8_t {
        Created,
        Deleted,
        Modified,
        MovedTo,
        MovedFrom,
    };

    struct DirectoryChange {
        DirectoryChangeEvent event;
        std::filesystem::path path{};
    };

    using DirectoryChangeList = std::vector<DirectoryChange>;

    Watcher(const std::filesystem::path &path);
    virtual ~Watcher();
    void getChanges(DirectoryChangeList &out_list);

  private:
    void thread_watch();

    std::filesystem::path m_watchedDirectory;
    HANDLE m_hNotif;
    std::thread m_watchThread;
    DirectoryChangeList m_directoryChanges;
    mutable std::mutex m_directoryChangesMutex;
};

} // namespace fs
namespace std
{
std::string to_string(fs::Watcher::DirectoryChangeEvent t);
}
