#include "Watcher.h"

#include <exception>
#include <map>
#include <string>
namespace fs
{

Watcher::Watcher(const std::filesystem::path &path)
    : m_watchedDirectory(path)
    , m_hNotif{nullptr}
{
    if (path.empty() || !std::filesystem::is_directory(path))
        throw std::logic_error("invalid path");

    HANDLE hDir =
        CreateFileA(path.generic_string().c_str(),
                    FILE_LIST_DIRECTORY,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, // security attributes
                    OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS,
                    nullptr);
    if (hDir == INVALID_HANDLE_VALUE) {
        m_hNotif = nullptr;
        throw std::runtime_error("CreateFileA failed");
    }

    m_hNotif = hDir;
    m_watchThread = std::thread(&Watcher::thread_watch, this);
}

Watcher::~Watcher()
{
    if (m_hNotif) {
        CloseHandle(m_hNotif);
        m_hNotif = nullptr;
    }
}

void Watcher::getChanges(DirectoryChangeList &out_list)
{
    out_list.clear();
    if (!m_hNotif)
        throw std::logic_error("Watcher was not initialized");
    std::lock_guard<std::mutex> lock(m_directoryChangesMutex);
    std::swap(out_list, m_directoryChanges);
}

void Watcher::thread_watch()
{
    uint8_t buf[8 * 1024]; // TODO: no idea whats the best size ATM
    DWORD dwRead = 0;

    while (ReadDirectoryChangesW(
        m_hNotif,
        buf,
        sizeof(buf),
        true, // watch subtree
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_CREATION,
        &dwRead,
        nullptr,
        nullptr)) {

        size_t idx = 0;
        for (;;) {
            FILE_NOTIFY_INFORMATION *fni = reinterpret_cast<FILE_NOTIFY_INFORMATION *>(&buf[idx]);

            int reqLen = WideCharToMultiByte(CP_UTF8, 0, fni->FileName, fni->FileNameLength >> 1, nullptr, 0, nullptr, nullptr);
            std::vector<char> tmpBuf(reqLen);
            int actLen = WideCharToMultiByte(CP_UTF8, 0, fni->FileName, fni->FileNameLength >> 1, &tmpBuf[0], tmpBuf.size(), nullptr, nullptr);

            if (actLen <= 0)
                throw std::runtime_error("Error converting filename from WCHAR* to UTF8");

            const std::string filName(&tmpBuf[0], actLen);

            DirectoryChange newEntry;
            newEntry.path = m_watchedDirectory / filName;

            switch (fni->Action) {
            case FILE_ACTION_ADDED:
                newEntry.changeType = ChangeType::Created;
                break;
            case FILE_ACTION_REMOVED:
                newEntry.changeType = ChangeType::Deleted;
                break;
            case FILE_ACTION_MODIFIED:
                newEntry.changeType = ChangeType::Modified;
                break;
            case FILE_ACTION_RENAMED_OLD_NAME:
                newEntry.changeType = ChangeType::MovedFrom;
                break;
            case FILE_ACTION_RENAMED_NEW_NAME:
                newEntry.changeType = ChangeType::MovedTo;
                break;
            default:
                throw std::logic_error("unexpected FILE_NOTIFY_INFORMATION action");
            }
            std::lock_guard<std::mutex> lock(m_directoryChangesMutex);
            m_directoryChanges.push_back(newEntry);

            if (fni->NextEntryOffset > 0)
                idx += fni->NextEntryOffset;
            else
                break;
        }
    }
}
} // namespace fs
namespace std
{
std::string to_string(fs::Watcher::ChangeType t)
{
    static const std::map<fs::Watcher::ChangeType, const char *> nameMap{
        {fs::Watcher::ChangeType::Created, "Created"},
        {fs::Watcher::ChangeType::Deleted, "Deleted"},
        {fs::Watcher::ChangeType::Modified, "Modified"},
        {fs::Watcher::ChangeType::MovedTo, "MovedTo"},
        {fs::Watcher::ChangeType::MovedFrom, "MovedFrom"}};
    auto it = nameMap.find(t);
    return it == nameMap.end() ? "Unknown Value" : it->second;
}
} // namespace std
