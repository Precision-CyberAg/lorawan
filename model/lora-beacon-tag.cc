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

#include "ns3/lora-beacon-tag.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3 {
namespace lorawan {

NS_OBJECT_ENSURE_REGISTERED (LoraBeaconTag);

TypeId
LoraBeaconTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoraBeaconTag")
    .SetParent<Tag> ()
    .SetGroupName ("lorawan")
    .AddConstructor<LoraBeaconTag> ()
  ;
  return tid;
}

TypeId
LoraBeaconTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

LoraBeaconTag::LoraBeaconTag (uint32_t m_time) :
  m_time(0)
{
}

LoraBeaconTag::~LoraBeaconTag ()
{
}

uint32_t
LoraBeaconTag::GetSerializedSize (void) const
{
  return 4;
}

void
LoraBeaconTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_time);
}

void
LoraBeaconTag::Deserialize (TagBuffer i)
{
  m_time = i.ReadU32();
}

void
LoraBeaconTag::Print (std::ostream &os) const
{
  os << m_time;
}

void
LoraBeaconTag::SetTime(uint32_t time)
{
  m_time = time;
}

uint32_t
LoraBeaconTag::GetTime()
{
  return m_time;
}

}
} // namespace ns3
