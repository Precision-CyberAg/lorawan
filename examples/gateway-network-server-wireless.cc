

#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/log.h"
#include "ns3/lorawan-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/point-to-point-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/yans-wifi-helper.h"


using namespace ns3;
using namespace lorawan;

void
PacketReceivedGW(Ptr<const Packet> packet, const Address& from)
{
    std::cout << "Packet received at GW: " << packet << " " << Simulator::Now().GetSeconds()
              << std::endl;
}

void
PacketReceivedNS(Ptr<const Packet> packet, const Address& from)
{
    std::cout << "Packet received at NS: " << Simulator::Now().GetSeconds() << std::endl;
}

void
SendPacket(Ptr<Socket> socket, Ipv4Address remoteAddress, uint16_t remotePort)
{
    Ptr<Packet> packet = Create<Packet>(1024);
    socket->SendTo(packet, 0, InetSocketAddress(remoteAddress, remotePort));
    Simulator::Schedule(Seconds(1.0),
                        &SendPacket,
                        socket,
                        remoteAddress,
                        remotePort); // Schedule the next packet sending after 1 second
}


bool ReceiveFromLora (Ptr<NetDevice> loraNetDevice, Ptr<const Packet>
                                                             packet, uint16_t protocol, const Address& sender)
{
    std::cout<<"Received From Lora";
    return true;
}


