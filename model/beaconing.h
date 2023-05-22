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

#ifndef BEACONING_H
#define BEACONING_H

#include "ns3/application.h"
#include "ns3/lora-net-device.h"
#include "ns3/nstime.h"
#include "ns3/attribute.h"

namespace ns3 {
namespace lorawan {

/**
 * This application sends a beacon from Gateway's NetDevices:
 * Gateway NetDevice -> LoraNetDevice.
 */
class Beaconing : public Application
{
public:
  Beaconing ();
  ~Beaconing ();

  static TypeId GetTypeId (void);

  /**
   * Sets the device to use to communicate with the EDs.
   *
   * \param loraNetDevice The LoraNetDevice on this node.
   */
  void SetLoraNetDevice (Ptr<LoraNetDevice> loraNetDevice);


  /**
   * Broadcasts the synchronous beacon defined for class B devices
   */
  void BroadcastBeacon();

  /**
   * Start the application
   */
  void StartApplication (void);

  /**
   * Stop the application
   */
  void StopApplication (void);

private:
  Ptr<LoraNetDevice> m_loraNetDevice; //!< Pointer to the node's LoraNetDevice
};

} //namespace ns3

}
#endif /* BEACONING */
