#include <iostream>
#include "can_tools.h"

using namespace esd_can_tools;
int main()
{

    uint8_t can_byte[can_bytes_length]{0};

    double physical_data = 800;

    CanDataAdapter adapter(can_byte, sizeof(can_byte), true);
    adapter.add(std::make_shared<CanDataTansformer>(20, 10, 0, 1023, true));
    adapter.add(std::make_shared<CanDataTansformer>(40, 10, 1, 1024, true));
    adapter.add(std::make_shared<CanDataTansformer>(56, 14, 0, 50, true));

    adapter.visualizeCanMatrix();
    adapter.assign(0, 500);
    adapter.assign(1, 1024);
    adapter.assign(2, 50);

    for (int i = can_bytes_length - 1; i >= 0; --i)
    {
        std::cout << std::bitset<8>(can_byte[i]) << " ";
    }
    std::cout << std::endl;

    adapter.fetch(0, physical_data);
    std::cout << physical_data << std::endl;

    adapter.fetch(1, physical_data);
    std::cout << physical_data << std::endl;

    adapter.fetch(2, physical_data);
    std::cout << physical_data << std::endl;

    return 0;
}