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

#ifndef VISUALIZER_HELPER_H
#define VISUALIZER_HELPER_H

#include "../model/visualizer.h"

#include "ns3/address.h"
#include "ns3/application-container.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

#include <stdint.h>
#include <string>

namespace ns3 {
namespace lorawan {

/**
 * This class can be used to install Visualizer applications on a set of
 * devices.
 */
class VisualizerHelper
{
public:
  VisualizerHelper ();

  ~VisualizerHelper ();

  void SetAttribute (std::string name, const AttributeValue &value);

  ApplicationContainer Install (NodeContainer c, Visualizer::DeviceType deviceType) const;

  ApplicationContainer Install (Ptr<Node> node, Visualizer::DeviceType deviceType) const;

  void SetSimulationTime(const Time seconds);

private:
  ApplicationContainer InstallPriv (Ptr<Node> node, Visualizer::DeviceType deviceType) const;

  ObjectFactory m_factory;

  Time m_simulationTime;

};

} // namespace ns3

}
#endif /* VISUALIZER_HELPER_H */
