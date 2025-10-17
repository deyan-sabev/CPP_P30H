#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <iomanip>

#include "export_data.hpp"

namespace export_data
{
    /**
    * Функция за получаване на текущата дата и час.
    * @return string, който съдържа текущата локална дата и час.
    */
    std::string current_timestamp()
    {
        std::time_t now = std::time(nullptr);
        std::tm local_tm = *std::localtime(&now);
        std::ostringstream oss;
        oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    /**
    * Функция, която записва получените резултати от регистрите в .csv файл.
    * @param reader Устройството, от което ще се чете.
    * @param reg_map Масив, от който се извлича кои данни да бъдат прочетени от устройството.
    * @param reg_count Броя на елементите в масива reg_map.
    * @param stop_flag Флаг, с който се прекъсва функцията при необходимост.
    * @param log_path Пътят към .csv файла/файловете (без името на файла с неговото разширение).
    * @param interval Интервал от време, за който да се изчака преди да се направи нов запис в файла. По подразбиране е една секунда.
    * @param max_samples Максимален позволен брой записи. По подразбиране няма ограничение.
    */
    void poll_to_csv(P30HTcpReader& reader, reg::RegisterRead* reg_map, size_t reg_count, std::atomic<bool>* stop_flag, std::string_view log_path, float interval, size_t max_samples)
    {
        namespace fs = std::filesystem;
        fs::create_directories(log_path);

        time_t t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);
        std::ostringstream fname;
        fname << "P30H(" << reader.get_host() << ")_data_" << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ".csv";
        
        std::string csv_filename = (fs::path(log_path) / fname.str()).string();
        std::ofstream csv(csv_filename);
        if (!csv.is_open())
        {
            std::cerr << "Грешка при отварянето на файл: " << csv_filename << std::endl;
            return;
        }

        bool header_written = false;
        size_t count = 0;
        while (true)
        {
            if (stop_flag && stop_flag->load())
                break;

            std::string timestamp = current_timestamp();

            reg::RegisterResult* results = nullptr;
            try
            {
                results = reader.read_registers(reg_map, reg_count);
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Грешка по време на четене на регистрите: " << ex.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(interval * 1000)));
                continue;
            }

            if (!header_written)
            {
                csv << "timestamp";
                for (size_t i = 0; i < reg_count; ++i)
                    csv << "," << reg_map[i].symbol << " (" << reg_map[i].unit << ")";
                csv << "\n";
                header_written = true;
            }

            csv << timestamp;
            for (size_t i = 0; i < reg_count; ++i)
            {
                if (!results[i].valid)
                {
                    csv << ",";
                    continue;
                }
                if (reg_map[i].type == reg::REG_INT16)
                    csv << "," << results[i].value.val_int16;
                else if (reg_map[i].type == reg::REG_FLOAT32)
                    csv << "," << results[i].value.val_float32;
                else
                    csv << ",";
            }
            csv << std::endl;
            csv.flush();

            results = nullptr; // Няма нужда да се освобождава паметта. Вижте имплементацията на P30HTcpReader::read_registers.
            ++count;

            if (max_samples > 0 && count >= max_samples)
                break;

            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(interval * 1000)));
        }
        csv.close();
    }
};
