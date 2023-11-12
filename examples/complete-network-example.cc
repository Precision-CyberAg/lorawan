/*
 * This script simulates a complex scenario with multiple gateways and end
 * devices. The metric of interest for this script is the throughput of the
 * network.
 */

#include "../helper/visualizer-helper.h"

#include "ns3/building-allocator.h"
#include "ns3/building-penetration-loss.h"
#include "ns3/buildings-helper.h"
#include "ns3/class-a-end-device-lorawan-mac.h"
#include "ns3/command-line.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/correlated-shadowing-propagation-loss-model.h"
#include "ns3/double.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/forwarder-helper.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/gateway-lorawan-mac.h"
#include "ns3/log.h"
#include "ns3/lora-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/network-server-helper.h"
#include "ns3/beaconing-helper.h"
#include "ns3/node-container.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/pointer.h"
#include "ns3/position-allocator.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <ctime>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("ComplexLorawanNetworkExample");

// Network settings
int nDevices = 200;
int nGateways = 1;
double radius = 6400; // Note that due to model updates, 7500 m is no longer the maximum distance
double simulationTime = 600;
int dataUpType = 0;
char endDeviceType = 'B';

// Channel model
bool realisticChannelModel = true;

int appPeriodSeconds = 600;

// Output control
bool print = true;

