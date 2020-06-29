#include "can_tools.h"

#include <assert.h>
#include <cmath>
#include <vector>
#include <bitset>
#include <string>
#include <eigen3/Eigen/Core>
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

bool CanDataTansformer::FromPhysicalData(const double src, uint8_t *dest, int len) const
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

    if (pos_end < -1 || pos_end > len)
    {
        std::cerr << "warn:fromPhysicalData:Physical data is truncated,abort\n";
        return false;
    }
    pos_end = std::max(pos_end, -1);
    pos_end = std::min(pos_end, len);

    for (int i = 0; pos != pos_end; pos += step)
    {
        dest[pos] |= p[i++];
    }
    return true;
}

bool CanDataTansformer::FromByteData(double &dest, const uint8_t *src, const int len) const
{
    assert(src && "fromByteData:nullptr");

    uint64_t bytes(0l);
    auto p = reinterpret_cast<uint8_t *>(&bytes);

    int pos = _start_bit / 8;

    int step = _is_big_endian ? -1 : 1;

    int pos_end = pos + step * _byte_covered_length;

    if (pos_end < -1 || pos_end > len)
    {
        std::cerr << "warn:fromByteData:Physical data is truncated,abort\n";
        return false;
    }
    pos_end = std::max(pos_end, -1);
    pos_end = std::min(pos_end, len);

    for (int i = 0; pos != pos_end; pos += step)
    {
        p[i++] = src[pos];
    }

    bytes >>= _start_bit % 8;
    bytes &= _bit_mask;
    dest = 1.0 * bytes / _bit_mask * _range + _min_val;
    return true;
}

bool CanDataTansformer::FetchMask(BitMask &mask) const
{
    uint8_t bytes[can_bytes_length]{0};
    if (!FromPhysicalData(_max_val, bytes, can_bytes_length))
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

bool CanDataAdapter::SetSignal(const int i, const CanDataTansformer::Ptr &transformer)
{
    if (i < 0 || i >= _transformer_list.size())
    {
        std::cerr << "warn:CanDataAdapter::SetSignal: index out of range\n";
        return false;
    }

    if (_is_big_endian ^ transformer->IsBigEndian())
    {
        std::cerr << "warn:CanDataAdapter::SetSignal: "
                  << _transformer_list.size()
                  << "th tranformer, differen endian data,abort\n";
        return false;
    }

    BitMask mask;
    if (!transformer->FetchMask(mask))
    {
        std::cerr << "warn:CanDataAdapter::SetSignal: "
                  << _transformer_list.size()
                  << "th tranformer, bit map overflow,abort\n";
        return false;
    }
    if ((mask & _mask).any())
    {
        std::cerr << "warn:CanDataAdapter::SetSignal: "
                  << _transformer_list.size()
                  << "th tranformer, bit map conflict,abort\n";
        return false;
    }
    _byte_count = std::max(_byte_count, transformer->end_byte());
    _transformer_list[i] = transformer;
    _mask |= mask;
    return true;
}

void CanDataAdapter::VisualizeCanMatrix() const
{
    // cv::Mat visualMatrix = cv::Mat::zeros(cv::Size(8, can_bytes_length), CV_8UC1);
    // uint8_t visualMatrix[8 * can_bytes_length];
    Eigen::Matrix<int,8,can_bytes_length> visualMatrix;
    visualMatrix.setZero();
    BitMask bit_map;

    for (int i = 0; i < _transformer_list.size(); ++i)
    {
        if (_transformer_list[i])
        {
            if (!_transformer_list[i]->FetchMask(bit_map))
            {
                continue;
            }

            for (int j = 0; j < bit_map.size(); ++j)
            {
                if (bit_map.test(j))
                {
                    visualMatrix(j / 8, j % 8) = i + 1;
                }
            }
        }
    }
    std::cout << "\ncan id: 0x" << std::hex << _can_id << "\ncan matrix:\n" 
              << visualMatrix << std::endl;
}

bool CanDataAdapter::CheckStatus() const
{
    bool status = true;
    for (int i = 0; i < _transformer_list.size(); ++i)
    {
        if (nullptr == _transformer_list[i])
        {
            std::cerr << "signal " << i << " is not set yet, can id = " << std::hex << _can_id << std::endl;
            status = false;
        }
    }
    return status;
}

bool ntCanWrapper::SetBaudRate()
{
    uint32_t ret_baud = 0;
    if (ntCan::canGetBaudrate(_h, &ret_baud) != NTCAN_SUCCESS)
    {
        return false;
    }
    if (NTCAN_NO_BAUDRATE != ret_baud && _baud != ret_baud)
    {
        std::cerr << "warn:ntCanWrapper::SetBaudRate: setting differen baud("
                  << _baud << "!=" << ret_baud << ") for net: " << _net << std::endl
                  << "use the baud in use(" << ret_baud << ") instead\n";
        _baud = ret_baud;
    }
    if (ntCan::canSetBaudrate(_h, _baud) != NTCAN_SUCCESS)
    {
        return false;
    }
    return true;
}

bool ntCanWrapper::Init()
{
    if (_is_init)
    {
        std::cerr << "ntCanWrapper::Init: already inited \n";
        return false;
    }

    if (this->Open() != NTCAN_SUCCESS)
    {
        std::cerr << "ntCanWrapper::Init: open failed\n";
        return false;
    }

    if (!this->SetBaudRate())
    {
        std::cerr << "ntCanWrapper::Init: set baud rate failed\n";
        return false;
    }

    return _is_init = true;
}

ntCanWrapper::~ntCanWrapper()
{
    if (this->Close() != NTCAN_SUCCESS)
    {
        std::cerr << "error:ntCanWrapper::~ntCanWrapper:can not release can resource: handle:" << _h << std::endl;
    }
}

} // namespace esd_can_tools
