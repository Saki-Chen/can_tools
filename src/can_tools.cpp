#include "can_tools.h"

#include <assert.h>
#include <cmath>
#include <vector>
#include <bitset>
#include <string>
#include <opencv2/opencv.hpp>
#include <iostream>

namespace esd_can_tools
{
CanDataTansformer::CanDataTansformer(int32_t start_bit, int32_t bit_length, double min_val, double max_val, bool is_big_endian)
    : _start_bit(start_bit), _bit_length(bit_length),
      _byte_covered_length((start_bit + bit_length - 1) / 8 - start_bit / 8 + 1),
      _bit_mask((1ll << bit_length) - 1ll),
      _min_val(min_val), _max_val(max_val),
      _range(max_val - min_val),
      _is_big_endian(is_big_endian)
{
}

bool CanDataTansformer::fromPhysicalData(const double src, uint8_t *dest, size_t len) const
{
    assert(dest && "fromPhysicalData:nullptr");
    if (src > _max_val || src < _min_val)
    {
        std::cerr << "fromPhysicalData: input data is out of range, abort\n";
        return false;
    }

    //convert & saturate & add offset
    auto bytes = static_cast<uint64_t>((src - _min_val) / _range * _bit_mask);
    bytes &= _bit_mask;
    bytes <<= _start_bit % 8;

    // reinterpret as uint8_t[]
    auto p = reinterpret_cast<const uint8_t *>(&bytes);

    int pos = _start_bit / 8;

    int step = _is_big_endian ? -1 : 1;

    int pos_end = pos + step * _byte_covered_length;

    if (pos_end < -1 || static_cast<size_t>(pos_end) > len)
    {
        std::cerr << "warn:fromPhysicalData:Physical data is truncated,abort\n";
        return false;
    }
    pos_end = std::max(pos_end, -1);
    pos_end = std::min(pos_end, static_cast<int>(len));

    for (int i = 0; pos != pos_end; pos += step)
    {
        dest[pos] |= p[i++];
    }
    return true;
}

bool CanDataTansformer::fromByteData(double &dest, const uint8_t *src, const size_t len) const
{
    assert(src && "fromByteData:nullptr");

    uint64_t bytes(0l);
    auto p = reinterpret_cast<uint8_t *>(&bytes);

    int pos = _start_bit / 8;

    int step = _is_big_endian ? -1 : 1;

    int pos_end = pos + step * _byte_covered_length;

    if (pos_end < -1 || static_cast<size_t>(pos_end) > len)
    {
        std::cerr << "warn:fromByteData:Physical data is truncated,abort\n";
        return false;
    }
    pos_end = std::max(pos_end, -1);
    pos_end = std::min(pos_end, static_cast<int>(len));

    for (int i = 0; pos != pos_end; pos += step)
    {
        p[i++] = src[pos];
    }

    bytes >>= _start_bit % 8;
    bytes &= _bit_mask;
    dest = 1.0 * bytes / _bit_mask * _range + _min_val;
    return true;
}

bool CanDataTansformer::fetchMask(BitMask &mask) const
{
    uint8_t bytes[can_bytes_length]{0};
    if (!fromPhysicalData(_max_val, bytes, can_bytes_length))
    {
        return false;
    }
    mask.reset();
    for (int i = can_bytes_length - 1; i >= 0; --i)
    {
        mask <<= 8;
        mask |= bytes[i];
    }
    return true;
}

bool CanDataAdapter::add(const CanDataTansformer::Ptr &transformer)
{
    if (_is_big_endian ^ transformer->IsBigEndian())
    {
        _transformer_list.emplace_back(nullptr);
        std::cerr << "warn:CanDataAdapter::add: "
                  << _transformer_list.size()
                  << "th tranformer, differen endian data,abort\n";
        return false;
    }

    BitMask mask;
    if (!transformer->fetchMask(mask))
    {
        _transformer_list.emplace_back(nullptr);
        std::cerr << "warn:CanDataAdapter::add: "
                  << _transformer_list.size()
                  << "th tranformer, bit map overflow,abort\n";
        return false;
    }
    if ((mask & _mask).any())
    {
        _transformer_list.emplace_back(nullptr);
        std::cerr << "warn:CanDataAdapter::add: "
                  << _transformer_list.size()
                  << "th tranformer, bit map conflict,abort\n";
        return false;
    }

    _transformer_list.emplace_back(transformer);
    _mask |= mask;
    return true;
}

bool CanDataAdapter::assign(size_t i, const double val) const
{
    if (i >= _transformer_list.size())
    {
        std::cerr << "warn:CanDataAdapter::assign: "
                  << i + 1 << "th tranformer, out of range,aort\n";
        return false;
    }
    if (!_transformer_list[i])
    {
        std::cerr << "warn:CanDataAdapter::assign: "
                  << i + 1 << "th tranformer, invalid tranformer,abort\n";
        return false;
    }
    _transformer_list[i]->fromPhysicalData(val, _can_data, _len);
    return true;

} // namespace esd_can_tools

bool CanDataAdapter::fetch(size_t i, double &dest) const
{
    if (i >= _transformer_list.size())
    {
        std::cerr << "warn:CanDataAdapter::fetch: "
                  << i + 1 << "th tranformer,out of range,abort\n";
        return false;
    }
    if (!_transformer_list[i])
    {
        std::cerr << "warn:CanDataAdapter::fetch: "
                  << i + 1 << "th tranformer, invalid tranformer,,abort\n";
        return false;
    }
    _transformer_list[i]->fromByteData(dest, _can_data, _len);
    return true;
}

void CanDataAdapter::visualizeCanMatrix() const
{
    cv::Mat visualMatrix = cv::Mat::zeros(cv::Size(8, can_bytes_length), CV_8UC1);
    BitMask bit_map;

    for (int i = 0; i < _transformer_list.size(); ++i)
    {
        if (_transformer_list[i])
        {
            if (!_transformer_list[i]->fetchMask(bit_map))
            {
                continue;
            }

            for (int j = 0; j < bit_map.size(); ++j)
            {
                if (bit_map.test(j))
                {
                    visualMatrix.at<uint8_t>(j / 8, 7 - j % 8) = i + 1;
                }
            }
        }
    }

    std::cout << visualMatrix << std::endl;
}

} // namespace esd_can_tools
