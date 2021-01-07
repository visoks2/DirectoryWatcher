#include "App.hpp"
#include "CommandLineOptions.h"

int main(int argc, char const *argv[])
{
    if (argc != 3) {
        std::cout << "usage: MyApp.exe ../watchDir ../backupDir" << std::endl;
        return -1;
    }
    CommandLineOptions option{
        .watchFolderPath{argv[1]},
        .backupFolderPath{argv[2]}};

    if (option.watchFolderPath.is_relative())
        option.watchFolderPath = std::filesystem::current_path() / option.watchFolderPath;
    if (option.backupFolderPath.is_relative())
        option.backupFolderPath = std::filesystem::current_path() / option.backupFolderPath;

    try {
        App app{option};
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Sorry" << std::endl;
        return -1;
    }

    return 0;
}
