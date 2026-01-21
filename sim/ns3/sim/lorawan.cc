#include "ns3/command-line.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/end-device-lorawan-mac.h"
#include "ns3/forwarder-helper.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/gateway-lorawan-mac.h"
#include "ns3/log.h"
#include "ns3/lora-device-address-generator.h"
#include "ns3/lora-helper.h"
#include "ns3/lora-net-device.h"
#include "ns3/lora-tag.h"
#include "ns3/mobility-helper.h"
#include "ns3/network-module.h"
#include "ns3/network-server-helper.h"
#include "ns3/node-container.h"
#include "ns3/one-shot-sender-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/position-allocator.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

using namespace ns3;
using namespace lorawan;

#define PATH_LOSS_EXPONENT 3.5
#define REFERENCE_DISTANCE 1.0
#define REFERENCE_LOSS 8.0

#define NOISE_FLOOR -100.0
#define SNR(rxPower) ((rxPower) - (NOISE_FLOOR))

NS_LOG_COMPONENT_DEFINE("LorawanSimpleExample");

NodeContainer g_gateways;
NodeContainer g_endDevices;

void
SendDownlinkWithSNR(double snr, uint32_t frequency, uint8_t dataRate)
{
    // Get the gateway MAC
    Ptr<Node> gateway = g_gateways.Get(0);
    Ptr<NetDevice> gwNetDevice = gateway->GetDevice(0);
    Ptr<LoraNetDevice> gwLoraNetDevice = DynamicCast<LoraNetDevice>(gwNetDevice);
    Ptr<GatewayLorawanMac> gwMac = DynamicCast<GatewayLorawanMac>(gwLoraNetDevice->GetMac());

    // Create a packet with SNR value as payload
    std::ostringstream oss;
    oss << "SNR=" << snr << " dB";
    std::string payload = oss.str();
    Ptr<Packet> replyPacket = Create<Packet>((uint8_t*)payload.c_str(), payload.length());

    // Add LoraTag with downlink parameters
    LoraTag tag;
    tag.SetFrequency(frequency);
    tag.SetDataRate(dataRate);
    replyPacket->AddPacketTag(tag);

    NS_LOG_UNCOND("Gateway sending downlink: " << payload);

    // Send the packet through the gateway MAC
    gwMac->Send(replyPacket);
}

void
OnPacketReceptionCallback(Ptr<const Packet> packet)
{
    Ptr<Packet> packetCopy = packet->Copy();

    // Get the LoraTag to extract reception information
    LoraTag tag;
    packetCopy->RemovePacketTag(tag);

    double rxPower = tag.GetReceivePower();
    double snr = SNR(rxPower);
    uint8_t sf = tag.GetSpreadingFactor();
    uint32_t freq = tag.GetFrequency();
    uint8_t dataRate = tag.GetDataRate();

    NS_LOG_UNCOND("\nGateway received uplink:");
    NS_LOG_UNCOND("  RX Power: " << rxPower << " dBm");
    NS_LOG_UNCOND("  SNR: " << snr << " dB");
    NS_LOG_UNCOND("  Spreading Factor: " << (int)sf);
    NS_LOG_UNCOND("  Frequency: " << freq << " Hz");
    NS_LOG_UNCOND("  Data Rate: " << (int)dataRate);

    Simulator::Schedule(Seconds(1.0), &SendDownlinkWithSNR, snr, freq, dataRate);
}

// Callback when end device receives a packet (downlink)
void
OnEndDeviceReceptionCallback(Ptr<const Packet> packet)
{
    Ptr<Packet> packetCopy = packet->Copy();
    uint8_t buffer[256];
    uint32_t size = packetCopy->CopyData(buffer, packetCopy->GetSize());
    std::string payload(reinterpret_cast<char*>(buffer), size);

    NS_LOG_UNCOND("\n=== End Device received downlink ===");
    NS_LOG_UNCOND("  Packet size: " << packet->GetSize() << " bytes");
    NS_LOG_UNCOND("  Payload: " << payload);
}

// Helper function to print device position
void
PrintDevicePositions(NodeContainer devices, std::string deviceType)
{
    NS_LOG_UNCOND("\n=== " << deviceType << " Positions ===");
    for (uint32_t i = 0; i < devices.GetN(); i++)
    {
        Ptr<Node> node = devices.Get(i);
        Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
        Vector pos = mobility->GetPosition();
        double distance = sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
        NS_LOG_UNCOND("  " << deviceType << " " << i << " (Node " << node->GetId()
                           << "): Position (" << pos.x << ", " << pos.y << ", " << pos.z
                           << "), Distance from origin: " << distance << " m");
    }
}

