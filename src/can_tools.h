#ifndef ESD_CAN_TOOLS_H_
#define ESD_CAN_TOOLS_H_

#include <stdint.h>
#include <bitset>
#include <vector>
#include <memory>

namespace ntCan
{
extern "C"
{
#include <ntcan.h>
}
} // namespace ntCan

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

    bool FromPhysicalData(const double src, uint8_t *dest, int len = can_bytes_length) const;

    bool FromByteData(double &dest, const uint8_t *src, int len = can_bytes_length) const;

    bool FetchMask(BitMask &mask) const;

    inline bool IsBigEndian() const
    {
        return _is_big_endian;
    }

    inline uint32_t start_byte() const
    {
        return _is_big_endian ? _start_bit / 8 + 1 - _byte_covered_length : _start_bit / 8;
    }

    inline uint32_t end_byte() const
    {
        return _is_big_endian ? _start_bit / 8 : _start_bit / 8 + _byte_covered_length - 1;
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

    CanDataAdapter(int32_t can_id, int signal_count, bool is_big_endian = false)
        : _can_id(can_id), _is_big_endian(is_big_endian), _transformer_list(signal_count, nullptr) {}

    inline std::bitset<8 * can_bytes_length> getMask() const { return _mask; }

    bool SetSignal(const int i, const CanDataTansformer::Ptr &transformer);

    inline bool SetSignal(const int i, int32_t start_bit, int32_t bit_length, double min_val, double max_val)
    {
        return SetSignal(i, std::make_shared<CanDataTansformer>(start_bit, bit_length, min_val, max_val, _is_big_endian));
    }

    template <class T_CMSG>
    bool Write(T_CMSG &can_msg, const std::vector<double> &val) const;

    template <class T_CMSG>
    bool Read(const T_CMSG &can_msg, std::vector<double> &dest) const;

    bool CheckStatus() const;

    inline bool IsEmpty() const
    {
        return _transformer_list.empty();
    }

    inline size_t size() const
    {
        return _transformer_list.size();
    }

    void VisualizeCanMatrix() const;

private:
    BitMask _mask = 0;

    int32_t _can_id;
    bool _is_big_endian;
    uint32_t _byte_count = 0;
    std::vector<CanDataTansformer::Ptr> _transformer_list;
}; // class CanDataAdapter

class ntCanWrapper
{
public:
    ntCanWrapper(int net, uint32_t mode = 0, uint32_t baud = NTCAN_BAUD_500,
                 int32_t rx_queue_size = 8, int32_t rx_timeout_ms = 100,
                 int32_t tx_queue_size = 8, int32_t tx_timeout_ms = 100)
        : _net(net), _mode(mode), _baud(baud),
          _rxqueuesize(rx_queue_size),
          _rxtimeout(rx_timeout_ms),
          _txqueuesize(tx_queue_size), _txtimeout(tx_timeout_ms)
    {
    }

    bool Init();

    ~ntCanWrapper();

    inline ntCan::NTCAN_RESULT Add(int32_t can_id)
    {
        return ntCan::canIdAdd(_h, can_id);
    }

    inline ntCan::NTCAN_RESULT Del(int32_t can_id)
    {
        return ntCan::canIdDelete(_h, can_id);
    }

    inline ntCan::NTCAN_RESULT Take(ntCan::CMSG *cmsg, int32_t *len)
    {
        return ntCan::canTake(_h, cmsg, len);
    }

    inline ntCan::NTCAN_RESULT Read(ntCan::CMSG *cmsg, int32_t *len, ntCan::OVERLAPPED *ovrlppd = nullptr)
    {
        return ntCan::canRead(_h, cmsg, len, ovrlppd);
    }

    inline ntCan::NTCAN_RESULT Send(ntCan::CMSG *cmsg, int32_t *len)
    {
        return ntCan::canSend(_h, cmsg, len);
    }

    inline ntCan::NTCAN_RESULT Write(ntCan::CMSG *cmsg, int32_t *len, ntCan::OVERLAPPED *ovrlppd = nullptr)
    {
        return ntCan::canWrite(_h, cmsg, len, ovrlppd);
    }

    inline ntCan::NTCAN_RESULT Ioctl(uint32_t ulCmd, void *pArg)
    {
        return ntCan::canIoctl(_h, ulCmd, pArg);
    }

protected:
    bool SetBaudRate();

    inline ntCan::NTCAN_RESULT Open()
    {
        return ntCan::canOpen(_net, _mode, _txqueuesize, _rxqueuesize, _txtimeout, _rxtimeout, &_h);
    }

    inline ntCan::NTCAN_RESULT Close()
    {
        return ntCan::canClose(_h);
    }

private:
    /* logical net number (here: 0) */
    int _net;

    /* mode bits for canOpen */
    uint32_t _mode;

    /* baud rate */
    uint32_t _baud;

    /* maximum number of messages to receive */
    int32_t _rxqueuesize;

    /* timeout for receiving data in ms */
    int32_t _rxtimeout;

    /* maximum number of messages to transmit */
    int32_t _txqueuesize;

    /* timeout for transmit in ms */
    int32_t _txtimeout;

    /* can handle returned by canOpen() */
    ntCan::NTCAN_HANDLE _h;

    bool _is_init = false;

}; // class ntCanWrapper
} // namespace esd_can_tools

#endif // ESD_CAN_TOOLS_H_