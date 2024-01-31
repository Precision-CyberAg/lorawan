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

#include "ns3/class-b-end-device-lorawan-mac.h"

#include "lora-beacon-tag.h"

#include "ns3/end-device-lora-phy.h"
#include "ns3/end-device-lorawan-mac.h"
#include "ns3/log.h"

#include <algorithm>

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("ClassBEndDeviceLorawanMac");

NS_OBJECT_ENSURE_REGISTERED (ClassBEndDeviceLorawanMac);

TypeId
ClassBEndDeviceLorawanMac::GetTypeId (void)
{
static TypeId tid = TypeId ("ns3::ClassBEndDeviceLorawanMac")
  .SetParent<EndDeviceLorawanMac> ()
  .SetGroupName ("lorawan")
  .AddConstructor<ClassBEndDeviceLorawanMac> ();
return tid;
}

ClassBEndDeviceLorawanMac::ClassBEndDeviceLorawanMac () :
  // LoraWAN default
  m_receiveDelay1 (Seconds (1)),
  // LoraWAN default
  m_receiveDelay2 (Seconds (2)),
  m_rx1DrOffset (0)
{
  NS_LOG_FUNCTION (this);

  // Void the two receiveWindow events
  m_closeFirstWindow = EventId ();
  m_closeFirstWindow.Cancel ();
  m_closeSecondWindow = EventId ();
  m_closeSecondWindow.Cancel ();
  m_secondReceiveWindow = EventId ();
  m_secondReceiveWindow.Cancel ();
}

ClassBEndDeviceLorawanMac::~ClassBEndDeviceLorawanMac ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

/////////////////////
// Sending methods //
/////////////////////


void
ClassBEndDeviceLorawanMac::SendToPhy (Ptr<Packet> packetToSend)
{
  /////////////////////////////////////////////////////////
  // Add headers, prepare TX parameters and send the packet
  /////////////////////////////////////////////////////////

  NS_LOG_DEBUG ("PacketToSend: " << packetToSend);

  // Data Rate Adaptation as in LoRaWAN specification, V1.0.2 (2016)
  if (m_enableDRAdapt && (m_dataRate > 0)
      && (m_retxParams.retxLeft < m_maxNumbTx)
      && (m_retxParams.retxLeft % 2 == 0) )
    {
      m_txPower = 14; // Reset transmission power
      m_dataRate = m_dataRate - 1;
    }

  // Craft LoraTxParameters object
  LoraTxParameters params;
  params.sf = GetSfFromDataRate (m_dataRate);
  params.headerDisabled = m_headerDisabled;
  params.codingRate = m_codingRate;
  params.bandwidthHz = GetBandwidthFromDataRate (m_dataRate);
  params.nPreamble = m_nPreambleSymbols;
  params.crcEnabled = 1;
  params.lowDataRateOptimizationEnabled = LoraPhy::GetTSym (params) > MilliSeconds (16) ? true : false;

  // Wake up PHY layer and directly send the packet

  Ptr<LogicalLoraChannel> txChannel = GetChannelForTx ();

  NS_LOG_DEBUG ("PacketToSend: " << packetToSend);
  m_phy->Send (packetToSend, params, txChannel->GetFrequency (), m_txPower);

  //////////////////////////////////////////////
  // Register packet transmission for duty cycle
  //////////////////////////////////////////////

  // Compute packet duration
  Time duration = m_phy->GetOnAirTime (packetToSend, params);

  // Register the sent packet into the DutyCycleHelper
  m_channelHelper.AddEvent (duration, txChannel);

  //////////////////////////////
  // Prepare for the downlink //
  //////////////////////////////

  // Switch the PHY to the channel so that it will listen here for downlink
  m_phy->GetObject<EndDeviceLoraPhy> ()->SetFrequency (txChannel->GetFrequency ());
  tx_freq = txChannel->GetFrequency();

  // Instruct the PHY on the right Spreading Factor to listen for during the window
  // create a SetReplyDataRate function?
  uint8_t replyDataRate = GetFirstReceiveWindowDataRate ();
  NS_LOG_DEBUG ("m_dataRate: " << unsigned (m_dataRate) <<
                ", m_rx1DrOffset: " << unsigned (m_rx1DrOffset) <<
                ", replyDataRate: " << unsigned (replyDataRate) << ".");

  m_phy->GetObject<EndDeviceLoraPhy> ()->SetSpreadingFactor
    (GetSfFromDataRate (replyDataRate));

}

