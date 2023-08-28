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

#include "ns3/visualizer.h"

#include "end-device-lorawan-mac.h"
#include "gateway-lorawan-mac.h"
#include "lora-frame-header.h"
#include "lorawan-mac-header.h"

#include "ns3/core-config.h"
#include "ns3/core-module.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/ppp-header.h"
#include "ns3/wifi-module.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include "ns3/internet-module.h"

namespace ns3
{
namespace lorawan
{

NS_LOG_COMPONENT_DEFINE("Visualizer");

NS_OBJECT_ENSURE_REGISTERED(Visualizer);

TypeId
Visualizer::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::Visualizer")
                            .SetParent<Application>()
                            .AddConstructor<Visualizer>()
                            .SetGroupName("lorawan");
    return tid;
}

Visualizer::Visualizer()
{
    NS_LOG_FUNCTION_NOARGS();
}

Visualizer::~Visualizer()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
Visualizer::SetNetDevice(Ptr<NetDevice> netDevice)
{
    NS_LOG_FUNCTION(this << netDevice);

    m_netDevice = netDevice;
}

void Visualizer::SetMobilityModel(Ptr<ns3::MobilityModel> mobilityModel)
{
    NS_LOG_FUNCTION_NOARGS();
    m_mobilityModel = mobilityModel;
}

void
Visualizer::SetDeviceType(ns3::lorawan::Visualizer::DeviceType deviceType)
{
    m_deviceType = deviceType;
}

void
Visualizer::StartApplication(void)
{
    NS_LOG_FUNCTION(this);
    if (Ptr<LoraNetDevice> loraNetDevice = m_netDevice->GetObject<LoraNetDevice>())
    {
        Ptr<LoraPhy> loraPhy = loraNetDevice->GetPhy();
        Ptr<LorawanMac> lorawanMac = loraNetDevice->GetMac();
        Ptr<LoraChannel> loraChannel = loraNetDevice->GetChannel()->GetObject<LoraChannel>();

        loraPhy->TraceConnectWithoutContext("StartSending",
                                            MakeCallback(&Visualizer::PhyTraceStartSending, this));

        loraPhy->TraceConnectWithoutContext("PhyRxBegin",
                                            MakeCallback(&Visualizer::PhyTracePhyRxBegin, this));

        loraPhy->TraceConnectWithoutContext(
            "ReceivedPacket",
            MakeCallback(&Visualizer::PhyTraceReceivedPacket, this));


        if (Ptr<EndDeviceLoraPhy> endDeviceLoraPhy = loraPhy->GetObject<EndDeviceLoraPhy>())
        {
            endDeviceLoraPhy->TraceConnectWithoutContext("EndDeviceState", MakeCallback(&Visualizer::PhyEndDeviceState, this));
        }
        else
        {
            // Handle for gws
        }


    }
    else if(Ptr<PointToPointNetDevice> p2pDev = m_netDevice->GetObject<PointToPointNetDevice>())
    {
        Ptr<PointToPointChannel> p2pChannel = p2pDev->GetChannel()->GetObject<PointToPointChannel>();
        p2pChannel->TraceConnectWithoutContext("TxRxPointToPoint", MakeCallback(&Visualizer::TxRxPointToPoint, this));
        NS_LOG_DEBUG("p2p dev device!");
    }
    else if(Ptr<WifiNetDevice> wifiDev = m_netDevice->GetObject<WifiNetDevice>() ){
        wifiDev -> GetPhy() -> TraceConnectWithoutContext("MonitorSnifferTx", MakeCallback(&Visualizer::WifiPhyTx, this));
        wifiDev -> GetPhy() -> TraceConnectWithoutContext("MonitorSnifferRx", MakeCallback(&Visualizer::WifiPhyRx, this));
//        wifiDev -> GetPhy() ->TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&Visualizer::WifiPhyRxEnd, this));
//        wifiDev -> GetMac() ->TraceConnectWithoutContext("",MakeCallback(&Visualizer::WifiPacketReceivedCallback, this));
//        Ptr<Ipv4L3Protocol> protocol = wifiDev -> GetNode() ->GetObject<Ipv4L3Protocol>();
//        protocol->TraceConnectWithoutContext("Tx", MakeCallback(&Visualizer::Ipv4TxTrace, this));
//        protocol->TraceConnectWithoutContext("Rx", MakeCallback(&Visualizer::Ipv4RxTrace, this));
        NS_LOG_FUNCTION_NOARGS();
    }
    else{
        NS_LOG_DEBUG("Unspecified net device!");
    }


    m_mobilityModel->TraceConnectWithoutContext("CourseChange", MakeCallback(&Visualizer::MobilityTraceCourseChange, this));
    MobilityTraceCourseChange(m_mobilityModel);

}
//
void
Visualizer::Ipv4TxTrace(Ptr<const Packet> p,
                                Ptr<Ipv4> ipv4,
                                uint32_t interfaceIndex)
{
    NS_LOG_FUNCTION_NOARGS();

}

void
Visualizer::Ipv4RxTrace(Ptr<const Packet> p,
                                Ptr<Ipv4> ipv4,
                                uint32_t interfaceIndex)
{
    NS_LOG_FUNCTION_NOARGS();

}
//
//Ptr<Node>
//Visualizer::GetNodeFromContext(const std::string& context) const
//{
//    // Use "NodeList/*/ as reference
//    // where element [1] is the Node Id
//
//    std::vector<std::string> elements = GetElementsFromContext(context);
//    Ptr<Node> n = NodeList::GetNode(std::stoi(elements.at(1)));
//    NS_ASSERT(n);
//
//    return n;
//}

void
Visualizer::StopApplication(void)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_LOG_DEBUG("Hello!");
    FileManager& fileManager = FileManager::getInstance();
    fileManager.WriteToFile();
}

