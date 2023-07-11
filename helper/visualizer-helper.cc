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

#include "ns3/visualizer-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("VisualizerHelper");

VisualizerHelper::VisualizerHelper ()
{
  m_factory.SetTypeId ("ns3::Visualizer");
}

VisualizerHelper::~VisualizerHelper ()
{
}

void
VisualizerHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void VisualizerHelper::SetSimulationTime(const ns3::Time seconds)
{
  m_simulationTime = seconds;
}

ApplicationContainer
VisualizerHelper::Install (Ptr<Node> node, Visualizer::DeviceType deviceType) const
{
  return ApplicationContainer (InstallPriv (node, deviceType));
}

ApplicationContainer
VisualizerHelper::Install (NodeContainer c, Visualizer::DeviceType deviceType) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i, deviceType));
    }
    apps.Stop(m_simulationTime- Seconds(0.000000001));
  return apps;
}

void Action(){

}

ApplicationContainer
VisualizerHelper::InstallPriv (Ptr<Node> node, Visualizer::DeviceType deviceType) const
{
  NS_LOG_FUNCTION (this << node);
ApplicationContainer apps;
  //Link the Visualizer to NetDevices
  for (uint32_t i = 0; i < node->GetNDevices(); i++){
      Ptr<Visualizer> app = m_factory.Create<Visualizer> ();

      app->SetNode (node);
      node->AddApplication (app);

      Ptr<NetDevice> netDevice = node->GetDevice(i);
      app->SetNetDevice(netDevice);
      app->SetDeviceType(deviceType);
      app->SetMobilityModel(node->GetObject<MobilityModel>());

      apps.Add(app);

  }

  return apps;
}
}
} // namespace ns3
