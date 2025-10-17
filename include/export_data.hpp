#pragma once

#include <atomic>
#include "p30h_tcpReader.hpp"

namespace export_data
{
    std::string current_timestamp();
    void poll_to_csv(P30HTcpReader& reader, reg::RegisterRead* reg_map, size_t reg_count, std::atomic<bool>* stop_flag = nullptr, std::string_view log_path = "log", float interval = 1.0f, size_t max_samples = 0);
};
