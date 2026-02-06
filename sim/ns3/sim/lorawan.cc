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
#include "ns3/lorawan-mac-header.h"
#include "ns3/mobility-helper.h"
#include "ns3/network-module.h"
#include "ns3/network-server-helper.h"
#include "ns3/node-container.h"
#include "ns3/periodic-sender-helper.h"
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

void
OnGatewayMacReceive(Ptr<const Packet> packet)
{
    Ptr<Packet> packetCopy = packet->Copy();

    LoraTag tag;
    packetCopy->RemovePacketTag(tag);

    double rxPower = tag.GetReceivePower();
    double snr = SNR(rxPower);
    uint8_t sf = tag.GetSpreadingFactor();
    uint32_t freq = tag.GetFrequency();
    // EU868: SF12=DR0 ... SF7=DR5
    uint8_t dataRate = (sf >= 7 && sf <= 12) ? (12 - sf) : 0;

    NS_LOG_UNCOND("\n[GW] Uplink received at " << Simulator::Now().As(Time::S));
    NS_LOG_UNCOND("  RX Power : " << rxPower << " dBm");
    NS_LOG_UNCOND("  SNR      : " << snr << " dB");
    NS_LOG_UNCOND("  SF       : " << (int)sf << "  (DR" << (int)dataRate << ")");
    NS_LOG_UNCOND("  Frequency: " << freq << " Hz");
}

void
OnEndDeviceMacReceive(Ptr<const Packet> packet)
{
    NS_LOG_UNCOND("\n[ED] MAC received downlink at " << Simulator::Now().As(Time::S));
    NS_LOG_UNCOND("  Packet size: " << packet->GetSize() << " bytes");

    Ptr<Packet> copy = packet->Copy();
    LoraTag tag;
    if (copy->RemovePacketTag(tag))
    {
        NS_LOG_UNCOND("  RX Power : " << tag.GetReceivePower() << " dBm");
        NS_LOG_UNCOND("  Frequency: " << tag.GetFrequency() << " Hz");
        NS_LOG_UNCOND("  SF       : " << (int)tag.GetSpreadingFactor());
    }
}

void
OnEndDevicePhyReceive(Ptr<const Packet> packet, uint32_t nodeId)
{
    Ptr<Packet> copy = packet->Copy();
    LoraTag tag;
    copy->RemovePacketTag(tag);

    NS_LOG_UNCOND("\n[ED] PHY received packet on Node " << nodeId << " at "
                                                        << Simulator::Now().As(Time::S));
    NS_LOG_UNCOND("  RX Power : " << tag.GetReceivePower() << " dBm");
    NS_LOG_UNCOND("  Frequency: " << tag.GetFrequency() << " Hz");
    NS_LOG_UNCOND("  SF       : " << (int)tag.GetSpreadingFactor());
    NS_LOG_UNCOND("  Size     : " << packet->GetSize() << " bytes");
}

void
OnLostWrongFrequency(Ptr<const Packet>, uint32_t nodeId)
{
    NS_LOG_UNCOND("[LOST] Node " << nodeId << ": wrong frequency");
}

void
OnLostWrongSf(Ptr<const Packet>, uint32_t nodeId)
{
    NS_LOG_UNCOND("[LOST] Node " << nodeId << ": wrong spreading factor");
}

void
OnLostUnderSensitivity(Ptr<const Packet>, uint32_t nodeId)
{
    NS_LOG_UNCOND("[LOST] Node " << nodeId << ": under sensitivity");
}

void
OnLostInterference(Ptr<const Packet>, uint32_t nodeId)
{
    NS_LOG_UNCOND("[LOST] Node " << nodeId << ": interference");
}

void
OnNetworkServerReceive(Ptr<const Packet> packet)
{
    NS_LOG_UNCOND("\n[NS] Network server received packet at "
                  << Simulator::Now().As(Time::S) << ", size: " << packet->GetSize() << " bytes");
}

void
PrintDevicePositions(NodeContainer devices, std::string deviceType)
{
    NS_LOG_UNCOND("\n=== " << deviceType << " Positions ===");
    for (uint32_t i = 0; i < devices.GetN(); i++)
    {
        Ptr<Node> node = devices.Get(i);
        Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
        Vector pos = mob->GetPosition();
        double distance = sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
        NS_LOG_UNCOND("  " << deviceType << " " << i << " (Node " << node->GetId() << "): ("
                           << pos.x << ", " << pos.y << ", " << pos.z
                           << "), dist from origin: " << distance << " m");
    }
}