//////////////////////////
//  Receiving methods   //
//////////////////////////
void
ClassBEndDeviceLorawanMac::Receive (Ptr<Packet const> packet)
{
  NS_LOG_FUNCTION (this << packet);

  // Work on a copy of the packet
  Ptr<Packet> packetCopy = packet->Copy ();

  // Remove the Mac Header to get some information
  LorawanMacHeader mHdr;
  packetCopy->RemoveHeader (mHdr);

  NS_LOG_DEBUG ("Mac Header: " << mHdr);

  // Only keep analyzing the packet if it's downlink
  if (!mHdr.IsUplink ())
    {
      NS_LOG_INFO ("Found a downlink packet.");

      // Remove the Frame Header
      LoraFrameHeader fHdr;
      fHdr.SetAsDownlink ();
      packetCopy->RemoveHeader (fHdr);

      NS_LOG_DEBUG ("Frame Header: " << fHdr);

      // Determine whether this packet is for us
      bool messageForUs = (m_address == fHdr.GetAddress ());
      LoraDeviceAddress broadcastAddress;
      broadcastAddress.SetNwkID(0000000);
      broadcastAddress.SetNwkAddr(0000000000000000000000000);

      if (messageForUs)
        {
          NS_LOG_INFO ("The message is for us!");

          // If it exists, cancel the second receive window event
          // THIS WILL BE GetReceiveWindow()
          Simulator::Cancel (m_secondReceiveWindow);


          // Parse the MAC commands
          ParseCommands (fHdr);

          // TODO Pass the packet up to the NetDevice


          // Call the trace source
          m_receivedPacket (packet);
        }
        else if(fHdr.GetAddress() == broadcastAddress){
          NS_LOG_INFO ("The message is a broadcast");

          LoraBeaconTag beaconTag;
          if(packetCopy->PeekPacketTag(beaconTag)){
              //Found a beacon tag
              NS_LOG_DEBUG("Received a beacon packet with time: "<<beaconTag.GetTime());
              m_receivedPacket (packet);

              //Schedule Ping Slots
              //Calculate Ping Slot Start time
              double periodicity = 0.96 * std::pow(2,m_pingSlotPeriodicity);
              NS_LOG_DEBUG("Calculated PingSlot periodicity:"<<periodicity);
              double maxDelay = 128;
              m_pingSlotEvents.clear();
              for(int i = 1 ; i < maxDelay ; i++){
                    if(i*periodicity>128) {
                        break ;
                    }
                    EventId pingSlotEvent = Simulator::Schedule(Seconds(i*periodicity),&ClassBEndDeviceLorawanMac::OpenPingSlotReceiveWindow,this);
                    m_pingSlotEvents.push_back(pingSlotEvent);
              }
          }else{
              NS_LOG_DEBUG("Unspecified broadcast");
          }

        }
      else
        {
          NS_LOG_DEBUG ("The message is intended for another recipient.");

          // In this case, we are either receiving in the first receive window
          // and finishing reception inside the second one, or receiving a
          // packet in the second receive window and finding out, after the
          // fact, that the packet is not for us. In either case, if we no
          // longer have any retransmissions left, we declare failure.
          if (m_retxParams.waitingAck && m_secondReceiveWindow.IsExpired ())
            {
              if (m_retxParams.retxLeft == 0)
                {
                  uint8_t txs = m_maxNumbTx - (m_retxParams.retxLeft);
                  m_requiredTxCallback (txs, false, m_retxParams.firstAttempt, m_retxParams.packet);
                  NS_LOG_DEBUG ("Failure: no more retransmissions left. Used " << unsigned(txs) << " transmissions.");

                  // Reset retransmission parameters
                  resetRetransmissionParameters ();
                }
              else       // Reschedule
                {
                  this->Send (m_retxParams.packet);
                  NS_LOG_INFO ("We have " << unsigned(m_retxParams.retxLeft) << " retransmissions left: rescheduling transmission.");
                }
            }
        }
    }
  else if (m_retxParams.waitingAck && m_secondReceiveWindow.IsExpired ())
    {
      NS_LOG_INFO ("The packet we are receiving is in uplink.");
      if (m_retxParams.retxLeft > 0)
        {
          this->Send (m_retxParams.packet);
          NS_LOG_INFO ("We have " << unsigned(m_retxParams.retxLeft) << " retransmissions left: rescheduling transmission.");
        }
      else
        {
          uint8_t txs = m_maxNumbTx - (m_retxParams.retxLeft);
          m_requiredTxCallback (txs, false, m_retxParams.firstAttempt, m_retxParams.packet);
          NS_LOG_DEBUG ("Failure: no more retransmissions left. Used " << unsigned(txs) << " transmissions.");

          // Reset retransmission parameters
          resetRetransmissionParameters ();
        }
    }

  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToSleep ();
}

