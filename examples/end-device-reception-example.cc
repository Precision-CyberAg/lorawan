

#include "ns3/mobility-helper.h"

#include "ns3/command-line.h"
#include "ns3/lora-helper.h"
#include "ns3/one-shot-sender-helper.h"

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("EndDeviceReceptionExample");

int main(int argc, char *argv[]){
    LogComponentEnable ("EndDeviceReceptionExample", LOG_LEVEL_ALL);
    LogComponentEnable ("LoraChannel", LOG_LEVEL_INFO);
    LogComponentEnable ("LoraPhy", LOG_LEVEL_ALL);
    LogComponentEnable ("EndDeviceLoraPhy", LOG_LEVEL_ALL);
    LogComponentEnable ("GatewayLoraPhy", LOG_LEVEL_ALL);
    LogComponentEnable ("LoraInterferenceHelper", LOG_LEVEL_ALL);
    LogComponentEnable ("LorawanMac", LOG_LEVEL_ALL);
    LogComponentEnable ("EndDeviceLorawanMac", LOG_LEVEL_ALL);
    LogComponentEnable ("ClassAEndDeviceLorawanMac", LOG_LEVEL_ALL);
    LogComponentEnable ("ClassCEndDeviceLorawanMac", LOG_LEVEL_ALL);
    LogComponentEnable ("GatewayLorawanMac", LOG_LEVEL_ALL);
    LogComponentEnable ("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
    LogComponentEnable ("LogicalLoraChannel", LOG_LEVEL_ALL);
    LogComponentEnable ("LoraHelper", LOG_LEVEL_ALL);
    LogComponentEnable ("LoraPhyHelper", LOG_LEVEL_ALL);
    LogComponentEnable ("LorawanMacHelper", LOG_LEVEL_ALL);
    LogComponentEnable ("OneShotSenderHelper", LOG_LEVEL_ALL);
    LogComponentEnable ("OneShotSender", LOG_LEVEL_ALL);
    LogComponentEnable ("LorawanMacHeader", LOG_LEVEL_ALL);
    LogComponentEnable ("LoraFrameHeader", LOG_LEVEL_ALL);
    LogComponentEnableAll (LOG_PREFIX_FUNC);
    LogComponentEnableAll (LOG_PREFIX_NODE);
    LogComponentEnableAll (LOG_PREFIX_TIME);

    //CHANNEL
    Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
    loss->SetPathLossExponent(3.76);
    loss->SetReference(1,7.7);

    Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel>();
    Ptr<LoraChannel> channel = CreateObject<LoraChannel>(loss, delay);

    //HELPERS
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
    allocator->Add (Vector (1000,0,0));
    allocator->Add (Vector (0,0,0));
    mobility.SetPositionAllocator (allocator);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    LoraPhyHelper phyHelper = LoraPhyHelper();
    phyHelper.SetChannel(channel);

    LorawanMacHelper macHelper = LorawanMacHelper();

    LoraHelper helper = LoraHelper();

    NodeContainer endDevices;
    endDevices.Create(1);

    mobility.Install(endDevices);

    phyHelper.SetDeviceType(LoraPhyHelper::ED);
    macHelper.SetDeviceType(LorawanMacHelper::ED_A);
    helper.Install(phyHelper, macHelper, endDevices);

    NodeContainer gateways;
    gateways.Create(1);

    mobility.Install(gateways);

    phyHelper.SetDeviceType(LoraPhyHelper::GW);
    macHelper.SetDeviceType(LorawanMacHelper::GW);
    helper.Install(phyHelper, macHelper, gateways);

    OneShotSenderHelper oneShotSenderHelper;
    oneShotSenderHelper.SetSendTime(Seconds(2));

    oneShotSenderHelper.Install(endDevices);

//    oneShotSenderHelper.SetSendTime(Seconds(3));
//    oneShotSenderHelper.Install(gateways);


    std::vector<int> sfQuantity (6);
    sfQuantity = macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);

    /****************
  *  Simulation  *
  ****************/

    Simulator::Stop (Hours (2));

    Simulator::Run ();

    Simulator::Destroy ();

    return 0;
}