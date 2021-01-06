#include "fs/Watcher.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <string>
#include <thread>

int main(int argc, char const *argv[])
{
    using namespace std;
    if (argc != 2) {
        cout << "forgot argument :P" << endl;
        return -1;
    }
    const filesystem::path path{argv[1]};

    try {
        fs::Watcher watcher(path);
        while (true) {
            fs::Watcher::DirectoryChangeList dirChanges;
            watcher.getChanges(dirChanges);

            for (const auto &it : dirChanges) {

                cout << "changed: '" << it.path << "' ";

                if (filesystem::is_directory(it.path))
                    cout << "isDir ";
                if (it.eventModified)
                    cout << "modified ";
                if (it.eventDeleted)
                    cout << "deleted ";
                if (it.eventMovedTo)
                    cout << "moved_to ";
                if (it.eventMovedFrom)
                    cout << "moved_from ";
                if (it.eventCreated)
                    cout << "created ";

                cout << endl;
            }

            this_thread::sleep_for(1s);
        }
    } catch (const std::filesystem::filesystem_error &e) {
        cerr << e.path1() << " - " << e.path2() << '\n';
        cerr << e.what() << '\n';
        return -1;
    } catch (const exception &e) {
        cerr << e.what() << '\n';
        return -1;
    } catch (...) {
        cerr << "Sorry" << '\n';
        return -1;
    }
    return 0;
}