void
ClassBEndDeviceLorawanMac::FailedReception (Ptr<Packet const> packet)
{
  NS_LOG_FUNCTION (this << packet);

  // Switch to sleep after a failed reception
  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToSleep ();

  if (m_secondReceiveWindow.IsExpired () && m_retxParams.waitingAck)
    {
      if (m_retxParams.retxLeft > 0)
        {
          this->Send (m_retxParams.packet);
          NS_LOG_INFO ("We have " << unsigned(m_retxParams.retxLeft) << " retransmissions left: rescheduling transmission.");
        }
      else
        {
          uint8_t txs = m_maxNumbTx - (m_retxParams.retxLeft);
          m_requiredTxCallback (txs, false, m_retxParams.firstAttempt, m_retxParams.packet);
          NS_LOG_DEBUG ("Failure: no more retransmissions left. Used " << unsigned(txs) << " transmissions.");

          // Reset retransmission parameters
          resetRetransmissionParameters ();
        }
    }
}

void
ClassBEndDeviceLorawanMac::TxFinished (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Schedule the opening of the first receive window
  Simulator::Schedule (m_receiveDelay1,
                       &ClassBEndDeviceLorawanMac::OpenFirstReceiveWindow, this);

  // Schedule the opening of the second receive window
  m_secondReceiveWindow = Simulator::Schedule (m_receiveDelay2,
                                               &ClassBEndDeviceLorawanMac::OpenSecondReceiveWindow,
                                               this);
  // // Schedule the opening of the first receive window
  // Simulator::Schedule (m_receiveDelay1,
  //                      &ClassAEndDeviceLorawanMac::OpenFirstReceiveWindow, this);
  //
  // // Schedule the opening of the second receive window
  // m_secondReceiveWindow = Simulator::Schedule (m_receiveDelay2,
  //                                              &ClassAEndDeviceLorawanMac::OpenSecondReceiveWindow,
  //                                              this);

  // Switch the PHY to sleep
  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToSleep ();
}

void ClassBEndDeviceLorawanMac::OpenPingSlotReceiveWindow()
{
  NS_LOG_FUNCTION(this);

  // Set Phy in Standby mode
  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToStandby ();

  // Switch to appropriate channel and data rate
  NS_LOG_INFO ("Using parameters: " << m_pingSlotReceiveWindowFrequency << "Hz, DR"
                                   << unsigned(m_pingSlotReceiveWindowDataRate));

  m_phy->GetObject<EndDeviceLoraPhy> ()->SetFrequency
      (m_pingSlotReceiveWindowFrequency);
  m_phy->GetObject<EndDeviceLoraPhy> ()->SetSpreadingFactor (GetSfFromDataRate
                                                           (m_pingSlotReceiveWindowDataRate));

  uint8_t sfFromDataRate = GetSfFromDataRate (GetPingSlotReceiveWindowDataRate());
  NS_LOG_DEBUG("SF from data rate: "<< unsigned (sfFromDataRate));

  double bandwidthFromDataRate = GetBandwidthFromDataRate ( GetPingSlotReceiveWindowDataRate());
  NS_LOG_DEBUG("Bandwidth from data rate: "<<bandwidthFromDataRate);

  //Calculate the duration of a single symbol for the second receive window DR
  double tSym = pow (2, sfFromDataRate) / bandwidthFromDataRate;
  NS_LOG_DEBUG("Duration for a single symbol in second receive window: "<<tSym);

  // Schedule return to sleep after "at least the time required by the end
  // device's radio transceiver to effectively detect a downlink preamble"
  // (LoraWAN specification)
  const Time& pingSlotReceiveWindowDuration = Seconds((m_pingSlotReceiveWindowDurationInSymbols * tSym));
  NS_LOG_DEBUG("Duration for second receive window: "<<pingSlotReceiveWindowDuration.GetSeconds());
  m_closeBeaconReceiveWindow = Simulator::Schedule (pingSlotReceiveWindowDuration,
                                                   &ClassBEndDeviceLorawanMac::ClosePingSlotReceiveWindow, this);
}