int
main(int argc, char* argv[])
{
    CommandLine cmd;
    cmd.AddValue("nDevices", "Number of end devices to include in the simulation", nDevices);
    cmd.AddValue("radius", "The radius of the area to simulate", radius);
    cmd.AddValue("simulationTime", "The time for which to simulate", simulationTime);
    cmd.AddValue("appPeriod",
                 "The period in seconds to be used by periodically transmitting applications",
                 appPeriodSeconds);
    cmd.AddValue("print", "Whether or not to print various informations", print);
    cmd.AddValue("dataUpType", "The type of traffic coming from end devices: {Unconfirmed=0, Confirmed=1, Mixed=2}", dataUpType);
    cmd.AddValue("realisticModel", "Whether the channel model needs to be realistic or not", realisticChannelModel);
    cmd.AddValue("endDeviceType", "Specify the class of the end devices", endDeviceType);
    cmd.Parse(argc, argv);

    // Set up logging
    LogComponentEnable("ComplexLorawanNetworkExample", LOG_LEVEL_ALL);
    //  LogComponentEnable("LoraChannel", LOG_LEVEL_INFO);
    //  LogComponentEnable("LoraPhy", LOG_LEVEL_ALL);
    //  LogComponentEnable("EndDeviceLoraPhy", LOG_LEVEL_ALL);
    //  LogComponentEnable("GatewayLoraPhy", LOG_LEVEL_ALL);
    //  LogComponentEnable("LoraInterferenceHelper", LOG_LEVEL_ALL);
    //  LogComponentEnable("LorawanMac", LOG_LEVEL_ALL);
    //  LogComponentEnable("EndDeviceLorawanMac", LOG_LEVEL_ALL);
    //  LogComponentEnable("ClassCEndDeviceLorawanMac", LOG_LEVEL_ALL);
    //  LogComponentEnable("ClassBEndDeviceLorawanMac", LOG_LEVEL_ALL);
    //  LogComponentEnable("ClassAEndDeviceLorawanMac", LOG_LEVEL_ALL);
    //  LogComponentEnable("GatewayLorawanMac", LOG_LEVEL_ALL);
    //  LogComponentEnable("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
    //  LogComponentEnable("LogicalLoraChannel", LOG_LEVEL_ALL);
    //  LogComponentEnable("LoraHelper", LOG_LEVEL_ALL);
    //  LogComponentEnable("LoraPhyHelper", LOG_LEVEL_ALL);
    //  LogComponentEnable("LorawanMacHelper", LOG_LEVEL_ALL);
    //  LogComponentEnable("PeriodicSenderHelper", LOG_LEVEL_ALL);
    //  LogComponentEnable("PeriodicSender", LOG_LEVEL_ALL);
    //  LogComponentEnable("LorawanMacHeader", LOG_LEVEL_ALL);
    //  LogComponentEnable("LoraFrameHeader", LOG_LEVEL_ALL);
    //  LogComponentEnable("NetworkScheduler", LOG_LEVEL_ALL);
    //  LogComponentEnable("NetworkServer", LOG_LEVEL_ALL);
    //  LogComponentEnable("NetworkStatus", LOG_LEVEL_ALL);
    //  LogComponentEnable("NetworkController", LOG_LEVEL_ALL);
    //  LogComponentEnable("LoraPacketTracker", LOG_LEVEL_ALL);
    LogComponentEnableAll(LOG_PREFIX_TIME);

    /***********
     *  Setup  *
     ***********/

    // Create the time value from the period
    Time appPeriod = Seconds(appPeriodSeconds);

    // Mobility
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator",
                                  "rho",
                                  DoubleValue(radius),
                                  "X",
                                  DoubleValue(0.0),
                                  "Y",
                                  DoubleValue(0.0));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    /************************
     *  Create the channel  *
     ************************/

    // Create the lora channel object
    Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
    loss->SetPathLossExponent(3.76);
    loss->SetReference(1, 7.7);

    if (realisticChannelModel)
    {
        // Create the correlated shadowing component
        Ptr<CorrelatedShadowingPropagationLossModel> shadowing =
            CreateObject<CorrelatedShadowingPropagationLossModel>();

        // Aggregate shadowing to the logdistance loss
        loss->SetNext(shadowing);

        // Add the effect to the channel propagation loss
        Ptr<BuildingPenetrationLoss> buildingLoss = CreateObject<BuildingPenetrationLoss>();

        shadowing->SetNext(buildingLoss);
    }

    Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel>();

    Ptr<LoraChannel> channel = CreateObject<LoraChannel>(loss, delay);

    /************************
     *  Create the helpers  *
     ************************/

    // Create the LoraPhyHelper
    LoraPhyHelper phyHelper = LoraPhyHelper();
    phyHelper.SetChannel(channel);

    // Create the LorawanMacHelper
    LorawanMacHelper macHelper = LorawanMacHelper();

    // Create the LoraHelper
    LoraHelper helper = LoraHelper();
    helper.EnablePacketTracking(); // Output filename
    // helper.EnableSimulationTimePrinting ();

    // Create the NetworkServerHelper
    NetworkServerHelper nsHelper = NetworkServerHelper();

    // Create the ForwarderHelper
    ForwarderHelper forHelper = ForwarderHelper();

    /************************
     *  Create End Devices  *
     ************************/

    // Create a set of nodes
    NodeContainer endDevices;
    endDevices.Create(nDevices);

    // Assign a mobility model to each node
    mobility.Install(endDevices);

    // Make it so that nodes are at a certain height > 0
    for (NodeContainer::Iterator j = endDevices.Begin(); j != endDevices.End(); ++j)
    {
        Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel>();
        Vector position = mobility->GetPosition();
        position.z = 1.2;
        mobility->SetPosition(position);
    }

    // Create the LoraNetDevices of the end devices
    uint8_t nwkId = 54;
    uint32_t nwkAddr = 1864;
    Ptr<LoraDeviceAddressGenerator> addrGen =
        CreateObject<LoraDeviceAddressGenerator>(nwkId, nwkAddr);

    // Create the LoraNetDevices of the end devices
    macHelper.SetAddressGenerator(addrGen);
    phyHelper.SetDeviceType(LoraPhyHelper::ED);

    switch (endDeviceType)
    {
    case 'A':
        macHelper.SetDeviceType(LorawanMacHelper::ED_A);
        break;
    case 'B':
        macHelper.SetDeviceType(LorawanMacHelper::ED_B);
        break;
    case 'C':
        macHelper.SetDeviceType(LorawanMacHelper::ED_C);
        break;
    default:
        macHelper.SetDeviceType(LorawanMacHelper::ED_A);
        break;
    }
    
    helper.Install(phyHelper, macHelper, endDevices);

    LorawanMacHeader::MType mType1, mType2;
    switch (dataUpType)
    {
    case 0:
        mType1=LorawanMacHeader::MType::UNCONFIRMED_DATA_UP;
        mType2=LorawanMacHeader::MType::UNCONFIRMED_DATA_UP;
        break;
    
    case 1:
        mType1=LorawanMacHeader::MType::CONFIRMED_DATA_UP;
        mType2=LorawanMacHeader::MType::CONFIRMED_DATA_UP;
        break;
    
    case 2:
        mType1=LorawanMacHeader::MType::CONFIRMED_DATA_UP;
        mType2=LorawanMacHeader::MType::UNCONFIRMED_DATA_UP;
        break;
    
    default:
        mType1=LorawanMacHeader::MType::UNCONFIRMED_DATA_UP;
        mType2=LorawanMacHeader::MType::UNCONFIRMED_DATA_UP;
        break;
    }

    // Set message type (Default is unconfirmed)
    for (uint32_t i = 0; i < endDevices.GetN(); i++)
    {
        Ptr<LorawanMac> edMac =
            endDevices.Get(i)->GetDevice(0)->GetObject<LoraNetDevice>()->GetMac();
        Ptr<EndDeviceLorawanMac> edLoraMac = edMac->GetObject<EndDeviceLorawanMac>();

        if (i%2==0)
        {
            edLoraMac->SetMType(mType1);
        }
        else
        {
            edLoraMac->SetMType(mType2);
        }
    }

    // Now end devices are connected to the channel

    // Connect trace sources
    for (NodeContainer::Iterator j = endDevices.Begin(); j != endDevices.End(); ++j)
    {
        Ptr<Node> node = *j;
        Ptr<LoraNetDevice> loraNetDevice = node->GetDevice(0)->GetObject<LoraNetDevice>();
        Ptr<LoraPhy> phy = loraNetDevice->GetPhy();
    }

    /*********************
     *  Create Gateways  *
     *********************/

    // Create the gateway nodes (allocate them uniformely on the disc)
    NodeContainer gateways;
    gateways.Create(nGateways);

    Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator>();
    // Make it so that nodes are at a certain height > 0
    allocator->Add(Vector(0.0, 0.0, 15.0));
    mobility.SetPositionAllocator(allocator);
    mobility.Install(gateways);

    // Create a netdevice for each gateway
    phyHelper.SetDeviceType(LoraPhyHelper::GW);
    macHelper.SetDeviceType(LorawanMacHelper::GW);
    helper.Install(phyHelper, macHelper, gateways);

    /**********************
     *  Handle buildings  *
     **********************/

    double xLength = 130;
    double deltaX = 32;
    double yLength = 64;
    double deltaY = 17;
    int gridWidth = 2 * radius / (xLength + deltaX);
    int gridHeight = 2 * radius / (yLength + deltaY);
    if (realisticChannelModel == false)
    {
        gridWidth = 0;
        gridHeight = 0;
    }
    Ptr<GridBuildingAllocator> gridBuildingAllocator;
    gridBuildingAllocator = CreateObject<GridBuildingAllocator>();
    gridBuildingAllocator->SetAttribute("GridWidth", UintegerValue(gridWidth));
    gridBuildingAllocator->SetAttribute("LengthX", DoubleValue(xLength));
    gridBuildingAllocator->SetAttribute("LengthY", DoubleValue(yLength));
    gridBuildingAllocator->SetAttribute("DeltaX", DoubleValue(deltaX));
    gridBuildingAllocator->SetAttribute("DeltaY", DoubleValue(deltaY));
    gridBuildingAllocator->SetAttribute("Height", DoubleValue(6));
    gridBuildingAllocator->SetBuildingAttribute("NRoomsX", UintegerValue(2));
    gridBuildingAllocator->SetBuildingAttribute("NRoomsY", UintegerValue(4));
    gridBuildingAllocator->SetBuildingAttribute("NFloors", UintegerValue(2));
    gridBuildingAllocator->SetAttribute(
        "MinX",
        DoubleValue(-gridWidth * (xLength + deltaX) / 2 + deltaX / 2));
    gridBuildingAllocator->SetAttribute(
        "MinY",
        DoubleValue(-gridHeight * (yLength + deltaY) / 2 + deltaY / 2));
    BuildingContainer bContainer = gridBuildingAllocator->Create(gridWidth * gridHeight);

    BuildingsHelper::Install(endDevices);
    BuildingsHelper::Install(gateways);

    // Print the buildings
    if (print)
    {
        std::ofstream myfile;
        myfile.open("buildings.txt");
        std::vector<Ptr<Building>>::const_iterator it;
        int j = 1;
        for (it = bContainer.Begin(); it != bContainer.End(); ++it, ++j)
        {
            Box boundaries = (*it)->GetBoundaries();
            myfile << "set object " << j << " rect from " << boundaries.xMin << ","
                   << boundaries.yMin << " to " << boundaries.xMax << "," << boundaries.yMax
                   << std::endl;
        }
        myfile.close();
    }

    /**********************************************
     *  Set up the end device's spreading factor  *
     **********************************************/

    macHelper.SetSpreadingFactorsUp(endDevices, gateways, channel);

    NS_LOG_DEBUG("Completed configuration");

    /*********************************************
     *  Install applications on the end devices  *
     *********************************************/

    Time appStopTime = Seconds(simulationTime);
    Ptr<RandomVariableStream> rv =
        CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                          DoubleValue(0),
                                                          "Max",
                                                          DoubleValue(10));
    
    PeriodicSenderHelper appHelper = PeriodicSenderHelper();
    appHelper.SetPacketSize(23);
    ApplicationContainer appContainer;

    // for( uint32_t i = 0 ; i < endDevices.GetN() ; i++ ){

    //     if(i<(0.05*endDevices.GetN())){

    //         //5% of the devices
    //         appHelper.SetPeriod(Minutes(30));
    //         appContainer.Add(appHelper.Install(endDevices.Get(i)));

    //     }else if(i<(0.20*endDevices.GetN())){

    //         //15% of the devices
    //         appHelper.SetPeriod(Hours(1));
    //         appContainer.Add(appHelper.Install(endDevices.Get(i)));

    //     }else if(i<(0.60*endDevices.GetN())){

    //         //40% of the devices
    //         appHelper.SetPeriod(Hours(2));
    //         appContainer.Add(appHelper.Install(endDevices.Get(i)));

    //     }else{

    //         //40% of the devices
    //         appHelper.SetPeriod(Days(1));
    //         appContainer.Add(appHelper.Install(endDevices.Get(i)));

    //     }
    // }
    
    appHelper.SetPeriod(appPeriod);
    appContainer.Add(appHelper.Install(endDevices));

    appContainer.Start(Seconds(0));
    appContainer.Stop(appStopTime);

    /**************************
     *  Create Network Server  *
     ***************************/

    // Create the NS node
    NodeContainer networkServer;
    networkServer.Create(1);

    // Create a NS for the network
    nsHelper.SetEndDevices(endDevices);
    nsHelper.SetGateways(gateways);
    nsHelper.Install(networkServer);

    Ptr<ListPositionAllocator> allocatorNs = CreateObject<ListPositionAllocator>();
    // Make it so that nodes are at a certain height > 0
    allocatorNs->Add(Vector(3000.0, 4000.0, 15.0));
    mobility.SetPositionAllocator(allocatorNs);
    mobility.Install(networkServer);

    // Create a forwarder for each gateway
    forHelper.Install(gateways);

    if(endDeviceType=='B'){
        BeaconingHelper beaconingHelper;
        beaconingHelper.SetDeviceType(Beaconing::DeviceType::GW);
        beaconingHelper.Install(gateways);
        beaconingHelper.SetDeviceType(Beaconing::DeviceType::ED);
        beaconingHelper.Install(endDevices);
    }

    // Visualizer
    VisualizerHelper visHelper;
    visHelper.SetSimulationTime(appStopTime + Hours(1));
    visHelper.Install(endDevices, Visualizer::DeviceType::ED);
    visHelper.Install(gateways, Visualizer::DeviceType::GW);
    visHelper.Install(networkServer, Visualizer::DeviceType::NS);

    ////////////////
    // Simulation //
    ////////////////

    Simulator::Stop(appStopTime + Hours(1));

    NS_LOG_INFO("Running simulation...");
    Simulator::Run();

    Simulator::Destroy();

    ///////////////////////////
    // Print results to file //
    ///////////////////////////
    NS_LOG_INFO("Computing performance metrics...");

    LoraPacketTracker& tracker = helper.GetPacketTracker();
    std::cout << tracker.CountMacPacketsGlobally(Seconds(0), appStopTime + Hours(1)) << std::endl;

    std::string macPacketCountULPDR =
        tracker.CountMacPacketsGlobally(Seconds(0), appStopTime + Hours(1));
    double firstDoubleULPDR, secondDoubleULPDR, 
    thirdDoubleULPDR, fourthDoubleULPDR, confirmedSentULPDR, confirmedReceivedULPDR;

    std::istringstream ssULPDR(macPacketCountULPDR);
    ssULPDR >> firstDoubleULPDR >> secondDoubleULPDR >> 
    thirdDoubleULPDR >> fourthDoubleULPDR >> confirmedSentULPDR >> confirmedReceivedULPDR;

    std::string macPacketCountCPSR =
        tracker.CountMacPacketsGloballyCpsr(Seconds(0), appStopTime + Hours(1));
    double firstDoubleCPSR, secondDoubleCPSR;

    std::istringstream ssCPSR(macPacketCountCPSR);
    ssCPSR >> firstDoubleCPSR >> secondDoubleCPSR;

    std::cout << "{"
              << "\"SimTime\":"
              << "\"" + std::to_string(appStopTime.GetSeconds()) + "\""
              << ","
              << "\"ULPDR\":{"
              << "\"Total\":"
              << "\"" << firstDoubleULPDR << "\","
              << "\"Success\":"
              << "\"" << secondDoubleULPDR << "\""
              << "},"
              << "\"CPSR\":{"
              << "\"Total\":"
              << "\"" << firstDoubleCPSR << "\","
              << "\"Success\":"
              << "\"" << secondDoubleCPSR << "\""
              << "},"
              << "\"Packets\":{"
              << "\"Total\":"
              << "\"" << firstDoubleULPDR << "\","
              << "\"Confirmed\":"
              << "\"" << confirmedSentULPDR << "\","
              << "\"Unconfirmed\":"
              << "\"" << (thirdDoubleULPDR) << "\","
              << "\"ConfirmedSuccess\":"
              << "\"" << confirmedReceivedULPDR << "\","
              << "\"UnconfirmedSuccess\":"
              << "\"" << (fourthDoubleULPDR) << "\""
              << "}"
              << "}" << std::endl;

    return 0;
}
