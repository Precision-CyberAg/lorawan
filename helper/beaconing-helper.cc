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

#include "ns3/beaconing-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/beaconing.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("BeaconingHelper");

BeaconingHelper::BeaconingHelper ()
{
  m_factory.SetTypeId ("ns3::Beaconing");
}

BeaconingHelper::~BeaconingHelper ()
{
}

void
BeaconingHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
BeaconingHelper::SetDeviceType(Beaconing::DeviceType deviceType)
{
  m_deviceType = deviceType;
}

ApplicationContainer
BeaconingHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
BeaconingHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
BeaconingHelper::InstallPriv (Ptr<Node> node) const
{
  NS_LOG_FUNCTION (this << node);

  Ptr<Beaconing> app = m_factory.Create<Beaconing> ();

  app->SetDeviceType (m_deviceType);
  app->SetNode (node);
  node->AddApplication (app);

  //Link the Beaconing to NetDevices
  for (uint32_t i = 0; i < node->GetNDevices(); i++){
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice(i)->GetObject<LoraNetDevice>();
      if(loraNetDevice){
          app->SetLoraNetDevice(loraNetDevice);
      }

  }

  return app;
}
}
} // namespace ns3