void ClassBEndDeviceLorawanMac::ClosePingSlotReceiveWindow()
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<EndDeviceLoraPhy> phy = m_phy->GetObject<EndDeviceLoraPhy> ();

  // Check the Phy layer's state:
  // - RX -> We are receiving a preamble.
  // - STANDBY -> Nothing was received.
  // - SLEEP -> We have received a packet.
  // We should never be in TX or SLEEP mode at this point
  switch (phy->GetState ())
  {
  case EndDeviceLoraPhy::TX:
        break;
  case EndDeviceLoraPhy::RX:
        // PHY is receiving: let it finish. The Receive method will switch it back to SLEEP.
        break;
  case EndDeviceLoraPhy::SLEEP:
        // PHY has received, and the MAC's Receive already put the device to sleep
        break;
  case EndDeviceLoraPhy::STANDBY:
        // Turn PHY layer to SLEEP
        NS_LOG_DEBUG("Switching PHY to sleep");
        phy->SwitchToSleep ();
        break;
  }
}

void ClassBEndDeviceLorawanMac::OpenBeaconReceiveWindow()
{
  NS_LOG_FUNCTION(this);

  // Set Phy in Standby mode
  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToStandby ();

  // Switch to appropriate channel and data rate
  NS_LOG_INFO ("Using parameters: " << m_beaconReceiveWindowFrequency << "Hz, DR"
                                   << unsigned(m_beaconReceiveWindowFrequency));

  m_phy->GetObject<EndDeviceLoraPhy> ()->SetFrequency
      (m_beaconReceiveWindowFrequency);
  m_phy->GetObject<EndDeviceLoraPhy> ()->SetSpreadingFactor (GetSfFromDataRate
                                                           (m_beaconReceiveWindowDataRate));

  uint8_t sfFromDataRate = GetSfFromDataRate (GetBeaconReceiveWindowDataRate());
  NS_LOG_DEBUG("SF from data rate: "<< unsigned (sfFromDataRate));

  double bandwidthFromDataRate = GetBandwidthFromDataRate ( GetBeaconReceiveWindowDataRate());
  NS_LOG_DEBUG("Bandwidth from data rate: "<<bandwidthFromDataRate);

  //Calculate the duration of a single symbol for the second receive window DR
  double tSym = pow (2, sfFromDataRate) / bandwidthFromDataRate;
  NS_LOG_DEBUG("Duration for a single symbol in second receive window: "<<tSym);

  // Schedule return to sleep after "at least the time required by the end
  // device's radio transceiver to effectively detect a downlink preamble"
  // (LoraWAN specification)
  const Time& beaconReceiveWindowDuration = Seconds((m_beaconReceiveWindowDurationInSymbols * tSym));
  NS_LOG_DEBUG("Duration for second receive window: "<<beaconReceiveWindowDuration.GetSeconds());
  m_closeBeaconReceiveWindow = Simulator::Schedule (beaconReceiveWindowDuration,
                                            &ClassBEndDeviceLorawanMac::CloseBeaconReceiveWindow, this);
}

void ClassBEndDeviceLorawanMac::CloseBeaconReceiveWindow()
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<EndDeviceLoraPhy> phy = m_phy->GetObject<EndDeviceLoraPhy> ();

  // Check the Phy layer's state:
  // - RX -> We are receiving a preamble.
  // - STANDBY -> Nothing was received.
  // - SLEEP -> We have received a packet.
  // We should never be in TX or SLEEP mode at this point
  switch (phy->GetState ())
  {
  case EndDeviceLoraPhy::TX:
        break;
  case EndDeviceLoraPhy::RX:
        // PHY is receiving: let it finish. The Receive method will switch it back to SLEEP.
        break;
  case EndDeviceLoraPhy::SLEEP:
        // PHY has received, and the MAC's Receive already put the device to sleep
        break;
  case EndDeviceLoraPhy::STANDBY:
        // Turn PHY layer to SLEEP
        NS_LOG_DEBUG("Switching PHY to sleep");
        phy->SwitchToSleep ();
        break;
  }
}

