#ifndef ESD_CAN_TOOLS_HPP_
#define ESD_CAN_TOOLS_HPP_

#include "can_tools.h"
#include <iostream>
namespace esd_can_tools
{

template <class T_CMSG>
bool CanDataAdapter::Write(T_CMSG &can_msg, const std::vector<double> &val) const
{
    if (val.size() < _transformer_list.size())
    {
        return false;
    }
    for (int i = 0; i < _transformer_list.size(); ++i)
    {
        if (!_transformer_list[i])
        {
            std::cerr << "warn:CanDataAdapter::Write: "
                      << i + 1 << "th tranformer, invalid tranformer,abort\n";
            return false;
        }
        _transformer_list[i]->FromPhysicalData(val[i], can_msg.data, sizeof(can_msg.data));
    }
    can_msg.id = _can_id;
    can_msg.len = 0b111 & _byte_count;
    return true;
}

template <class T_CMSG>
bool CanDataAdapter::Read(const T_CMSG &can_msg, std::vector<double> &dest) const
{
    dest.resize(_transformer_list.size());
    if (can_msg.id != _can_id)
    {
        std::cerr << "warn:CanDataAdapter::Read: can_id is not match :" << std::hex
                  << _can_id << "!=" << can_msg.id << std::endl;
        return false;
    }
    for (int i = 0; i < _transformer_list.size(); ++i)
    {
        if (!_transformer_list[i])
        {
            std::cerr << "warn:CanDataAdapter::Read: "
                      << i + 1 << "th tranformer, invalid tranformer,abort\n";
            return false;
        }
        _transformer_list[i]->FromByteData(dest[i], can_msg.data, sizeof(can_msg.data));
    }
    return true;
}
} // namespace esd_can_tools

#endif //ESD_CAN_TOOLS_HPP_