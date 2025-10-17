#include <string>
#include <atomic>

#include "Device.hpp"
#include "p30h_tcpReader.hpp"

namespace program
{
    /**
    * Struct, който описва позволените аргументи за програмата.
    * @param config_path Пътят към конфигурационния файл. По подразбиране стойност: "conf".
    * @param json_name Името на конфигурационния файл. По подразбиране стойност: "devices.json".
    * @param log_path Пътят към .csv файла/файловете. По подразбиране стойност: "log".
    * @param show_help Помощна променлива, която при стойност 'true' се извиква 'print_help()'. По подразбиране стойност: 'false'.
    */
    struct Args
    {
        std::string config_path = "conf";
        std::string json_name = "devices.json";
        std::string log_path = "log";
        bool show_help = false;
    };

    #ifdef _WIN32
    BOOL WINAPI console_ctrl_handler(DWORD signal);
    #else
    void signal_handler(int signal);
    #endif

    void print_help();
    Args* parse_args(int& argc, char**& argv);
    void poll_device(const device::Device& dev, const std::string& log_path);
    int run(int& argc, char**& argv);
};