void
ClassBEndDeviceLorawanMac::OpenFirstReceiveWindow (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Set Phy in Standby mode
  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToStandby ();

  uint8_t sfFromDataRate;

  double bandwidthFromDataRate;

  if(m_rx1_params_rx2_swap){
        // Switch to appropriate channel and data rate
        NS_LOG_INFO ("Using parameters: " << m_secondReceiveWindowFrequency << "Hz, DR"
                                         << unsigned(m_secondReceiveWindowDataRate));

        m_phy->GetObject<EndDeviceLoraPhy> ()->SetFrequency
            (m_secondReceiveWindowFrequency);
        m_phy->GetObject<EndDeviceLoraPhy> ()->SetSpreadingFactor (GetSfFromDataRate
                                                                 (m_secondReceiveWindowDataRate));

        sfFromDataRate= GetSfFromDataRate(GetSecondReceiveWindowDataRate());
        NS_LOG_DEBUG("SF from data rate: "<< unsigned (sfFromDataRate));
        bandwidthFromDataRate= GetBandwidthFromDataRate ( GetSecondReceiveWindowDataRate());
        NS_LOG_DEBUG("Bandwidth from data rate: "<<bandwidthFromDataRate);

  }else{
        // Switch the PHY to the channel so that it will listen here for downlink
        m_phy->GetObject<EndDeviceLoraPhy> ()->SetFrequency (tx_freq);

        // Instruct the PHY on the right Spreading Factor to listen for during the window
        // create a SetReplyDataRate function?
        uint8_t replyDataRate = GetFirstReceiveWindowDataRate ();
        NS_LOG_DEBUG ("m_dataRate: " << unsigned (m_dataRate) <<
                     ", m_rx1DrOffset: " << unsigned (m_rx1DrOffset) <<
                     ", replyDataRate: " << unsigned (replyDataRate) << ".");

        m_phy->GetObject<EndDeviceLoraPhy> ()->SetSpreadingFactor
            (GetSfFromDataRate (replyDataRate));

        sfFromDataRate= GetSfFromDataRate(GetFirstReceiveWindowDataRate());
        NS_LOG_DEBUG("SF from data rate: "<< unsigned (sfFromDataRate));
        bandwidthFromDataRate= GetBandwidthFromDataRate ( GetFirstReceiveWindowDataRate ());
        NS_LOG_DEBUG("Bandwidth from data rate: "<<bandwidthFromDataRate);

  }

  //Calculate the duration of a single symbol for the first receive window DR
  double tSym = pow (2, sfFromDataRate) / bandwidthFromDataRate;
  NS_LOG_DEBUG("Duration for a single symbol in first receive window: "<<tSym);

  // Schedule return to sleep after "at least the time required by the end
  // device's radio transceiver to effectively detect a downlink preamble"
  // (LoraWAN specification)
  const Time& firstReceiveWindowDuration = Seconds((m_receiveWindowDurationInSymbols * tSym));
  NS_LOG_DEBUG("Duration for first receive window: "<<firstReceiveWindowDuration.GetSeconds());
  m_closeFirstWindow = Simulator::Schedule (firstReceiveWindowDuration,
                                            &ClassBEndDeviceLorawanMac::CloseFirstReceiveWindow, this); //m_receiveWindowDuration

}

void
ClassBEndDeviceLorawanMac::CloseFirstReceiveWindow (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<EndDeviceLoraPhy> phy = m_phy->GetObject<EndDeviceLoraPhy> ();

  // Check the Phy layer's state:
  // - RX -> We are receiving a preamble.
  // - STANDBY -> Nothing was received.
  // - SLEEP -> We have received a packet.
  // We should never be in TX or SLEEP mode at this point
  switch (phy->GetState ())
    {
    case EndDeviceLoraPhy::TX:
      NS_ABORT_MSG ("PHY was in TX mode when attempting to " <<
                    "close a receive window.");
      break;
    case EndDeviceLoraPhy::RX:
      // PHY is receiving: let it finish. The Receive method will switch it back to SLEEP.
      break;
    case EndDeviceLoraPhy::SLEEP:
      // PHY has received, and the MAC's Receive already put the device to sleep
      break;
    case EndDeviceLoraPhy::STANDBY:
      // Turn PHY layer to SLEEP
      phy->SwitchToSleep ();
      break;
    }
}