void Visualizer::TxRxPointToPoint(Ptr<const ns3::Packet> packet, Ptr<ns3::NetDevice> sender, Ptr<ns3::NetDevice> receiver, ns3::Time duration, ns3::Time lastBitReceiveTime)
{
    NS_LOG_FUNCTION_NOARGS();
    FileManager& fileManager = FileManager::getInstance();
    fileManager.WriteToJSONStream({{"TraceType", "TxRxPointToPoint"},
                                   {"Sender", std::to_string(sender->GetNode()->GetId())},
                                   {"Receiver", std::to_string(receiver->GetNode()->GetId())},
                                   {"Duration", std::to_string(duration.GetMicroSeconds())},
                                   {"NodeId", std::to_string(m_netDevice->GetNode()->GetId())}}, Simulator::Now());

}

void Visualizer::WifiPacketReceivedCallback(Ptr<const ns3::Packet> packet, const ns3::WifiMacHeader& hdr, const ns3::WifiTxVector& txVector, ns3::MpduInfo aMpdu)
{
    NS_LOG_FUNCTION_NOARGS();

    Ptr<Packet> copy = packet->Copy();
    FileManager& fileManager = FileManager::getInstance();
    fileManager.WriteToJSONStream({{"TraceType", "WifiPhyPacketReceived"},
                                   //                                   {"Sender", std::to_string(m_netDevice->GetNode()->GetId())},
                                   //                                   {"Receiver", std::to_string(receiver->GetNode()->GetId())},
                                   //                                   {"Duration", std::to_string(duration.GetMicroSeconds())},
                                   {"PacketUid", std::to_string(copy->GetUid())},
                                   {"DeviceType", GetDeviceType(m_deviceType)},
                                   {"NodeId", std::to_string(m_netDevice->GetNode()->GetId())}}, Simulator::Now());

}

  void Visualizer::WifiPhyTx(Ptr<const Packet> packet, unsigned short val1, ns3::WifiTxVector val2,ns3::MpduInfo val3, unsigned short val4){
    NS_LOG_FUNCTION_NOARGS();

    Ptr<Packet> copy = packet->Copy();
    Ipv4Header ipv4Header;
    copy->RemoveHeader(ipv4Header);

    std::cout<<"oolala"<<std::endl;
    copy->Print(std::cout);
    NS_LOG_DEBUG(packet);

    Ipv4Address sourceAddress = ipv4Header.GetSource();
    Ipv4Address destinationAddress = ipv4Header.GetDestination();

    std::cout<<ipv4Header.GetSource()<<std::endl;
    std::cout<<ipv4Header.GetDestination()<<std::endl;
    NS_LOG_DEBUG("Source IPv4 Address: " << sourceAddress);
    NS_LOG_DEBUG("Destination IPv4 Address: " << destinationAddress);


    FileManager& fileManager = FileManager::getInstance();
    fileManager.WriteToJSONStream({{"TraceType", "WifiPhyTx"},
//                                   {"Sender", std::to_string(m_netDevice->GetNode()->GetId())},
//                                   {"Receiver", std::to_string(receiver->GetNode()->GetId())},
//                                   {"Duration", std::to_string(duration.GetMicroSeconds())},
                                   {"PacketUid", std::to_string(copy->GetUid())},
                                   {"DeviceType", GetDeviceType(m_deviceType)},
                                   {"NodeId", std::to_string(m_netDevice->GetNode()->GetId())}}, Simulator::Now());

}

  void Visualizer::WifiPhyRxEnd(Ptr<const Packet> packet){
    NS_LOG_FUNCTION(packet->GetUid());
    NS_LOG_FUNCTION(packet);
    Ptr<Packet> copy = packet->Copy();


    Ipv4Header ipv4Header;
    copy->PeekHeader(ipv4Header);

    Ipv4Address sourceAddress = ipv4Header.GetSource();
    Ipv4Address destinationAddress = ipv4Header.GetDestination();

    NS_LOG_INFO("Source IPv4 Address: " << sourceAddress);
    NS_LOG_INFO("Destination IPv4 Address: " << destinationAddress);

    // Headers must be removed in the order they're present.
    PppHeader pppHeader;
    copy->RemoveHeader(pppHeader);
    Ipv4Header ipHeader;
    copy->RemoveHeader(ipHeader);

    std::cout << "Source IP: ";
    ipHeader.GetSource().Print(std::cout);
    std::cout << std::endl;

    std::cout << "Destination IP: ";
    ipHeader.GetDestination().Print(std::cout);
    std::cout << std::endl;

    FileManager& fileManager = FileManager::getInstance();
    fileManager.WriteToJSONStream({{"TraceType", "WifiPhyRxEnd"},
                                   //                                   {"Sender", std::to_string(m_netDevice->GetNode()->GetId())},
                                   //                                   {"Receiver", std::to_string(receiver->GetNode()->GetId())},
                                   //                                   {"Duration", std::to_string(duration.GetMicroSeconds())},
                                   {"PacketUid", std::to_string(copy->GetUid())},
                                   {"DeviceType", GetDeviceType(m_deviceType)},
                                   {"NodeId", std::to_string(m_netDevice->GetNode()->GetId())}}, Simulator::Now());

  }

