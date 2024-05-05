#include "routing/map.h"

void RouteTable::AddLink(TableKey src, TableKey dst, PathCost cost)
{
    ls_table_[src][dst] = cost;
}