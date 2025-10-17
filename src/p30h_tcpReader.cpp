#include <stdexcept>
#include "p30h_tcpReader.hpp"

/**
* Клас за четене и запис на данни от Modbus slave TCP/IP устройство - P30H.
* @param host IP адреса на устройството.
* @param port Порт за връзка (по подразбиране 502).
* @param timeout Време за изчакване за свързване (по подразбиране 3 секунди).
* @param device_id Идентификатор на устройството (по подразбиране 1).
* @return Обект от класа P30HTcpReader.
*/
P30HTcpReader::P30HTcpReader(std::string host, uint16_t port, int id)
 : client(host, port)
 , _host(host)
 , _port(port)
 , _id(id)
 , _cached_results(nullptr)
 , _cached_count(0)
{
    client.modbus_set_slave_id(id);
}

P30HTcpReader::~P30HTcpReader()
{
    delete[] _cached_results;
}

/**
* Инициализира връзка с устройството.
* @return True при успех, False при неуспешен опит за свързване.
*/
bool P30HTcpReader::connect()
{
    return client.modbus_connect();
}

/**
 * Затваря връзката с устройството.
 */
void P30HTcpReader::close() const
{
    client.modbus_close();
}

/**
 * Функция за получаване на IP адреса на устройството.
 */
std::string P30HTcpReader::get_host() const
{
    return _host;
}

/**
 * Функция за получаване на информация на кой 'port' е устройството.
 */
uint16_t P30HTcpReader::get_port() const
{
    return _port;
}

/**
 * Функция за получаване на ID на устройството.
 */
int P30HTcpReader::get_slave_id() const
{
    return _id;
}

/**
* Прочита съдържанието на 16-битов регистър от даден адрес.
* @param address Адресът на регистъра.
* @return Стойността на регистъра.
*/
uint16_t P30HTcpReader::read_16bit(uint16_t address)
{
    uint16_t read_holding_regs[1]{0};
    client.modbus_read_holding_registers(address, 1, read_holding_regs);
    return read_holding_regs[0];
}

/**
* Прочита 32-битово число с плаваща запетая от един 32-битов регистър/два 16-битови регистъра.
* @param address Адрес на първия регистър.
* @param addr2 Адрес на втория регистър (ако не е зададен, се използва само address).
* @param lo_first Ако е True, редът на байтовете е обратен.
* @return Стойността на 32-битовото число с плаваща запетая.
*/
float P30HTcpReader::read_float32(uint16_t address, int16_t addr2, bool lo_first)
{
    uint16_t hi_reg{}, lo_reg{};
    if (addr2 < 0)
    {
        uint16_t read_holding_regs[2]{0, 0};
        client.modbus_read_holding_registers(address, 2, read_holding_regs);
        hi_reg = read_holding_regs[0];
        lo_reg = read_holding_regs[1];
    }
    else
    {
        uint16_t read_holding_reg1[1]{0}, read_holding_reg2[1]{0};
        client.modbus_read_holding_registers(address, 1, read_holding_reg1);
        client.modbus_read_holding_registers(addr2, 1, read_holding_reg2);
        hi_reg = read_holding_reg1[0];
        lo_reg = read_holding_reg2[0];
    }

    uint8_t bytes[4];
    if (lo_first)
    {
        // lo_reg (2 байта) се задава да бъде първи
        bytes[0] = static_cast<uint8_t>(lo_reg >> 8);
        bytes[1] = static_cast<uint8_t>(lo_reg & 0xFF);
        bytes[2] = static_cast<uint8_t>(hi_reg >> 8);
        bytes[3] = static_cast<uint8_t>(hi_reg & 0xFF);
    }
    else
    {
        bytes[0] = static_cast<uint8_t>(hi_reg >> 8);
        bytes[1] = static_cast<uint8_t>(hi_reg & 0xFF);
        bytes[2] = static_cast<uint8_t>(lo_reg >> 8);
        bytes[3] = static_cast<uint8_t>(lo_reg & 0xFF);
    }

    uint32_t as_int = (uint32_t)bytes[0] << 24
                    | (uint32_t)bytes[1] << 16
                    | (uint32_t)bytes[2] << 8
                    | (uint32_t)bytes[3];

    float value;
    // Проверка по време на компилация: уверяваме се, че float е 32 бита (съвпада с размера на as_int)
    static_assert(sizeof(value) == sizeof(as_int), "float не е 32 битa!");
    // Копиране на битовото представяне от as_int в променливата value
    // Използва се memcpy, за да се избегне проблеми със strict aliasing
    std::memcpy(&value, &as_int, sizeof(value));
    return value;
}