int
main(int argc, char* argv[])
{
    LogComponentEnable("LorawanSimpleExample", LOG_LEVEL_ALL);
    // LogComponentEnable("LoraChannel", LOG_LEVEL_INFO);
    // LogComponentEnable("LoraPhy", LOG_LEVEL_ALL);
    // LogComponentEnable("EndDeviceLoraPhy", LOG_LEVEL_ALL);
    // LogComponentEnable("GatewayLoraPhy", LOG_LEVEL_ALL);
    // LogComponentEnable("LoraInterferenceHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("LorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable("EndDeviceLorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable("ClassAEndDeviceLorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable("GatewayLorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("LogicalLoraChannel", LOG_LEVEL_ALL);
    // LogComponentEnable("LoraHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("LoraPhyHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("LorawanMacHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("OneShotSenderHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("OneShotSender", LOG_LEVEL_ALL);
    // LogComponentEnable("LorawanMacHeader", LOG_LEVEL_ALL);
    // LogComponentEnable("LoraFrameHeader", LOG_LEVEL_ALL);
    // LogComponentEnableAll(LOG_PREFIX_FUNC);
    // LogComponentEnableAll(LOG_PREFIX_NODE);
    // LogComponentEnableAll(LOG_PREFIX_TIME);

    Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
    loss->SetPathLossExponent(PATH_LOSS_EXPONENT);
    loss->SetReference(REFERENCE_DISTANCE, REFERENCE_LOSS);

    Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel>();

    // LoraChannel class is used to interconnect the LoRa PHY layers of all devices wishing to
    // communicate using this technology.
    Ptr<LoraChannel> channel = CreateObject<LoraChannel>(loss, delay);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator>();
    allocator->Add(Vector(0, 0, 0));      // Gateway at origin
    allocator->Add(Vector(1000, 0, 0));   // End device 1: 1km to the east
    allocator->Add(Vector(500, 866, 0));  // End device 2: 1km to the northeast (60 degrees)
    allocator->Add(Vector(-750, 750, 0)); // End device 3: ~1km to the northwest
    mobility.SetPositionAllocator(allocator);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    /*The lorawan module features helpers to configure the PHY and MAC layers on a large number of
    devices. The two layers are split in two different classes, LorawanMacHelper and LoraPhyHelper,
    which can be leveraged by a LoraHelper object to fully configure a LoRa device (both for EDs and
    for GWs). Since the helpers are general purpose (i.e., they can be used both for ED and GW
    configuration), it is necessary to specify the device type via the SetDeviceType method before
    the Install method can be called.
    */
    LoraPhyHelper phyHelper = LoraPhyHelper();
    phyHelper.SetChannel(channel);
    LorawanMacHelper macHelper = LorawanMacHelper();
    LoraHelper helper = LoraHelper();

    // Create a LoraDeviceAddressGenerator
    uint8_t nwkId = 54;
    uint32_t nwkAddr = 1864;
    Ptr<LoraDeviceAddressGenerator> addrGen =
        CreateObject<LoraDeviceAddressGenerator>(nwkId, nwkAddr);

    NodeContainer endDevices;
    endDevices.Create(3); // Create 3 end devices
    g_endDevices = endDevices;
    mobility.Install(endDevices); // Assign a mobility model to the nodes

    /*
    Create a netdevice for each gateway. NetDeviceContainer holds together pointers to
    LoraChannel, LoraPhy and LorawanMac, exposing methods through which Application instances can
    send packets.
    */
    phyHelper.SetDeviceType(LoraPhyHelper::ED); // ED = End Device type
    macHelper.SetDeviceType(LorawanMacHelper::ED_A);
    macHelper.SetAddressGenerator(addrGen);
    macHelper.SetRegion(LorawanMacHelper::EU);
    NetDeviceContainer endDevicesNetDevices =
        helper.Install(phyHelper, macHelper, endDevices); // Install ED devices

    NodeContainer gateway;
    gateway.Create(1);
    g_gateways = gateway;
    mobility.Install(gateway);

    /*
    Create a netdevice for each gateway. NetDeviceContainer holds together pointers to
    LoraChannel, LoraPhy and LorawanMac, exposing methods through which Application instances can
    send packets.
    */
    phyHelper.SetDeviceType(LoraPhyHelper::GW); // GW = Gateway type
    macHelper.SetDeviceType(LorawanMacHelper::GW);
    NetDeviceContainer gatewaysNetDevices = helper.Install(phyHelper, macHelper, gateway);

    // Connect callback to gateway MAC to see received packets
    for (uint32_t i = 0; i < gateway.GetN(); i++)
    {
        Ptr<Node> gwNode = gateway.Get(i);
        Ptr<NetDevice> netDevice = gwNode->GetDevice(0);
        Ptr<LoraNetDevice> loraNetDevice = DynamicCast<LoraNetDevice>(netDevice);
        Ptr<LorawanMac> mac = loraNetDevice->GetMac();
        Ptr<GatewayLorawanMac> gwMac = DynamicCast<GatewayLorawanMac>(mac);

        // Connect to the ReceivedPacket trace source
        gwMac->TraceConnectWithoutContext("ReceivedPacket",
                                          MakeCallback(&OnPacketReceptionCallback));
    }

    for (uint32_t i = 0; i < endDevices.GetN(); i++)
    {
        Ptr<Node> endDevice = endDevices.Get(i);
        Ptr<NetDevice> netDevice = endDevice->GetDevice(0);
        Ptr<LoraNetDevice> loraNetDevice = DynamicCast<LoraNetDevice>(netDevice);
        Ptr<LorawanMac> mac = loraNetDevice->GetMac();
        Ptr<EndDeviceLorawanMac> edMac = DynamicCast<EndDeviceLorawanMac>(mac);

        // Connect to the ReceivedPacket trace source
        edMac->TraceConnectWithoutContext("ReceivedPacket",
                                        MakeCallback(&OnEndDeviceReceptionCallback));
    }

    // Send packets at different times from each device
    OneShotSenderHelper oneShotSenderHelper;

    // Device 0 sends at 2 seconds
    oneShotSenderHelper.SetSendTime(Seconds(2));
    oneShotSenderHelper.Install(endDevices.Get(0));

    // Device 1 sends at 5 seconds
    oneShotSenderHelper.SetSendTime(Seconds(5));
    oneShotSenderHelper.Install(endDevices.Get(1));

    // Device 2 sends at 8 seconds
    oneShotSenderHelper.SetSendTime(Seconds(8));
    oneShotSenderHelper.Install(endDevices.Get(2));

    /*The LorawanMacHelper also exposes a method to set up the Spreading Factors used by the devices
     * participating in the network automatically, based on the channel conditions and on the
     * placement of devices and gateways. This procedure is contained in the static method
     * SetSpreadingFactorsUp, and works by trying to minimize the time-on-air of packets, thus
     * assigning the lowest possible spreading factor such that reception by at least one gateway is
     * still possible. It should be noted that this is an heuristic, and that it doesn’t guarantee
     * that the SF distribution is optimal for the best possible operation of the network. In fact,
     * finding such a distribution based on the network scenario is still an open challenge.*/
    LorawanMacHelper::SetSpreadingFactorsUp(endDevices, gateway, channel);

    Ptr<Node> networkServer = CreateObject<Node>();

    // PointToPoint links between gateways and server
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    
    // Store network server app registration details
    P2PGwRegistration_t gwRegistration;
    for (auto gw = gateway.Begin(); gw != gateway.End(); ++gw)
    {
        auto container = p2p.Install(networkServer, *gw);
        auto serverP2PNetDev = DynamicCast<PointToPointNetDevice>(container.Get(0));
        gwRegistration.emplace_back(serverP2PNetDev, *gw);
    }

    // Install the NetworkServer application on the network server
    NetworkServerHelper networkServerHelper;
    networkServerHelper.SetGatewaysP2P(gwRegistration);
    networkServerHelper.SetEndDevices(endDevices);
    networkServerHelper.Install(networkServer);

    // Install the Forwarder application on the gateways
    ForwarderHelper forwarderHelper;
    forwarderHelper.Install(gateway);

    // Print device positions
    PrintDevicePositions(gateway, "Gateway");
    PrintDevicePositions(endDevices, "End Device");

    Simulator::Stop(Seconds(100));
    NS_LOG_INFO("Running simulation...");
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_INFO("Simulation finished!");

    return 0;
}