int
main(int argc, char* argv[])
{
    //  Propagation 
    Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
    loss->SetPathLossExponent(PATH_LOSS_EXPONENT);
    loss->SetReference(REFERENCE_DISTANCE, REFERENCE_LOSS);

    Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel>();
    Ptr<LoraChannel> channel = CreateObject<LoraChannel>(loss, delay);

    //  Mobility 
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator>();
    allocator->Add(Vector(0, 0, 0));      // End Device 0
    allocator->Add(Vector(1000, 0, 0));   // End Device 1: 1 km east
    allocator->Add(Vector(500, 866, 0));  // End Device 2: ~1 km NE
    allocator->Add(Vector(-750, 750, 0)); // Gateway:      ~1.06 km NW
    mobility.SetPositionAllocator(allocator);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    // Helpers
    LoraPhyHelper phyHelper;
    phyHelper.SetChannel(channel);
    LorawanMacHelper macHelper;
    LoraHelper helper;

    //  Address generator 
    uint8_t nwkId = 54;
    uint32_t nwkAddr = 1864;
    Ptr<LoraDeviceAddressGenerator> addrGen =
        CreateObject<LoraDeviceAddressGenerator>(nwkId, nwkAddr);

    //  End Devices 
    NodeContainer endDevices;
    endDevices.Create(3);
    mobility.Install(endDevices);

    phyHelper.SetDeviceType(LoraPhyHelper::ED);
    macHelper.SetDeviceType(LorawanMacHelper::ED_A);
    macHelper.SetAddressGenerator(addrGen);
    macHelper.SetRegion(LorawanMacHelper::EU);
    NetDeviceContainer edNetDevices = helper.Install(phyHelper, macHelper, endDevices);

    // Set end devices to use CONFIRMED uplinks so the network server sends ACK downlinks
    for (uint32_t i = 0; i < endDevices.GetN(); i++)
    {
        Ptr<LoraNetDevice> lnd = DynamicCast<LoraNetDevice>(edNetDevices.Get(i));
        Ptr<EndDeviceLorawanMac> edMac = DynamicCast<EndDeviceLorawanMac>(lnd->GetMac());
        edMac->SetMType(LorawanMacHeader::CONFIRMED_DATA_UP);
    }

    //  Gateway 
    NodeContainer gateways;
    gateways.Create(1);
    mobility.Install(gateways);

    phyHelper.SetDeviceType(LoraPhyHelper::GW);
    macHelper.SetDeviceType(LorawanMacHelper::GW);
    NetDeviceContainer gwNetDevices = helper.Install(phyHelper, macHelper, gateways);

    //  Spreading factor assignment 
    LorawanMacHelper::SetSpreadingFactorsUp(endDevices, gateways, channel);

    //  Network Server + P2P backhaul 
    Ptr<Node> networkServer = CreateObject<Node>();

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    P2PGwRegistration_t gwRegistration;
    for (auto gw = gateways.Begin(); gw != gateways.End(); ++gw)
    {
        auto container = p2p.Install(networkServer, *gw);
        auto serverP2PNetDev = DynamicCast<PointToPointNetDevice>(container.Get(0));
        gwRegistration.emplace_back(serverP2PNetDev, *gw);
    }

    NetworkServerHelper nsHelper;
    nsHelper.SetGatewaysP2P(gwRegistration);
    nsHelper.SetEndDevices(endDevices);
    nsHelper.Install(networkServer);

    //  Forwarder on gateway (bridges LoRa <-> P2P) 
    ForwarderHelper forwarderHelper;
    forwarderHelper.Install(gateways);

    // Gateway MAC: log uplinks
    for (uint32_t i = 0; i < gateways.GetN(); i++)
    {
        Ptr<LoraNetDevice> lnd = DynamicCast<LoraNetDevice>(gateways.Get(i)->GetDevice(0));
        Ptr<GatewayLorawanMac> gwMac = DynamicCast<GatewayLorawanMac>(lnd->GetMac());
        gwMac->TraceConnectWithoutContext("ReceivedPacket", MakeCallback(&OnGatewayMacReceive));
    }

    // End device MAC + PHY: log downlinks and loss reasons
    for (uint32_t i = 0; i < endDevices.GetN(); i++)
    {
        Ptr<LoraNetDevice> lnd = DynamicCast<LoraNetDevice>(edNetDevices.Get(i));
        Ptr<EndDeviceLorawanMac> edMac = DynamicCast<EndDeviceLorawanMac>(lnd->GetMac());
        Ptr<EndDeviceLoraPhy> edPhy = DynamicCast<EndDeviceLoraPhy>(lnd->GetPhy());

        edMac->TraceConnectWithoutContext("ReceivedPacket", MakeCallback(&OnEndDeviceMacReceive));
        edPhy->TraceConnectWithoutContext("ReceivedPacket", MakeCallback(&OnEndDevicePhyReceive));
        edPhy->TraceConnectWithoutContext("LostPacketBecauseWrongFrequency",
                                          MakeCallback(&OnLostWrongFrequency));
        edPhy->TraceConnectWithoutContext("LostPacketBecauseWrongSpreadingFactor",
                                          MakeCallback(&OnLostWrongSf));
        edPhy->TraceConnectWithoutContext("LostPacketBecauseUnderSensitivity",
                                          MakeCallback(&OnLostUnderSensitivity));
        edPhy->TraceConnectWithoutContext("LostPacketBecauseInterference",
                                          MakeCallback(&OnLostInterference));
    }

    // Network server: log when it receives uplinks
    Ptr<NetworkServer> nsApp = DynamicCast<NetworkServer>(networkServer->GetApplication(0));
    nsApp->TraceConnectWithoutContext("ReceivedPacket", MakeCallback(&OnNetworkServerReceive));

    PeriodicSenderHelper senderHelper;
    senderHelper.SetPeriod(Seconds(10));
    senderHelper.SetPacketSize(10);
    ApplicationContainer apps = senderHelper.Install(endDevices);
    for (uint32_t i = 0; i < apps.GetN(); i++)
    {
        apps.Get(i)->SetStartTime(Seconds(2 + i * 5));
    }

    PrintDevicePositions(gateways, "Gateway");
    PrintDevicePositions(endDevices, "End Device");

    Simulator::Stop(Seconds(100));
    NS_LOG_UNCOND("\nRunning simulation...\n");
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_UNCOND("\nSimulation finished!");
    return 0;
}