/**
* Прочита стойности от множество регистри.
* @param reg_map Списък с регистри и техните параметри. Задължителни параметри: "type", "address". Добре е да има и "name".
* @return Връща указател към масив от тип RegisterResult. Не изтривайте масива след използването му.
*/
reg::RegisterResult* P30HTcpReader::read_registers(reg::RegisterRead *reg_map, size_t reg_count)
{
    if (!_cached_results || _cached_count != reg_count)
    {
        delete[] _cached_results; // няма проблем дори и _cached_results да е nullptr.
        _cached_results = new reg::RegisterResult[reg_count];
        _cached_count = reg_count;
    }
    for (size_t i = 0; i < reg_count; ++i)
    {
        _cached_results[i].name = reg_map[i].name;
        _cached_results[i].valid = false;
        switch (reg_map[i].type)
        {
            case reg::REG_INT16:
                _cached_results[i].value.val_int16 = read_16bit(reg_map[i].address);
                _cached_results[i].valid = true;
                break;
            case reg::REG_FLOAT32:
                _cached_results[i].value.val_float32 = read_float32(reg_map[i].address, reg_map[i].addr2, reg_map[i].lo_first);
                _cached_results[i].valid = true;
                break;
            default:
                throw std::runtime_error("Непознат тип за " + reg_map[i].name);
        }
    }
    return _cached_results;
}

/**
* Записва 16-битово цяло число в даден регистър. Пример: value = 0b0101
* @param value Стойността за записване.
* @param address Адресът на регистъра.
*/
void P30HTcpReader::write_16bit(uint16_t value, uint16_t address)
{
    client.modbus_write_register(address, value);
}

/**
* Записва 32-битово число (с плаваща запетая) в един 32-битов регистър/два 16-битови регистъра. Ако се посочи само един адрес и той е на 16-битов регистър, стойността ще се запише на 'addr1' и 'addr1'+1.
* @param value Стойността за записване.
* @param address Адресът на първия регистър.
* @param addr2 Адресът на втория регистър (ако е -1, се използва само address).
* @param lo_first Ако е True, редът на байтовете е обратен.
*/
void P30HTcpReader::write_float32(float value, uint16_t address, int16_t addr2, bool lo_first)
{
    uint32_t raw;
    memcpy(&raw, &value, sizeof(raw)); // Преобразуване на float в 32-битово цяло число
    uint16_t high = static_cast<uint16_t>(raw >> 16), low = static_cast<uint16_t>(raw & 0xFFFF); // Разделяне на 32-битовото цяло число на двe 16-битови числа
    uint16_t byte_seq[2];
    if (lo_first)
    {
        byte_seq[0] = low;
        byte_seq[1] = high;
    }
    else
    {
        byte_seq[0] = high;
        byte_seq[1] = low;
    }
    if (addr2 < 0)
    {
        client.modbus_write_registers(address, 2, byte_seq);
    }
    else
    {
        client.modbus_write_register(address, byte_seq[0]);
        client.modbus_write_register(addr2, byte_seq[1]);
    }
}

/**
* Записва стойности в множество регистри.
* @param write_map Списък с регистри и техните стойности. Задължителни параметри: "type", "address", "value".
*/
void P30HTcpReader::write_registers(reg::RegisterWrite *write_map, size_t reg_count)
{
    for (size_t i = 0; i < reg_count; ++i)
    {
        switch (write_map[i].type)
        {
            case reg::REG_INT16:
                write_16bit(write_map[i].value.val_int16, write_map[i].address);
                break;
            case reg::REG_FLOAT32:
                write_float32(write_map[i].value.val_float32, write_map[i].address, write_map[i].addr2, write_map[i].lo_first);
                break;
            default:
                throw std::runtime_error("Непознат тип за " + write_map[i].name);
        }
    }
}