void
ClassBEndDeviceLorawanMac::OpenSecondReceiveWindow (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Check for receiver status: if it's locked on a packet, don't open this
  // window at all.
  if (m_phy->GetObject<EndDeviceLoraPhy> ()->GetState () == EndDeviceLoraPhy::RX)
    {
      NS_LOG_INFO ("Won't open second receive window since we are in RX mode.");

      return;
    }

  // Set Phy in Standby mode
  m_phy->GetObject<EndDeviceLoraPhy> ()->SwitchToStandby ();


  uint8_t sfFromDataRate;

  double bandwidthFromDataRate;

  if(m_rx1_params_rx2_swap){
      // Switch the PHY to the channel so that it will listen here for downlink
      m_phy->GetObject<EndDeviceLoraPhy> ()->SetFrequency (tx_freq);

      // Instruct the PHY on the right Spreading Factor to listen for during the window
      // create a SetReplyDataRate function?
      uint8_t replyDataRate = GetFirstReceiveWindowDataRate ();
      NS_LOG_DEBUG ("m_dataRate: " << unsigned (m_dataRate) <<
                   ", m_rx1DrOffset: " << unsigned (m_rx1DrOffset) <<
                   ", replyDataRate: " << unsigned (replyDataRate) << ".");

      m_phy->GetObject<EndDeviceLoraPhy> ()->SetSpreadingFactor
          (GetSfFromDataRate (replyDataRate));

      sfFromDataRate= GetSfFromDataRate(GetFirstReceiveWindowDataRate());
      NS_LOG_DEBUG("SF from data rate: "<< unsigned (sfFromDataRate));
      bandwidthFromDataRate= GetBandwidthFromDataRate ( GetFirstReceiveWindowDataRate ());
      NS_LOG_DEBUG("Bandwidth from data rate: "<<bandwidthFromDataRate);
  }else{
      // Switch to appropriate channel and data rate
      NS_LOG_INFO ("Using parameters: " << m_secondReceiveWindowFrequency << "Hz, DR"
                                       << unsigned(m_secondReceiveWindowDataRate));

      m_phy->GetObject<EndDeviceLoraPhy> ()->SetFrequency
          (m_secondReceiveWindowFrequency);
      m_phy->GetObject<EndDeviceLoraPhy> ()->SetSpreadingFactor (GetSfFromDataRate
                                                               (m_secondReceiveWindowDataRate));
      sfFromDataRate= GetSfFromDataRate(GetSecondReceiveWindowDataRate());
      NS_LOG_DEBUG("SF from data rate: "<< unsigned (sfFromDataRate));
      bandwidthFromDataRate= GetBandwidthFromDataRate ( GetSecondReceiveWindowDataRate());
      NS_LOG_DEBUG("Bandwidth from data rate: "<<bandwidthFromDataRate);

  }
  //Calculate the duration of a single symbol for the second receive window DR
  double tSym = pow (2, sfFromDataRate) / bandwidthFromDataRate;
  NS_LOG_DEBUG("Duration for a single symbol in second receive window: "<<tSym);

  // Schedule return to sleep after "at least the time required by the end
  // device's radio transceiver to effectively detect a downlink preamble"
  // (LoraWAN specification)
  const Time& secondReceiveWindowDuration = Seconds((m_receiveWindowDurationInSymbols * tSym));
  NS_LOG_DEBUG("Duration for second receive window: "<<secondReceiveWindowDuration.GetSeconds());
  m_closeSecondWindow = Simulator::Schedule (secondReceiveWindowDuration,
                                             &ClassBEndDeviceLorawanMac::CloseSecondReceiveWindow, this);

}

