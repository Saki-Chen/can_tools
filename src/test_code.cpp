#include <iostream>
#include <unordered_map>
#include "can_tools.hpp"

using namespace esd_can_tools;
int main()
{

  // uint8_t can_byte[can_bytes_length]{0};

  // double physical_data = 800;

  // CanDataAdapter adapter(can_byte, sizeof(can_byte), true);
  // adapter.Add(std::make_shared<CanDataTansformer>(20, 10, 0, 1023, true));
  // adapter.Add(std::make_shared<CanDataTansformer>(40, 10, 1, 1024, true));
  // adapter.Add(std::make_shared<CanDataTansformer>(56, 14, 0, 50, true));

  // adapter.VisualizeCanMatrix();
  // adapter.Assign(0, 500);
  // adapter.Assign(1, 1024);
  // adapter.Assign(2, 50);

  // for (int i = can_bytes_length - 1; i >= 0; --i)
  // {
  //     std::cout << std::bitset<8>(can_byte[i]) << " ";
  // }
  // std::cout << std::endl;

  // adapter.Fetch(0, physical_data);
  // std::cout << physical_data << std::endl;

  // adapter.Fetch(1, physical_data);
  // std::cout << physical_data << std::endl;

  // adapter.Fetch(2, physical_data);
  // std::cout << physical_data << std::endl;
  typedef std::unordered_map<uint32_t, CanDataAdapter::Ptr> CanIdMap;

  uint32_t can_id1 = 0x34;
  uint32_t can_id2 = 0x33;

  CanIdMap id_map;
  id_map[can_id1] = std::make_shared<CanDataAdapter>(can_id1, 2);
  id_map[can_id2] = std::make_shared<CanDataAdapter>(can_id2, 2, true);

  id_map[can_id1]->SetSignal(0, 0, 10, 0, 100);
  id_map[can_id1]->SetSignal(1, 10, 20, -100, 200);
  id_map[can_id1]->CheckStatus();
  id_map[can_id1]->VisualizeCanMatrix();

  id_map[can_id2]->SetSignal(0, 8, 16, -1, 1);
  id_map[can_id2]->SetSignal(1, 16, 8, 100, 300);
  id_map[can_id2]->CheckStatus();
  id_map[can_id2]->VisualizeCanMatrix();

  ntCanWrapper can_handle(0);
  if (!can_handle.Init())
    return 0;
  if (can_handle.Add(can_id1) == NTCAN_SUCCESS)
  {
    std::cout << "add success\n";
  }

  ntCan::CMSG tx_buf;
  int tx_len = 1;
  ntCan::CMSG rx_buf;
  int rx_len = 1;
  int n = 100;
  while (n-- > 0)
  {
    if (id_map[can_id2]->Write(tx_buf, {0.5, 100}) && can_handle.Send(&tx_buf, &tx_len) == NTCAN_SUCCESS)
    {
      std::cout << "send success\n";
    }
    tx_len = 1;
  }
  std::vector<double> rx_val;

  n = 100;
  while (n-- > 0)
  {
    if (can_handle.Read(&rx_buf, &rx_len) == NTCAN_SUCCESS && id_map[can_id1]->Read(rx_buf, rx_val))
    {
      std::cout << "read data[" << rx_val[0] << "," << rx_val[1] << "]\n";
    }
    rx_len = 1;
  }
  return 0;
}