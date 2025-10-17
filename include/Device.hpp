#include <stdint.h>
#include <string>

namespace device
{
    /**
    * Struct, който описва параметрите на всяко устройство.
    * @param ip IP адреса на устройството.
    * @param port Порт за връзка (по подразбиране 502).
    * @param device_id Идентификатор на устройството (по подразбиране 1).
    */
    struct Device
    {
        std::string ip;
        uint16_t port = 502;
        int device_id = 1;
    };

    std::string extract_string(const std::string& src, const std::string& key);
    int extract_int(const std::string& src, const std::string& key);
    Device* load_devices(const std::string& config_path, const std::string& json_name, size_t& device_count);
};