void
ClassBEndDeviceLorawanMac::CloseSecondReceiveWindow (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<EndDeviceLoraPhy> phy = m_phy->GetObject<EndDeviceLoraPhy> ();

  // NS_ASSERT (phy->m_state != EndDeviceLoraPhy::TX &&
  // phy->m_state != EndDeviceLoraPhy::SLEEP);

  // Check the Phy layer's state:
  // - RX -> We have received a preamble.
  // - STANDBY -> Nothing was detected.
  switch (phy->GetState ())
    {
    case EndDeviceLoraPhy::TX:
      break;
    case EndDeviceLoraPhy::SLEEP:
      break;
    case EndDeviceLoraPhy::RX:
      // PHY is receiving: let it finish
      NS_LOG_DEBUG ("PHY is receiving: Receive will handle the result.");
      return;
    case EndDeviceLoraPhy::STANDBY:
      // Turn PHY layer to sleep
      phy->SwitchToSleep ();
      break;
    }

  if (m_retxParams.waitingAck)
    {
      NS_LOG_DEBUG ("No reception initiated by PHY: rescheduling transmission.");
      if (m_retxParams.retxLeft > 0 )
        {
          NS_LOG_INFO ("We have " << unsigned(m_retxParams.retxLeft) << " retransmissions left: rescheduling transmission.");
          this->Send (m_retxParams.packet);
        }

      else if (m_retxParams.retxLeft == 0 && m_phy->GetObject<EndDeviceLoraPhy> ()->GetState () != EndDeviceLoraPhy::RX)
        {
          uint8_t txs = m_maxNumbTx - (m_retxParams.retxLeft);
          m_requiredTxCallback (txs, false, m_retxParams.firstAttempt, m_retxParams.packet);
          NS_LOG_DEBUG ("Failure: no more retransmissions left. Used " << unsigned(txs) << " transmissions.");

          // Reset retransmission parameters
          resetRetransmissionParameters ();
        }

      else
        {
          NS_ABORT_MSG ("The number of retransmissions left is negative ! ");
        }
    }
  else
    {
      uint8_t txs = m_maxNumbTx - (m_retxParams.retxLeft );
      m_requiredTxCallback (txs, true, m_retxParams.firstAttempt, m_retxParams.packet);
      NS_LOG_INFO ("We have " << unsigned(m_retxParams.retxLeft) <<
                   " transmissions left. We were not transmitting confirmed messages.");

      // Reset retransmission parameters
      resetRetransmissionParameters ();
    }
}


/////////////////////////
// Getters and Setters //
/////////////////////////

Time
ClassBEndDeviceLorawanMac::GetNextClassTransmissionDelay (Time waitingTime)
{
  NS_LOG_FUNCTION_NOARGS ();

  // This is a new packet from APP; it can not be sent until the end of the
  // second receive window (if the second recieve window has not closed yet)
  if (!m_retxParams.waitingAck)
    {
      if (!m_closeFirstWindow.IsExpired () ||
          !m_closeSecondWindow.IsExpired () ||
          !m_secondReceiveWindow.IsExpired () )
        {
          NS_LOG_WARN ("Attempting to send when there are receive windows:" <<
                       " Transmission postponed.");
          // Compute the duration of a single symbol for the second receive window DR
          double tSym = pow (2, GetSfFromDataRate (GetSecondReceiveWindowDataRate ())) / GetBandwidthFromDataRate (GetSecondReceiveWindowDataRate ());
          // Compute the closing time of the second receive window
          Time endSecondRxWindow = Time(m_secondReceiveWindow.GetTs()) + Seconds (m_receiveWindowDurationInSymbols*tSym);

          NS_LOG_DEBUG("Duration until endSecondRxWindow for new transmission:" << (endSecondRxWindow - Simulator::Now()).GetSeconds());
          waitingTime = std::max (waitingTime, endSecondRxWindow - Simulator::Now());
        }
    }
  // This is a retransmitted packet, it can not be sent until the end of
  // ACK_TIMEOUT (this timer starts when the second receive window was open)
  else
    {
      double ack_timeout = m_uniformRV->GetValue (1,3);
      // Compute the duration until ACK_TIMEOUT (It may be a negative number, but it doesn't matter.)
      Time retransmitWaitingTime = Time(m_secondReceiveWindow.GetTs()) - Simulator::Now() + Seconds (ack_timeout);

      NS_LOG_DEBUG("ack_timeout:" << ack_timeout <<
                   " retransmitWaitingTime:" << retransmitWaitingTime.GetSeconds());
      waitingTime = std::max (waitingTime, retransmitWaitingTime);
    }

  return waitingTime;
}

