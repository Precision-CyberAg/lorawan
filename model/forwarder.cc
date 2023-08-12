/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Padova
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
 * Author: Davide Magrin <magrinda@dei.unipd.it>
 */

#include "ns3/forwarder.h"
#include "ns3/log.h"
#include "ns3/lora-tag.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/wifi-phy.h"

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("Forwarder");

NS_OBJECT_ENSURE_REGISTERED (Forwarder);

TypeId
Forwarder::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Forwarder")
    .SetParent<Application> ()
    .AddConstructor<Forwarder> ()
    .SetGroupName ("lorawan");
  return tid;
}

Forwarder::Forwarder ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

Forwarder::~Forwarder ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
Forwarder::SetWifiNetDevice (Ptr<WifiNetDevice>
                                     wifiNetDevice)
{
  NS_LOG_FUNCTION (this << wifiNetDevice);

  m_wifiNetDevice = wifiNetDevice;
}

void
Forwarder::SetNsWifiNetDevice (Ptr<WifiNetDevice> nsWifiNetDevice){
  NS_LOG_FUNCTION (this << nsWifiNetDevice);

  m_nsWifiNetDevice = nsWifiNetDevice;
}

void
Forwarder::SetLoraNetDevice (Ptr<LoraNetDevice> loraNetDevice)
{
  NS_LOG_FUNCTION (this << loraNetDevice);

  m_loraNetDevice = loraNetDevice;
}

bool
Forwarder::ReceiveFromLora (Ptr<NetDevice> loraNetDevice, Ptr<const Packet>
                            packet, uint16_t protocol, const Address& sender)
{
  NS_LOG_FUNCTION (this << packet << protocol << sender);
  NS_LOG_DEBUG(m_nsWifiNetDevice->GetAddress());

  Ptr<Node> node = m_nsWifiNetDevice->GetNode();
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();

  NS_LOG_DEBUG(ipv4->GetAddress(ipv4->GetInterfaceForDevice(m_nsWifiNetDevice),0).GetAddress());


  Ptr<Packet> packetCopy = packet->Copy ();

  m_wifiNetDevice->Send (packetCopy,
                                 m_nsWifiNetDevice->GetAddress(),
                                 0x800);
  std::cout<<m_nsWifiNetDevice->GetAddress()<<std::endl;
  return true;
}

bool
Forwarder::ReceiveFromWifi (Ptr<NetDevice> wifiNetDevice,
                                    Ptr<const Packet> packet, uint16_t protocol,
                                    const Address& sender)
{
  NS_LOG_FUNCTION (this << packet << protocol << sender);

  Ptr<Packet> packetCopy = packet->Copy ();

  PacketTagIterator packetTagIterator = packetCopy -> GetPacketTagIterator();
  while(packetTagIterator.HasNext()){
    if(LoraTag::GetTypeId()==packetTagIterator.Next().GetTypeId()){
      m_loraNetDevice->Send (packetCopy);
      break;
    }
      

  }
  
  return true;
}

void
Forwarder::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  // TODO Make sure we are connected to both needed devices
}

void
Forwarder::StopApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  // TODO Get rid of callbacks
}

}
}
