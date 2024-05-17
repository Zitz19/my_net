#include "routing/map.h"

void RouteMap::PrintRouteMap()
{
    /* Should be calculated dynamicly */
    uint8_t cell_size = 3;

    /* cell_size * num_of_cells + headers + delimeters */
    uint64_t width = ls_table_.size() * cell_size + cell_size + 2 + ls_table_.size();

    if (ls_table_.empty())
    {
        std::cout << "ROUTE TABLE IS EMPTY NOW." << std::endl;
        return;
    }

    std::string header = std::string((width / 2 - 1), ' ') + "FROM" + std::string((width / 2 - 1), ' ') + "\n\n";
    header += "     "; /* left header */
    std::vector<TableKey> pids(ls_table_.size());
    auto key_selector = [](auto pair){ return pair.first; };
    std::transform(ls_table_.begin(), ls_table_.end(), pids.begin(), key_selector);
    std::sort(pids.begin(), pids.end());
    for (auto pid : pids)
    {
        std::string str_pid = std::to_string(pid);
        header += ' ' + std::string(cell_size - str_pid.size(), ' ') + str_pid;
    }
    header += "\n";
    std::cout << header;
    uint64_t line_cnt = 0, line_left_header = pids.size() / 2 - 1;
    for (auto pid_to : pids)
    {
        std::string line = "     " + std::string(width - 5 + 1, '-') + '\n';
        if (line_cnt == line_left_header)
        {
            line += "T ";
        } else if (line_cnt == line_left_header + 1)
        {
            line += "O ";
        } else
        {
            line += "  ";
        }
        std::string str_pid_to = std::to_string(pid_to);
        line += std::string(cell_size - str_pid_to.size(), ' ') + str_pid_to;
        for(auto pid_from : pids)
        {
            std::string str_cost = std::to_string(ls_table_[pid_from][pid_to]);
            line += '|' + std::string(cell_size - str_cost.size(), ' ') + str_cost;
        }
        line += "|\n";
        std::cout << line;
        line_cnt++;
    }
    std::cout << "     " + std::string(width - 5 + 1, '-') + '\n';
}

Packet RouteMap::UpdateLink(TableKey src, TableKey dst, PathCost cost)
{
    ls_table_[src][dst] = cost;
    ls_table_[dst][src] = cost;
    Packet rtm_upd_packet(dst, src, "", PacketFormat::RTM_UPD);
    rtm_upd_packet.SetPathCost(cost);
    return rtm_upd_packet;
}

uint64_t RouteMap::CalculatePathCostfromTime(posix_time::time_duration ping_time)
{
    return ping_time.total_milliseconds() / 2 + 1;
}