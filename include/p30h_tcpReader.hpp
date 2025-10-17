#pragma once

#include "modbuspp/modbus.h"
#include "p30h_regTypeDef.hpp"

class P30HTcpReader
{
public:
    P30HTcpReader(std::string host, uint16_t port = 502, int id = 1);
    ~P30HTcpReader();

    std::string get_host() const;
    uint16_t get_port() const;
    int get_slave_id() const;

    bool connect();
    void close();

    uint16_t read_16bit(uint16_t address);
    float read_float32(uint16_t address, int16_t addr2 = -1, bool lo_first = false);
    reg::RegisterResult* read_registers(reg::RegisterRead *reg_map, size_t reg_count);
    
    void write_16bit(uint16_t value, uint16_t address);
    void write_float32(float value, uint16_t address, int16_t addr2 = -1, bool lo_first = false);
    void write_registers(reg::RegisterWrite *write_map, size_t reg_count);

private:
    modbus client;
    std::string _host;
    uint16_t _port;
    int _id;
    reg::RegisterResult* _cached_results;
    size_t _cached_count;
    
};