void Visualizer::WifiPhyRx(Ptr<const Packet> packet, unsigned short val1, ns3::WifiTxVector val2, ns3::MpduInfo val3, ns3::SignalNoiseDbm val4, unsigned short val5){
    NS_LOG_FUNCTION_NOARGS(); 

    Ptr<Packet> copy = packet->Copy();
    Ipv4Header ipv4Header;
    copy->PeekHeader(ipv4Header);

    Ipv4Address sourceAddress = ipv4Header.GetSource();
    Ipv4Address destinationAddress = ipv4Header.GetDestination();

    NS_LOG_INFO("Source IPv4 Address: " << sourceAddress);
    NS_LOG_INFO("Destination IPv4 Address: " << destinationAddress);

    FileManager& fileManager = FileManager::getInstance();
    fileManager.WriteToJSONStream({{"TraceType", "WifiPhyRx"},
                                   //                                   {"Sender", std::to_string(m_netDevice->GetNode()->GetId())},
                                   //                                   {"Receiver", std::to_string(receiver->GetNode()->GetId())},
                                   //                                   {"Duration", std::to_string(duration.GetMicroSeconds())},
                                   {"PacketUid", std::to_string(copy->GetUid())},
                                   {"DeviceType", GetDeviceType(m_deviceType)},
                                   {"NodeId", std::to_string(m_netDevice->GetNode()->GetId())}}, Simulator::Now());
}

