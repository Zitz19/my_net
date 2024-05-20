#pragma once

#include <cstdint>
#include <unordered_map>
#include <algorithm>

#include <boost/date_time/posix_time/posix_time.hpp>
#include "packet/packet.h"

#define TableKey uint32_t
#define PathCost uint64_t

namespace posix_time = boost::posix_time;

class RouteMap
{
private:
    std::unordered_map<TableKey, std::unordered_map<TableKey, PathCost>> ls_table_;

public:
    RouteMap() {}
    std::unordered_map<TableKey, std::unordered_map<TableKey, PathCost>> &GetLinkStateTable() { return ls_table_; }

    void PrintRouteMap();

    Packet UpdateLink(TableKey src, TableKey dst, PathCost cost);
    PathCost GetPathCost(TableKey src, TableKey dst) { return ls_table_[src][dst]; };
    void RemovePID(TableKey pid);

    static uint64_t CalculatePathCostfromTime(posix_time::time_duration ping_time);
};