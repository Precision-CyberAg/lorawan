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

#include "ns3/beaconing.h"
#include "ns3/log.h"

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("Beaconing");

NS_OBJECT_ENSURE_REGISTERED (Beaconing);

TypeId
Beaconing::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Beaconing")
    .SetParent<Application> ()
    .AddConstructor<Beaconing> ()
    .SetGroupName ("lorawan");
  return tid;
}

Beaconing::Beaconing ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

Beaconing::~Beaconing ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
Beaconing::SetLoraNetDevice (Ptr<LoraNetDevice> loraNetDevice)
{
  NS_LOG_FUNCTION (this << loraNetDevice);

  m_loraNetDevice = loraNetDevice;
}

void Beaconing::BroadcastBeacon()
{
  NS_LOG_FUNCTION(this);

  Ptr<Packet> packet = Create<Packet>(10);
  LoraTag tag;
  packet->RemovePacketTag(tag);
  tag.SetDataRate(3);
  tag.SetFrequency(869.525);
  packet->AddPacketTag(tag);
  m_loraNetDevice->Send(packet);
  Simulator::Schedule(Beaconing::GetNextBeaconBroadcastTime(), &Beaconing::BroadcastBeacon, this);
}

void
Beaconing::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  Simulator::Schedule(Beaconing::GetNextBeaconBroadcastTime(), &Beaconing::BroadcastBeacon, this);

}

void
Beaconing::StopApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

}

Time Beaconing::GetNextBeaconBroadcastTime(){
  double currentTime = Simulator::Now().GetSeconds();
  double k = std::ceil(static_cast<double>(currentTime)/128);
  double nextBT = (k*128)+0.0015;
  double secondsTillNextBT = nextBT - currentTime;
  NS_LOG_DEBUG("Seconds till next beacon broadcast: "<<secondsTillNextBT);
  return Seconds(secondsTillNextBT);
}

}
}
