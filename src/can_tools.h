#ifndef ESD_CAN_TOOLS_H_
#define ESD_CAN_TOOLS_H_

#include <stdint.h>
#include <bitset>
#include <vector>
#include <memory>

namespace esd_can_tools
{
constexpr int can_bytes_length = 8;
typedef std::bitset<8 * can_bytes_length> BitMask;

class CanDataTansformer
{
public:
    typedef std::shared_ptr<CanDataTansformer> Ptr;
    typedef std::shared_ptr<const CanDataTansformer> ConstPtr;

    CanDataTansformer(int32_t start_bit, int32_t bit_length, double min_val, double max_val, bool is_big_endian = false);

    bool fromPhysicalData(const double src, uint8_t *dest, size_t len = can_bytes_length) const;

    bool fromByteData(double &dest, const uint8_t *src, size_t len = can_bytes_length) const;

    bool fetchMask(BitMask &mask) const;

    inline bool IsBigEndian() const
    {
        return _is_big_endian;
    }

private:
    int32_t _start_bit;
    int32_t _bit_length;
    int32_t _byte_covered_length;
    uint64_t _bit_mask;
    double _min_val, _max_val, _range;
    bool _is_big_endian;
}; // CanDataTansformer

class CanDataAdapter
{
public:
    typedef std::shared_ptr<CanDataAdapter> Ptr;
    typedef std::shared_ptr<const CanDataAdapter> ConstPtr;

    CanDataAdapter(uint8_t *can_data, const int8_t len = can_bytes_length, bool is_big_endian = false)
        : _can_data(can_data), _len(len), _is_big_endian(is_big_endian) {}

    inline std::bitset<8 * can_bytes_length> getMask() const { return _mask; }

    bool add(const CanDataTansformer::Ptr &transformer);

    bool assign(size_t i, const double val) const;

    bool fetch(size_t i, double &dest) const;

    inline bool IsEmpty() const
    {
        return _transformer_list.empty();
    }

    inline size_t size() const
    {
        return _transformer_list.size();
    }

    inline void clear()
    {
        _transformer_list.clear();
    }

    void visualizeCanMatrix() const;

private:
    uint8_t *_can_data;
    const int8_t _len;
    BitMask _mask = 0;
    bool _is_big_endian;
    std::vector<CanDataTansformer::Ptr> _transformer_list;
}; // class CanDataAdapter

} // namespace esd_can_tools

#endif // ESD_CAN_TOOLS_H_