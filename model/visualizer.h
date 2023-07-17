/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2023 Dakota State University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Aditya Jagatha <aditya.jagatha@trojans.dsu.edu>
 */

#ifndef VISUALIZER_H
#define VISUALIZER_H

#include "ns3/application.h"
#include "ns3/lora-net-device.h"
#include "ns3/nstime.h"
#include "ns3/attribute.h"
#include "ns3/lora-tag.h"
#include "ns3/end-device-lora-phy.h"
#include "ctime"

namespace ns3 {
namespace lorawan {

/**
 * This application sends a beacon from Gateway's NetDevices:
 * Gateway NetDevice -> LoraNetDevice.
 */
class Visualizer : public Application
{
public:
  Visualizer ();
  ~Visualizer ();

  enum DeviceType{
      ED,
      GW,
      NS
  };

  DeviceType m_deviceType;

  static TypeId GetTypeId (void);

  /**
   * Sets the device to use to communicate with the EDs.
   *
   * \param loraNetDevice The LoraNetDevice on this node.
   */
  void SetNetDevice (Ptr<NetDevice> netDevice);

  void SetMobilityModel(Ptr<MobilityModel> mobilityModel);

  void SetDeviceType (DeviceType deviceType);
  std::string GetDeviceType(Visualizer::DeviceType deviceType);

  std::string GetDeviceState(EndDeviceLoraPhy::State state);

  std::string Vector3DToString(Vector3D vector3D);
  /**
   * Start the application
   */
  void StartApplication (void);

  /**
   * Stop the application
   */
  void StopApplication (void);

  void PhyTraceStartSending(Ptr<Packet const> packet, uint32_t t, double duration);

  void PhyTracePhyRxBegin(Ptr<Packet const> packet);

  void PhyTraceReceivedPacket(Ptr<Packet const> packet, uint32_t t);

  void PhyEndDeviceState(EndDeviceLoraPhy::State state1, EndDeviceLoraPhy::State state2);

  void MacTraceReceivedPacket(Ptr<Packet const> packet);

  void MobilityTraceCourseChange(Ptr<MobilityModel const> mobilityModel);

  void TxRxPointToPoint(Ptr<const Packet> packet, Ptr<NetDevice> sender, Ptr<NetDevice> receiver, Time duration, Time lastBitReceiveTime);

//  void EndDeviceLoraPhyTraceEndDeviceState(EndDeviceLoraPhy::State state);

private:
  Ptr<NetDevice> m_netDevice; //!< Pointer to the node's LoraNetDevice
  Ptr<MobilityModel> m_mobilityModel;
};

class FileManager{
private:
  std::mutex mutex;  // Mutex for synchronization
  std::ostringstream oss;
  bool fileWriteStatus;
  FileManager() {}

public:
  static FileManager& getInstance() {
      static FileManager instance;
      return instance;
  }

  void WriteToStream(const std::string data){
      std::lock_guard<std::mutex> lock(mutex);
      oss<<data;
  }


  void WriteToJSONStream(const std::unordered_map<std::string , std::string>& data, Time time){
      std::lock_guard<std::mutex> lock(mutex);
      int dataLen = data.size();

      if(!oss.str().empty()) {
          oss<<",";
      } else {
          oss<<"[";
      }

      oss<<"{\""<<time.GetSeconds()<<"\":{";
      for (const auto& pair : data) {
          const std::string& key = pair.first;
          oss<<"\""<<pair.first<<"\":";
          oss<<"\""<<pair.second<<"\"";
          dataLen--;
          if(dataLen!=0){
              oss<<",";
          }
          const std::string& value = pair.second;
          std::cout << "Key: " << key << ", Value: " << value << std::endl;
      }
      oss<<"}}";
  }

  void WriteToFile(){
      if(!fileWriteStatus){
          std::lock_guard<std::mutex> lock(mutex);  // Lock the mutex
          oss<<"]";
          std::ofstream outputFile("anim.json");
          if (outputFile.is_open()) {
              outputFile << oss.str() << std::endl;
              outputFile.close();
              fileWriteStatus = true;
          } else {
              std::cerr << "Failed to open the file!" << std::endl;
          }
      }
  }

  void WriteToFile2(){

      // Open the JSON file in read mode
      std::ifstream inputFile("data.json");
      if (!inputFile.is_open()) {
          std::cerr << "Failed to open input file!" << std::endl;

      }

      // Read the existing JSON data
      std::stringstream buffer;
      buffer << inputFile.rdbuf();
      inputFile.close();

      // Parse the JSON data
      std::string jsonData = buffer.str();

      // Append new data to the JSON object
      jsonData = jsonData.substr(0, jsonData.length() - 1);  // Remove the closing bracket of the existing JSON object
      jsonData += ", \"newKey\": \"newValue\"}";  // Append new key-value pair
      // You can append more data or modify the existing JSON structure as per your requirements

      // Open the JSON file in write mode
      std::ofstream outputFile("data.json");
      if (!outputFile.is_open()) {
          std::cerr << "Failed to open output file!" << std::endl;

      }

      // Write the updated JSON object to the file
      outputFile << jsonData;
      outputFile.close();

      std::cout << "JSON data appended to data.json" << std::endl;


  }
};
} //namespace ns3


}
#endif /* VISUALIZER */
