#pragma once

#include <cstdint>
#include <unordered_map>

#define TableKey uint32_t
#define PathCost uint32_t

class RouteTable
{
private:
    std::unordered_map<TableKey, std::unordered_map<TableKey, PathCost>> ls_table_;

public:
    RouteTable() {}

    void AddLink(TableKey src, TableKey dst, PathCost cost);
};