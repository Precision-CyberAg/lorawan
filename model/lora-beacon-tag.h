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

#ifndef LORA_BEACON_TAG_H
#define LORA_BEACON_TAG_H

#include "ns3/tag.h"

namespace ns3 {
namespace lorawan {

/**
 * Tag used to save various data about a packet, like its Spreading Factor and
 * data about interference.
 */
class LoraBeaconTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  /**
   * Create a LoraTag with a given spreading factor and collision.
   *
   * \param sf The Spreading Factor.
   * \param destroyedBy The SF this tag's packet was destroyed by.
   */
  LoraBeaconTag (uint32_t m_time = 0);

  virtual ~LoraBeaconTag ();

  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual uint32_t GetSerializedSize () const;
  virtual void Print (std::ostream &os) const;

  uint32_t GetTime();

  void SetTime(uint32_t m_time);

private:
  uint32_t m_time; //!< Time in seconds
};
} // namespace ns3
}
#endif
