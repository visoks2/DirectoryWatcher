#include "App.h"
#include "CommandLineOptions.h"
#include <iostream>
constexpr auto *usage = "usage:\n"
                        "  MyApp.exe WATCH_DIR BACKUP_DIR \n"
                        "    Example:\n"
                        "      MyApp.exe ../workingDir ../_backup\n"
                        "  or \n"
                        "  MyApp.exe --showlog             print file changes log\n"
                        "    optional arguments: \n"
                        "      --path REGEX                REGEX to filter file path\n"
                        "      --dateFrom DATE             filter file changes by date\n"
                        "      --dateTo DATE\n"
                        "    Example:\n"
                        "      MyApp.exe --showlog --path '(([A-Z])\\w+)'\n"
                        "      MyApp.exe --showlog --path --dateFrom '2021-01-10'\n"
                        "      MyApp.exe --showlog --path '(([A-Z])\\w+)' --dateFrom '2021-01-10 20:40:00' --dateTo '2021-01-10 20:45:00'\n";
int main(int argc, char const *argv[])
{
    if (argc < 2) {
        std::cout << usage << std::endl;
        return -1;
    }

    CommandLineOptions option{
        .watchFolderPath{},
        .backupFolderPath{},
        .showLog = false,
        .showLog_pathRegex{},
        .showLog_dateFrom{},
        .showLog_dateTo{}};

    for (int i = 1; i < argc; i++) {
        std::string str{argv[i]};
        if ((str.compare("-h") == 0) || (str.compare("--help") == 0)) {
            std::cout << usage << std::endl;
            return 0;
        }
        if (str.compare("--showlog") == 0)
            option.showLog = true;
        else if ((str.compare("--path") == 0) && (argc >= (i + 1)))
            option.showLog_pathRegex = argv[i + 1];
        else if ((str.compare("--dateFrom") == 0) && (argc >= (i + 1)))
            option.showLog_dateFrom = argv[i + 1];
        else if ((str.compare("--dateTo") == 0) && (argc >= (i + 1)))
            option.showLog_dateTo = argv[i + 1];
    }

    if (!option.showLog) {
        if (argc != 3) {
            std::cout << usage << std::endl;
            return -1;
        }
        option.watchFolderPath = {argv[1]};
        option.backupFolderPath = {argv[2]};
        if (option.watchFolderPath.is_relative())
            option.watchFolderPath = std::filesystem::current_path() / option.watchFolderPath;
        if (option.backupFolderPath.is_relative())
            option.backupFolderPath = std::filesystem::current_path() / option.backupFolderPath;
    }

    try {
        App app{option};
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "unexpected error" << std::endl;
        return -1;
    }

    return 0;
}