void
Visualizer::PhyTraceStartSending(Ptr<const ns3::Packet> packet, uint32_t t, double duration)
{
    NS_LOG_FUNCTION_NOARGS();
    Ptr<Packet> packet2 = packet->Copy();
    LoraTag loraTag;
    packet->PeekPacketTag(loraTag);

    LorawanMacHeader macHeader;
    packet2->RemoveHeader(macHeader);

    LoraFrameHeader frameHeader;
    packet2->RemoveHeader(frameHeader);

    std::string devAddress;
    if(m_deviceType==ED){
        devAddress = m_netDevice->GetObject<LoraNetDevice>()
                         ->GetMac()
                         ->GetObject<EndDeviceLorawanMac>()
                         ->GetDeviceAddress()
                         .Print();
    }else if(m_deviceType==GW){
    }
    FileManager& fileManager = FileManager::getInstance();
    fileManager.WriteToJSONStream({{"TraceType", "PHYTraceStartSending"},
                                   {"NodeId", std::to_string(m_netDevice->GetNode()->GetId())},
                                   {"DeviceType", GetDeviceType(m_deviceType)},
                                   {"DeviceAddress", devAddress},
                                   {"FrameHeaderAddress", frameHeader.GetAddress().Print()},
                                   {"PacketUid", std::to_string(packet->GetUid())},
                                   {"Duration", std::to_string(duration)}
                                  },
                                  Simulator::Now());
}

void
Visualizer::PhyTracePhyRxBegin(Ptr<const ns3::Packet> packet)
{
    NS_LOG_FUNCTION_NOARGS();
}

void
Visualizer::PhyTraceReceivedPacket(Ptr<const ns3::Packet> packet, uint32_t t)
{
    NS_LOG_FUNCTION_NOARGS();
    Ptr<Packet> packet2 = packet->Copy();
    LoraTag loraTag;
    packet->PeekPacketTag(loraTag);

    LorawanMacHeader macHeader;
    packet2->RemoveHeader(macHeader);

    LoraFrameHeader frameHeader;
    packet2->RemoveHeader(frameHeader);

    std::string devAddress;
    if(m_deviceType==ED){
        devAddress = m_netDevice->GetObject<LoraNetDevice>()
            ->GetMac()
            ->GetObject<EndDeviceLorawanMac>()
            ->GetDeviceAddress()
            .Print();
    }else if(m_deviceType==GW){
    }
    FileManager& fileManager = FileManager::getInstance();
    fileManager.WriteToJSONStream({{"TraceType", "PHYTraceReceivedPacket"},
                                   {"NodeId", std::to_string(m_netDevice->GetNode()->GetId())},
                                   {"DeviceType", GetDeviceType(m_deviceType)},
                                   {"DeviceAddress", devAddress},
                                   {"FrameHeaderAddress", frameHeader.GetAddress().Print()},
                                   {"PacketUid", std::to_string(packet->GetUid())}
                                  },
                                  Simulator::Now());
}

void Visualizer::PhyEndDeviceState(EndDeviceLoraPhy::State state1, EndDeviceLoraPhy::State state2)
{
    NS_LOG_FUNCTION_NOARGS();
    FileManager& fileManager = FileManager::getInstance();
    fileManager.WriteToJSONStream({{"TraceType", "PhyEndDeviceState"},
                                   {"NodeId", std::to_string(m_netDevice->GetNode()->GetId())},
                                   {"DeviceType", GetDeviceType(m_deviceType)},
                                   {"DeviceState1", GetDeviceState(state1)},
                                   {"DeviceState2", GetDeviceState(state2)}
                                  },
                                  Simulator::Now());
}