uint8_t
ClassBEndDeviceLorawanMac::GetFirstReceiveWindowDataRate (void)
{
  return m_replyDataRateMatrix.at (m_dataRate).at (m_rx1DrOffset);
}

void
ClassBEndDeviceLorawanMac::SetSecondReceiveWindowDataRate (uint8_t dataRate)
{
  m_secondReceiveWindowDataRate = dataRate;
}

uint8_t
ClassBEndDeviceLorawanMac::GetSecondReceiveWindowDataRate (void)
{
  return m_secondReceiveWindowDataRate;
}

void
ClassBEndDeviceLorawanMac::SetSecondReceiveWindowFrequency (double frequencyMHz)
{
  m_secondReceiveWindowFrequency = frequencyMHz;
}

double
ClassBEndDeviceLorawanMac::GetSecondReceiveWindowFrequency (void)
{
  return m_secondReceiveWindowFrequency;
}

void ClassBEndDeviceLorawanMac::SetPingSlotReceiveWindowDataRate(uint8_t dataRate)
{
  m_pingSlotReceiveWindowDataRate = dataRate;
}

uint8_t ClassBEndDeviceLorawanMac::GetPingSlotReceiveWindowDataRate()
{
  return m_pingSlotReceiveWindowDataRate;
}

void ClassBEndDeviceLorawanMac::SetPingSlotReceiveWindowFrequency(double frequencyMHz)
{
  m_pingSlotReceiveWindowFrequency = frequencyMHz;
}

double ClassBEndDeviceLorawanMac::GetPingSlotReceiveWindowFrequency()
{
  return m_pingSlotReceiveWindowFrequency;
}

void ClassBEndDeviceLorawanMac::SetBeaconReceiveWindowFrequency(double frequencyMHz){
  m_beaconReceiveWindowFrequency = frequencyMHz;
}

double ClassBEndDeviceLorawanMac::GetBeaconReceiveWindowFrequency()
{
  return m_beaconReceiveWindowFrequency;
}

void ClassBEndDeviceLorawanMac::SetBeaconReceiveWindowDataRate(uint8_t dataRate){
  m_beaconReceiveWindowDataRate = dataRate;
}

uint8_t ClassBEndDeviceLorawanMac::GetBeaconReceiveWindowDataRate()
{
  return m_beaconReceiveWindowDataRate;
}

/////////////////////////
// MAC command methods //
/////////////////////////

void
ClassBEndDeviceLorawanMac::OnRxClassParamSetupReq (Ptr<RxParamSetupReq> rxParamSetupReq)
{
  NS_LOG_FUNCTION (this << rxParamSetupReq);

  bool offsetOk = true;
  bool dataRateOk = true;

  uint8_t rx1DrOffset = rxParamSetupReq->GetRx1DrOffset ();
  uint8_t rx2DataRate = rxParamSetupReq->GetRx2DataRate ();
  double frequency = rxParamSetupReq->GetFrequency ();

  NS_LOG_FUNCTION (this << unsigned (rx1DrOffset) << unsigned (rx2DataRate) <<
                   frequency);

  // Check that the desired offset is valid
  if ( !(0 <= rx1DrOffset && rx1DrOffset <= 5))
    {
      offsetOk = false;
    }

  // Check that the desired data rate is valid
  if (GetSfFromDataRate (rx2DataRate) == 0
      || GetBandwidthFromDataRate (rx2DataRate) == 0)
    {
      dataRateOk = false;
    }

  // For now, don't check for validity of frequency
  m_secondReceiveWindowDataRate = rx2DataRate;
  m_rx1DrOffset = rx1DrOffset;
  m_secondReceiveWindowFrequency = frequency;

  // Craft a RxParamSetupAns as response
  NS_LOG_INFO ("Adding RxParamSetupAns reply");
  m_macCommandList.push_back (CreateObject<RxParamSetupAns> (offsetOk,
                                                             dataRateOk, true));

}

} /* namespace lorawan */
} /* namespace ns3 */
