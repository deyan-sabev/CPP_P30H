#include <iostream>
#include <locale>
#include <chrono>
#include <thread>
#include <future>
#include <csignal>

#include "program.hpp"
#include "p30h_registers.hpp"
#include "export_data.hpp"

namespace program
{
    /**
    * Променлива, която следи за възникнало събитие за прекратяване на програмата.
    */
    std::atomic<bool> stop_flag(false);

    #ifdef _WIN32
    /**
    * Функция, която използва Windows API за прихващане на събитие за прекъсване.
    * @param signal Променлива, с която се проверява за получен сигнал.
    * @return Дали е прихванат сигнал за прекъсване.
    */
    BOOL WINAPI console_ctrl_handler(DWORD signal)
    {
        if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT)
        {
            std::cout << "\n[Windows] Получен е сигнал за прекъсване. Програма се затвяря..." << std::endl;
            stop_flag.store(true);
            return TRUE;
        }
        return FALSE;
    }
    #else
    /**
    * Функция, която проверява за прекъсващо събитие.
    * @param signal Променлива, с която се проверява за получен сигнал.
    */
    void signal_handler(int signal)
    {
        if (signal == SIGINT)
        {
            std::cout << "\nПолучен е сигнал за прекъсване. Програма се затвяря..." << std::endl;
            stop_flag.store(true);
        }
    }
    #endif

    /**
    * Функция, която извежда на конзолата помощна информация за програмата.
    */
    void print_help()
    {
        std::cout << 
            "Прочитане и запис на данните в .csv файл от едно или няколко P30H, чрез използване на Modbus slave TCP/IP.\n\n"
            "Аргументи:\n"
            "  --config <path>   Пътят към конфигурационния файл (по подразбиране: conf)\n"
            "  --json <file>     Името на конфигурационния файл (по подразбиране: devices.json)\n"
            "  --log <path>      Пътят към .csv файла/файловете (по подразбиране: log)\n"
            "  -h, --help        Показва това съобщение\n\n"
            "Примери:\n"
            "  program.exe --config conf --json devices.json\n"
            "  program.exe --log log_folder\n"
            "  program.exe -h"
        << std::endl;
    }

    /**
    * Функция, която проверява за въведени аргументи към програмата.
    * @param argc Променлива, която съдържа броят на аргументите (стойността на променливата винаги е поне единица).
    * @param argv Променлива, която съдържа аргументите (името на програмата е първият аргумент).
    * @return Указател към новосъздадената структура в паметта. Трябва да се освободи паметта след използването на указателя.
    */
    Args* parse_args(int& argc, char**& argv)
    {
        Args* args = new Args;
        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--config" && i + 1 < argc)
            {
                args->config_path = argv[++i];
            }
            else if (arg == "--json" && i + 1 < argc)
            {
                args->json_name = argv[++i];
            }
            else if (arg == "--log" && i + 1 < argc)
            {
                args->log_path = argv[++i];
            }
            else if (arg == "-h" || arg == "--help")
            {
                args->show_help = true;
            }
            else
            {
                std::cerr << "\nНепознат аргумент: " << arg << '\n' << std::endl;
                args->show_help = true;
            }
        }
        return args;
    }

    /**
    * Функция, която за всяко устройство извиква функцията 'poll_to_csv'.
    * @param dev Конкретното устройство, от което ще се извличат данни.
    * @param log_path Пътят, на който да се запазват .csv файловете.
    */
    void poll_device(const device::Device& dev, const std::string& log_path)
    {
        P30HTcpReader reader(dev.ip, dev.port, dev.device_id);
        if (!reader.connect())
        {
            stop_flag.store(true);
            throw std::runtime_error("Неуспешна връзка с " + dev.ip + ":" + std::to_string(dev.port));
        }
        try
        {
            export_data::poll_to_csv(reader, reg::reg_map, reg::reg_count, &stop_flag, log_path);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("\nГрешка при " + dev.ip + ": " + e.what());
        }
        reader.close();
    }

    /**
    * Главната функция на програмата.
    * @param argc Променлива, която съдържа броят на аргументите (стойността на променливата винаги е поне единица).
    * @param argv Променлива, която съдържа аргументите (името на програмата е първият аргумент).
    * @return Код, с който програма е завършила изпълнение.
    */
    int run(int& argc, char**& argv)
    {
        std::setlocale(LC_ALL, "bg_BG.UTF-8");
    #ifdef _WIN32
        if (!SetConsoleOutputCP(CP_UTF8) || !SetConsoleCP(CP_UTF8)) std::cerr << "\n[Windows] Cannot set locale.\n" << std::endl;
        SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
    #else
        try
        {
            std::wcout.imbue(std::locale());
            std::cout.imbue(std::locale());
        }
        catch (const std::runtime_error& e)
        {
            std::cerr << "\nCannot set locale: " << e.what() << '\n' << std::endl;
            std::cout << "\nSetting default locale: C.UTF-8\n" << std::endl;
            std::locale::global(std::locale("C.UTF-8"));
            std::wcout.imbue(std::locale());
            std::cout.imbue(std::locale());
        }
        std::signal(SIGINT, signal_handler);
    #endif

        Args* args = parse_args(argc, argv);
        if (!args) throw std::runtime_error("Неуспешна инициализация на 'args'.");
        else if (args->show_help)
        {
            print_help();
            delete args;
            args = nullptr;
            return 0;
        }

        size_t device_count = 0;
        device::Device* devices = nullptr;
        try
        {
            devices = device::load_devices(args->config_path, args->json_name, device_count);
        }
        catch (const std::exception& e)
        {
            std::cerr << "\nГрешка при зареждане на данните на устройствата: " << e.what() << std::endl;
        }

        std::cout << "\nЗа свързване с устройствата може да отнеме до 20 секунди преди да се затвори програмата.\n" << std::endl;
        std::future<void>* futures = new std::future<void>[device_count];
        if (!futures) throw std::runtime_error("Неуспешна инициализация на нишките.");
        for (size_t i = 0; i < device_count; ++i)
        {
            futures[i] = std::async(std::launch::async, poll_device, devices[i], args->log_path);
        }
        while (!stop_flag.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        for (size_t i = 0; i < device_count; ++i)
        {
            try
            {
                futures[i].get();
            }
            catch (const std::exception &e)
            {
                std::cerr << "\n[Thread] Получена е грешка: " << e.what() << std::endl;
            }
        }
        delete[] devices;
        delete[] futures;
        delete args;
        devices = nullptr;
        futures = nullptr;
        args = nullptr;
        return 0;
    }
};