int
main(int argc, char* argv[])
{

    Time simulationTime = Seconds(600);
    uint32_t gwSsize = 4;
    uint32_t nsSsize = 1;

    // NetworkServer
    NodeContainer networkServers;
    NetDeviceContainer nsDevices;
    Ipv4InterfaceContainer nsInterfaces;

    networkServers.Create(1);
    for (uint32_t i = 0; i < nsSsize; ++i)
    {
        std::ostringstream os;
        os << "ns node-" << i;
        Names::Add(os.str(), networkServers.Get(i));
    }

    MobilityHelper nsMobility;

    Ptr<ListPositionAllocator> nsAllocator = CreateObject<ListPositionAllocator>();
    nsAllocator->Add(Vector(0.0, 0.0, 0.0));
    nsMobility.SetPositionAllocator(nsAllocator);
    nsMobility.Install(networkServers);

    // Gateways
    NodeContainer gateways;
    NetDeviceContainer gwDevices;
    Ipv4InterfaceContainer gwInterfaces;

    gateways.Create(gwSsize);
    for (uint32_t i = 0; i < gwSsize; ++i)
    {
        std::ostringstream os;
        os << "gateway node-" << i;
        Names::Add(os.str(), gateways.Get(i));
    }

    MobilityHelper gatewayMobility;

    Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator>();
    allocator->Add(Vector(0.0, 0.0, 0.0));
    allocator->Add(Vector(0.0, 50.0, 0.0));
    allocator->Add(Vector(0.0, 100.0, 0.0));
    allocator->Add(Vector(0.0, 150.0, 0.0));
    gatewayMobility.SetPositionAllocator(allocator);
    gatewayMobility.Install(gateways);

    // Wifi Connection
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());
    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("OfdmRate6Mbps"),
                                 "RtsCtsThreshold",
                                 UintegerValue(0));

    gwDevices = wifi.Install(wifiPhy, wifiMac, gateways);
    nsDevices = wifi.Install(wifiPhy, wifiMac, networkServers);

    wifiPhy.EnablePcapAll(std::string("aodv"));

    AodvHelper aodv;
    // you can configure AODV attributes here using aodv.Set(name, value)
    InternetStackHelper stack;
    stack.SetRoutingHelper(aodv); // has effect on the next Install ()

    stack.Install(gateways);
    stack.Install(networkServers);

    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.0.0.0");

    gwInterfaces = address.Assign(gwDevices);
    nsInterfaces = address.Assign(nsDevices);

    Ptr<OutputStreamWrapper> routingStream =
        Create<OutputStreamWrapper>("aodv.routes", std::ios::out);
    aodv.PrintRoutingTableAllEvery(Seconds(1), routingStream);

    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), 6666));

    ApplicationContainer gwSinkApps = packetSinkHelper.Install(gateways);

    for (uint32_t i = 0; i < gwSinkApps.GetN(); i++)
    {
        gwSinkApps.Get(i)->GetObject<PacketSink>()->TraceConnectWithoutContext(
            "Rx",
            MakeCallback(&PacketReceivedGW));
    }

    ApplicationContainer nsSinkApps = packetSinkHelper.Install(networkServers);

    for (uint32_t i = 0; i < nsSinkApps.GetN(); i++)
    {
        nsSinkApps.Get(i)->GetObject<PacketSink>()->TraceConnectWithoutContext(
            "Rx",
            MakeCallback(&PacketReceivedNS));
    }

    Ptr<Socket> socket = Socket::CreateSocket(gateways.Get(0), UdpSocketFactory::GetTypeId());

    // Define a destination address and port
    Ipv4Address remoteAddress = nsInterfaces.GetAddress(0);
    uint16_t remotePort = 6666;

    // Bind the socket to a local port
    InetSocketAddress local = InetSocketAddress(gwInterfaces.GetAddress(0), 7777);
    socket->Bind(local);

    // Schedule the initial packet sending
    Simulator::Schedule(Seconds(2.0), &SendPacket, socket, remoteAddress, remotePort);

    /////////////////////////////////
    ///////Install lora stack///////
    ///////////////////////////////

    Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
    loss->SetPathLossExponent (3.76);
    loss->SetReference (1, 7.7);

    Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();

    Ptr<LoraChannel> loraChannel = CreateObject<LoraChannel> (loss, delay);

    LoraHelper loraHelper = LoraHelper();
    loraHelper.EnablePacketTracking();

    LoraPhyHelper loraPhyHelper = LoraPhyHelper();
    loraPhyHelper.SetChannel(loraChannel);

    LorawanMacHelper lorawanMacHelper = LorawanMacHelper();

    NetworkServerHelper nsHelper = NetworkServerHelper ();

    /////Create end devices
    NodeContainer loraEndDevices;
    loraEndDevices.Create(20);


    //////End devices Mobility

    double radius = 18000;
    MobilityHelper endDeviceMobility;
    endDeviceMobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator", "rho", DoubleValue (radius),
                                  "X", DoubleValue (0.0), "Y", DoubleValue (0.0));
    endDeviceMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    endDeviceMobility.Install(loraEndDevices);
    ////// Make it so that nodes are at a certain height > 0
    for (NodeContainer::Iterator j = loraEndDevices.Begin (); j != loraEndDevices.End (); ++j)
    {
        Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
        Vector position = mobility->GetPosition ();
        position.z = 1.2;
        mobility->SetPosition (position);
    }
    ////// Create the LoraNetDevices of the end devices
    uint8_t nwkId = 54;
    uint32_t nwkAddr = 1864;
    Ptr<LoraDeviceAddressGenerator> addrGen =
        CreateObject<LoraDeviceAddressGenerator> (nwkId, nwkAddr);

    ////// Create the LoraNetDevices of the end devices
    lorawanMacHelper.SetAddressGenerator (addrGen);
    loraPhyHelper.SetDeviceType (LoraPhyHelper::ED);
    lorawanMacHelper.SetDeviceType (LorawanMacHelper::ED_A);
    loraHelper.Install (loraPhyHelper, lorawanMacHelper, loraEndDevices);


    /////// Install Lora Gateways
    loraPhyHelper.SetDeviceType(ns3::lorawan::LoraPhyHelper::GW);
    lorawanMacHelper.SetDeviceType(ns3::lorawan::LorawanMacHelper::GW);
    loraHelper.Install(loraPhyHelper, lorawanMacHelper, gateways);

    for (NodeContainer::Iterator j = gateways.Begin (); j != gateways.End (); ++j)
    {
        Ptr<Node> node = *j;
        Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (2)->GetObject<LoraNetDevice> ();
        loraNetDevice->SetReceiveCallback(MakeCallback(&ReceiveFromLora));

    }

    ///// Set spreading factors up
    lorawanMacHelper.SetSpreadingFactorsUp (loraEndDevices, gateways, loraChannel);

    ////// Install applications on end devices

    PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
    appHelper.SetPeriod (simulationTime);
    appHelper.SetPacketSize (23);
    Ptr<RandomVariableStream> rv = CreateObjectWithAttributes<UniformRandomVariable> (
        "Min", DoubleValue (0), "Max", DoubleValue (10));
    ApplicationContainer appContainer = appHelper.Install (loraEndDevices);

    appContainer.Start (Seconds (0));
    appContainer.Stop (simulationTime);


    /////// Install Lora Network Server
    nsHelper.SetEndDevices(loraEndDevices);
    nsHelper.SetGateways(gateways);
    nsHelper.Install(networkServers);


    //////////////////////////////
    //////End lora stack//////////
    //////////////////////////////

    Simulator::Stop(simulationTime);
    Simulator::Run();
    Simulator::Destroy();
}
