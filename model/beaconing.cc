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

#include "class-b-end-device-lorawan-mac.h"
#include "gateway-lorawan-mac.h"

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

void
Beaconing::SetDeviceType(Beaconing::DeviceType deviceType)
{
  m_deviceType = deviceType;
}

Beaconing::DeviceType Beaconing::GetDeviceType()
{
  return m_deviceType;
}

void Beaconing::BroadcastBeacon()
{
  NS_LOG_FUNCTION(this);

  Ptr<GatewayLorawanMac> gwMac = m_loraNetDevice->GetMac()->GetObject<GatewayLorawanMac>();
  gwMac->SendBeacon();
  Simulator::Schedule(Beaconing::GetNextBeaconBroadcastTime(), &Beaconing::BroadcastBeacon, this);

}

void Beaconing::ScheduleEndDeviceBeaconReception(){
  NS_LOG_FUNCTION(this);
  Ptr<ClassBEndDeviceLorawanMac> classBMac = m_loraNetDevice->
                                             GetMac()->
                                             GetObject<ClassBEndDeviceLorawanMac>();
  classBMac->OpenBeaconReceiveWindow();
  Simulator::Schedule(Beaconing::GetNextBeaconBroadcastTime(), &Beaconing::ScheduleEndDeviceBeaconReception, this);
}

void
Beaconing::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  Time nextBroadcastTime = GetNextBeaconBroadcastTime();
  if(m_deviceType==DeviceType::GW){
      Simulator::Schedule(nextBroadcastTime, &Beaconing::BroadcastBeacon, this);
  }else if(m_deviceType==DeviceType::ED){
      Simulator::Schedule(nextBroadcastTime,&Beaconing::ScheduleEndDeviceBeaconReception,this);
  }

}

void
Beaconing::StopApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

}

Time Beaconing::GetNextBeaconBroadcastTime(){
  double currentTime = Simulator::Now().GetSeconds();
  double k = static_cast<double>(currentTime)/128;
  if(k == std::ceil(k)){
      k = k+1;
  }else {
      k = std::ceil(k);
  }
  double nextBT = (k*128)+0.0015;
  double secondsTillNextBT = nextBT - currentTime;
  NS_LOG_DEBUG("Seconds till next beacon broadcast: "<<secondsTillNextBT);
  return Seconds(secondsTillNextBT);
}



}
}