void Visualizer::MacTraceReceivedPacket(Ptr<ns3::Packet const> packet)
{
    NS_LOG_FUNCTION_NOARGS();
    Ptr<Packet> packet2 = packet->Copy();
    LoraTag loraTag;
    packet->PeekPacketTag(loraTag);

    LorawanMacHeader macHeader;
    packet2->RemoveHeader(macHeader);

    LoraFrameHeader frameHeader;

    frameHeader.SetAsDownlink();


    packet2->RemoveHeader(frameHeader);
    std::string devAddress;
    if(m_deviceType==ED){
        devAddress = m_netDevice->GetObject<LoraNetDevice>()
                         ->GetMac()
                         ->GetObject<EndDeviceLorawanMac>()
                         ->GetDeviceAddress()
                         .Print();
    }else if(m_deviceType==GW){
    }
    FileManager& fileManager = FileManager::getInstance();
    fileManager.WriteToJSONStream({{"TraceType", "PHYTraceReceivedPacket"},
                                   {"NodeId", std::to_string(m_netDevice->GetNode()->GetId())},
                                   {"DeviceType", GetDeviceType(m_deviceType)},
                                   {"DeviceAddress", devAddress},
                                   {"FrameHeaderAddress", frameHeader.GetAddress().Print()},
                                   {"PacketUid", std::to_string(packet->GetUid())}
                                  },
                                  Simulator::Now());
}

void Visualizer::MobilityTraceCourseChange(Ptr<ns3::MobilityModel const> mobilityModel)
{
    NS_LOG_FUNCTION_NOARGS();
    std::string devAddress;
    if(m_deviceType==ED){
        devAddress = m_netDevice->GetObject<LoraNetDevice>()
                         ->GetMac()
                         ->GetObject<EndDeviceLorawanMac>()
                         ->GetDeviceAddress()
                         .Print();
    }else if(m_deviceType==GW){
    }
    FileManager& fileManager = FileManager::getInstance();
    fileManager.WriteToJSONStream({{"TraceType", "MobilityTraceCourseChange"},
                                   {"NodeId", std::to_string(m_netDevice->GetNode()->GetId())},
                                   {"DeviceType", GetDeviceType(m_deviceType)},
                                   {"Position", Vector3DToString(mobilityModel->GetPosition())},
                                   {"DeviceAddress", devAddress}
                                  },
                                  Simulator::Now());
}

std::string Visualizer::Vector3DToString(Vector3D vector3D){
    NS_LOG_FUNCTION_NOARGS();
    std::ostringstream oss;
    oss<< vector3D.x <<","<< vector3D.y <<","<< vector3D.z;
    return oss.str();
}


std::string Visualizer::GetDeviceType(Visualizer::DeviceType deviceType){
    NS_LOG_FUNCTION_NOARGS();
    switch (deviceType)
    {
    case Visualizer::ED:
        return "EndDevice";
    case Visualizer::GW:
        return "Gateway";
    case Visualizer::NS:
        return "NetworkServer";
    default:
        return "Unknown";
    }
}

std::string Visualizer::GetDeviceState(EndDeviceLoraPhy::State state){
    NS_LOG_FUNCTION_NOARGS();
    switch (state)
    {
    case EndDeviceLoraPhy::State::SLEEP:
        return "SLEEP";
    case EndDeviceLoraPhy::State::STANDBY:
        return "STANDBY";
    case EndDeviceLoraPhy::State::TX:
        return "TX";
    case EndDeviceLoraPhy::State::RX:
        return "RX";
    default:
        return "Unknown";
    }
}

} // namespace lorawan

} // namespace ns3
