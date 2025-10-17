#pragma once

#include <stdint.h>
#include <string>

namespace reg
{
    /**
    * Структура за изброяване на раличните видове регистри.
    * @param REG_INT16 16-битов регистър.
    * @param REG_FLOAT32 32-битов регистър или 2 16-битови регистри.
    * @param REG_UNKNOWN Непознат тип регистър.
    */
    typedef enum
    {
        REG_INT16,
        REG_FLOAT32,
        REG_UNKNOWN
    } RegType;

    /**
    * Структура с параметри за четене от регистър/регистри.
    * @param name Променлива от тип string, която указва името на величината.
    * @param symbol Променлива от тип string, която указва обозначението на величината.
    * @param unit Променлива от тип string, която указва мерната единица.
    * @param type Променлива от тип RegType (структура за изброяване на раличните видове регистри).
    * @param address Адрес на първия регистър от тип unsigned short.
    * @param addr2 Адрес на втория регистър от тип short.
    * @param lo_first Променлива от тип bool, която указва как да се запишат байтовете в регистрите.
    */
    typedef struct
    {
        std::string name;
        std::string symbol;
        std::string unit;
        RegType type;
        uint16_t address;
        int16_t addr2 = -1;
        bool lo_first = false;
    } RegisterRead;

    /**
    * Структура с параметри за запис в регистър/регистри.
    * @param name Променлива от тип string, която указва името на величината.
    * @param symbol Променлива от тип string, която указва обозначението на величината.
    * @param unit Променлива от тип string, която указва мерната единица.
    * @param type Променлива от тип RegType (структура за изброяване на раличните видове регистри).
    * @param value Променлива, с която се указва стойността, която да бъде записана в регистъра/регистрите.
    * @param address Адрес на първия регистър от тип unsigned short.
    * @param addr2 Адрес на втория регистър от тип short.
    * @param lo_first Променлива от тип bool, която указва как да се запишат байтовете в регистрите.
    */
    typedef struct
    {
        std::string name;
        std::string symbol;
        std::string unit;
        union
        {
            uint16_t val_int16;
            float val_float32;
        } value;
        RegType type;
        uint16_t address;
        int16_t addr2 = -1;
        bool lo_first = false;
    } RegisterWrite;

    /**
    * Структура с параметри за получаване на резултат при четете от регистър/регистри.
    * @param name Променлива от тип string.
    * @param value Променлива, която указва стойността в регистъра/регистрите.
    * @param valid Променлива от тип bool, която указва на всеки получен резултат дали е успешен.
    */
    typedef struct
    {
        std::string name;
        union
        {
            uint16_t val_int16;
            float val_float32;
        } value;
        bool valid;
    } RegisterResult;
};
