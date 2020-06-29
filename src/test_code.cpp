#include <iostream>
#include <unordered_map>
#include "can_tools.hpp"

using namespace esd_can_tools;
int main()
{

  uint32_t can_id1 = 0x34;
  uint32_t can_id2 = 0x33;

  // adpater for decoding data & encoding data
  auto rx_adapter = std::make_shared<CanDataAdapter>(can_id1, 2);
  auto tx_adapter = std::make_shared<CanDataAdapter>(can_id2, 2, true);

  // set params for rx_adpater
  rx_adapter->SetSignal(0, 0, 10, 0, 100);
  rx_adapter->SetSignal(1, 10, 20, -100, 200);
  rx_adapter->CheckStatus();
  rx_adapter->VisualizeCanMatrix();

  //set params for tx_adapter
  tx_adapter->SetSignal(0, 8, 16, -1, 1);
  tx_adapter->SetSignal(1, 16, 8, 100, 300);
  tx_adapter->CheckStatus();
  tx_adapter->VisualizeCanMatrix();

  //a wrapper for ntcan to simplify code
  ntCanWrapper can_handle(0);

  //init can handle
  if (!can_handle.Init())
    return 0;
  if (can_handle.Add(can_id1) == NTCAN_SUCCESS)
  {
    std::cout << "add success\n";
  }

  //init buffer
  ntCan::CMSG tx_buf;
  int tx_len = 1;
  ntCan::CMSG rx_buf;
  int rx_len = 1;
  std::vector<double> rx_val;

  //send data for 100 times
  int n = 100;
  while (n-- > 0)
  {
    if (can_handle.Read(&rx_buf, &rx_len) == NTCAN_SUCCESS && rx_adapter->Read(rx_buf, rx_val))
    {
      std::cout << "read data[" << rx_val[0] << "," << rx_val[1] << "]\n";
    }
    rx_len = 1;
  }

  //get data for 100 times
  n = 100;
  while (n-- > 0)
  {
    if (tx_adapter->Write(tx_buf, {0.5, 100}) && can_handle.Send(&tx_buf, &tx_len) == NTCAN_SUCCESS)
    {
      std::cout << "send success\n";
    }
    tx_len = 1;
  }

  can_handle.close();
  return 0;
}