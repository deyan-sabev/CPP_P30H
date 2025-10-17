#include <fstream>
#include <sstream>
#include <filesystem>

#include "Device.hpp"

namespace device
{
    /**
    * Функция, която извлича стойността на низ от даден JSON-форматиран текст.
    * Стойността се извлича по ключа, който е подаден като аргумент.
    * @param src String, съдържащ JSON съдържание.
    * @param key Ключът, за който да се извлече съответната стойност.
    * @return Стойността, свързана с дадения ключ, или празен низ, ако ключът не бъде намерен.
    */
    std::string extract_string(const std::string& src, const std::string& key)
    {
        std::string pattern = "\"" + key + "\""; // "key"
        size_t pos = src.find(pattern);
        if (pos == std::string::npos) return "";
        pos = src.find(':', pos); // "key" :
        if (pos == std::string::npos) return "";
        pos = src.find('"', pos); // "key" : "
        if (pos == std::string::npos) return "";
        size_t end = src.find('"', pos + 1); // "key" : "value"
        if (end == std::string::npos) return "";
        return src.substr(pos + 1, end - pos - 1); // value
    }

    /**
    * Функция, която извлича числовата стойност от даден JSON-форматиран текст.
    * Стойността се извлича по ключа, който е подаден като аргумент.
    * @param src String, съдържащ JSON съдържание.
    * @param key Ключът, за който да се извлече съответната стойност.
    * @return Числовата стойност, свързана с дадения ключ, или 0, ако ключът не бъде намерен.
    */
    int extract_int(const std::string& src, const std::string& key)
    {
        std::string pattern = "\"" + key + "\""; // "key"
        size_t pos = src.find(pattern);
        if (pos == std::string::npos) return 0;
        pos = src.find(':', pos); // "key" :
        if (pos == std::string::npos) return 0;
        std::string num_str;
        for (size_t i = pos + 1; i < src.size() && (isdigit(src[i]) || src[i] == ' '); ++i)
            if (isdigit(src[i])) num_str.push_back(src[i]);
        return std::stoi(num_str);
    }

    /**
    * Функция, която извлича данни за устройствата от конфигурационния JSON файл и ги записва в динамично създаден масив.
    * @param config_path Пътят до директорията, в която се намира конфигурационния файл.
    * @param json_name Името на JSON файла, който съдържа информация за устройствата.
    * @param device_count Променлива, в която се записва броят на намерените устройствата в конфигурационния файл.
    * @return Указател към новосъздаден масив от структури 'Device', съдържащ информация за устройствата. Трябва да се освободи паметта след използването на масива.
    * @throws std::runtime_error Ако файлът не може да бъде отворен или ако не се съдържа информация за устройства.
    */
    Device* load_devices(const std::string& config_path, const std::string& json_name, size_t& device_count)
    {
        std::string filename = (std::filesystem::path(config_path) / json_name).string();
        std::ifstream file(filename);
        if (!file.is_open()) throw std::runtime_error("Не може да се отвори файл: " + filename);

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        device_count = 0;
        for (char c : content) if (c == '{') ++device_count;
        if (device_count == 0) throw std::runtime_error("Не е открита информация за устройства в файла.");
        Device* devices = new Device[device_count];
        size_t pos = 0;
        size_t index = 0;
        while ((pos = content.find('{', pos)) != std::string::npos && index < device_count)
        {
            size_t end = content.find('}', pos);
            if (end == std::string::npos) break;

            std::string obj = content.substr(pos, end - pos + 1);

            devices[index].ip = extract_string(obj, "ip");
            devices[index].port = static_cast<uint16_t>(extract_int(obj, "port"));
            devices[index].device_id = extract_int(obj, "id");

            ++index;
            pos = end + 1;
        }
        return devices;
    }
};
