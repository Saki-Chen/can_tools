#include "can_tools.hpp"
#include <iostream>
#include <sstream>

using namespace std;
using namespace esd_can_tools;
int main(int argc, char **argv)
{
    if (argc < 6)
    {
        cerr << "usage: echo_can_msg <can id> <start bit> <bit lenght> <min_val> <max_val> <optional is_bigendian 1 | 0>\n";
        exit(0);
    }

    uint32_t can_id;
    int32_t start_bit, bit_length;
    double min_val, max_val;
    bool is_bigendian = false;

    stringstream(argv[1]) >> hex >> can_id;
    stringstream(argv[2]) >> start_bit;
    stringstream(argv[3]) >> bit_length;
    stringstream(argv[4]) >> min_val;
    stringstream(argv[5]) >> max_val;
    if (argc > 6)
        stringstream(argv[6]) >> is_bigendian;

    auto rx_adapter = CanDataAdapter(can_id, 1, is_bigendian);
    rx_adapter.SetSignal(0, start_bit, bit_length, min_val, max_val);
    if (!rx_adapter.CheckStatus())
    {
        cerr << "params error\n";
        exit(0);
    }
    rx_adapter.VisualizeCanMatrix();

    //a wrapper for ntcan to simplify code
    ntCanWrapper can_handle(0);

    //init can handle
    if (!can_handle.Init())
        return 0;
    if (can_handle.Add(can_id) == NTCAN_SUCCESS)
    {
        std::cout << "add success\n";
    }

    //init buffer
    ntCan::CMSG rx_buf;
    int rx_len = 1;
    std::vector<double> rx_val;

    while (true)
    {
        if (can_handle.Read(&rx_buf, &rx_len) == NTCAN_SUCCESS && rx_adapter.Read(rx_buf, rx_val))
        {
            cout << can_id <<":receive data:" << rx_val[0] << endl;
        }
        rx_len = 1;
    }
    can_handle.close();
    exit(0);
}
