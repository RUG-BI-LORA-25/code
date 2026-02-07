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
#include "ns3/lora-frame-header.h"
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

#include <algorithm>
#include <cmath>
#include <map>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("LorawanPidAdr");

constexpr double PATH_LOSS_EXPONENT = 3.5;
constexpr double REFERENCE_DISTANCE = 1.0;
constexpr double REFERENCE_LOSS = 8.0;

// EU868 transmit power limits (dBm ERP)
constexpr double P_MIN = 2.0;
constexpr double P_MAX = 14.0;
constexpr double P_INITIAL = 14.0;

// Spreading factor limits
constexpr uint8_t SF_MIN = 7;
constexpr uint8_t SF_MAX = 12;
constexpr uint8_t SF_INITIAL = 7;

// PD gains
constexpr double KP = 0.5;
constexpr double KD = 0.1;

// Slow-loop thresholds
constexpr double MARGIN_THRESHOLD = 5.0; // dB above target to consider SF decrease
constexpr uint32_t STABILITY_WINDOW = 3; // consecutive good packets before SF change
constexpr double COMFORT_MARGIN = 10.0;

static const double SNR_TARGET_PER_SF[6] = {
    -7.5,  // SF7
    -10.0, // SF8
    -12.5, // SF9
    -15.0, // SF10
    -17.5, // SF11
    -20.0  // SF12
};

constexpr double NOISE_FLOOR_DBM = -123.0;

inline double
ComputeSnr(double rxPowerDbm)
{
    return rxPowerDbm - NOISE_FLOOR_DBM;
}

inline double
SnrTarget(uint8_t sf)
{
    if (sf >= SF_MIN && sf <= SF_MAX)
    {
        return SNR_TARGET_PER_SF[sf - SF_MIN];
    }
    return SNR_TARGET_PER_SF[0];
}

inline uint8_t
SfToDataRate(uint8_t sf)
{
    return (sf >= 7 && sf <= 12) ? (12 - sf) : 0;
}

inline uint8_t
DataRateToSf(uint8_t dr)
{
    return (dr <= 5) ? (12 - dr) : 7;
}

struct PidState
{
    double ePrev = 0.0;         // previous error (for D term)
    uint8_t sf = SF_INITIAL;    // current spreading factor
    double txPower = P_INITIAL; // current TX power (dBm)
    uint32_t stableCount = 0;   // consecutive packets with margin > threshold
};

static std::map<uint32_t, PidState> g_pidState; // keyed by device address
static NodeContainer g_endDevices;
static NetDeviceContainer g_edNetDevices;

static int
FindDeviceIndex(uint32_t addr)
{
    for (uint32_t i = 0; i < g_edNetDevices.GetN(); i++)
    {
        Ptr<LoraNetDevice> lnd = DynamicCast<LoraNetDevice>(g_edNetDevices.Get(i));
        Ptr<EndDeviceLorawanMac> mac = DynamicCast<EndDeviceLorawanMac>(lnd->GetMac());
        if (mac->GetDeviceAddress().Get() == addr)
        {
            return (int)i;
        }
    }
    return -1;
}

static void
ApplyControlToDevice(int idx, double newTxPower, uint8_t newSf)
{
    Ptr<LoraNetDevice> lnd = DynamicCast<LoraNetDevice>(g_edNetDevices.Get(idx));
    Ptr<EndDeviceLorawanMac> mac = DynamicCast<EndDeviceLorawanMac>(lnd->GetMac());
    mac->SetTransmissionPowerDbm(newTxPower);
    mac->SetDataRate(SfToDataRate(newSf));
}

static void
OnGatewayUplinkReceived(Ptr<const Packet> packet)
{
    Ptr<Packet> copy = packet->Copy();

    LoraTag tag;
    copy->RemovePacketTag(tag);
    double rxPower = tag.GetReceivePower();
    double snrMeasured = ComputeSnr(rxPower);
    uint8_t sf = tag.GetSpreadingFactor();
    uint32_t freq = tag.GetFrequency();

    LorawanMacHeader macHdr;
    copy->RemoveHeader(macHdr);
    LoraFrameHeader frameHdr;
    frameHdr.SetAsUplink();
    copy->RemoveHeader(frameHdr);
    uint32_t devAddr = frameHdr.GetAddress().Get();

    int idx = FindDeviceIndex(devAddr);
    if (idx < 0)
    {
        NS_LOG_WARN("[PID] Unknown device address " << devAddr);
        return;
    }

    PidState& s = g_pidState[devAddr];

    // Remember what the device used for this packet
    double txPowerUsed = s.txPower;
    uint8_t sfUsed = s.sf;

    double target = SnrTarget(s.sf);
    double error = target - snrMeasured;

    double dTerm = KD * (error - s.ePrev);
    s.ePrev = error;

    double deltaP = KP * error + dTerm;

    // Round to nearest 2 dB step
    deltaP = std::round(deltaP / 2.0) * 2.0;
    s.txPower = std::clamp(s.txPower + deltaP, P_MIN, P_MAX);

    double margin = snrMeasured - target;

    double snrAtMaxPower = snrMeasured + (P_MAX - txPowerUsed);
    bool linkStressed = (snrMeasured < target) || (snrAtMaxPower - target < COMFORT_MARGIN);

    if (linkStressed && s.sf < SF_MAX)
    {
        s.sf++;
        s.txPower = P_MAX; 
        s.stableCount = 0;
        s.ePrev = 0.0;
        NS_LOG_UNCOND("[PID] Node " << idx << ": SF increased → " << (int)s.sf
                                    << "  (SNR@Pmax=" << snrAtMaxPower << " dB)");
    }
    else if (margin > MARGIN_THRESHOLD && s.txPower <= P_MIN)
    {
        s.stableCount++;
        if (s.stableCount >= STABILITY_WINDOW && s.sf > SF_MIN)
        {
            s.sf--;
            s.txPower = P_MAX;
            s.stableCount = 0;
            s.ePrev = 0.0;
            NS_LOG_UNCOND("[PID] Node " << idx << ": SF decreased → " << (int)s.sf);
        }
        else if (s.sf == SF_MIN)
        {
            s.stableCount = STABILITY_WINDOW;
        }
    }
    else
    {
        s.stableCount = 0;
    }

    ApplyControlToDevice(idx, s.txPower, s.sf);

    NS_LOG_UNCOND("\n[GW] Uplink at " << Simulator::Now().As(Time::S) << " from Node " << idx);
    NS_LOG_UNCOND("  RX Power  : " << rxPower << " dBm  (device TX'd at " << txPowerUsed
                                   << " dBm)");
    NS_LOG_UNCOND("  SNR       : " << snrMeasured << " dB  (target: " << target << " dB)");
    NS_LOG_UNCOND("  SF        : " << (int)sfUsed << " → " << (int)s.sf
                                   << "  DR=" << (int)SfToDataRate(s.sf));
    NS_LOG_UNCOND("  TX Power  : " << txPowerUsed << " → " << s.txPower << " dBm  (ΔP = " << deltaP
                                   << ")");
    NS_LOG_UNCOND("  Frequency : " << freq << " Hz");
    NS_LOG_UNCOND("  Margin    : " << margin << " dB  (stable: " << s.stableCount << "/"
                                   << STABILITY_WINDOW << ")");
}

static void
OnEndDeviceMacReceive(Ptr<const Packet> packet)
{
    NS_LOG_UNCOND("[ED] MAC downlink at " << Simulator::Now().As(Time::S)
                                          << "  size=" << packet->GetSize());
}

static void
OnLostWrongSf(Ptr<const Packet>, uint32_t nodeId)
{
    NS_LOG_UNCOND("[LOST] Node " << nodeId << ": wrong SF");
}

static void
OnLostUnderSensitivity(Ptr<const Packet>, uint32_t nodeId)
{
    NS_LOG_UNCOND("[LOST] Node " << nodeId << ": under sensitivity");
}

static void
PrintDevicePositions(NodeContainer devices, const std::string& deviceType)
{
    NS_LOG_UNCOND("\n=== " << deviceType << " Positions ===");
    for (uint32_t i = 0; i < devices.GetN(); i++)
    {
        Ptr<Node> node = devices.Get(i);
        Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
        Vector pos = mob->GetPosition();
        double distance = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
        NS_LOG_UNCOND("  " << deviceType << " " << i << " (Node " << node->GetId() << "): ("
                           << pos.x << ", " << pos.y << ", " << pos.z << "), dist=" << distance
                           << " m");
    }
}

int
main(int argc, char* argv[])
{
    double simTime = 300.0;  
    double sendPeriod = 30.0; 

    CommandLine cmd;
    cmd.AddValue("simTime", "Total simulation time (s)", simTime);
    cmd.AddValue("sendPeriod", "Uplink period (s)", sendPeriod);
    cmd.Parse(argc, argv);

    Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
    loss->SetPathLossExponent(PATH_LOSS_EXPONENT);
    loss->SetReference(REFERENCE_DISTANCE, REFERENCE_LOSS);

    Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel>();
    Ptr<LoraChannel> channel = CreateObject<LoraChannel>(loss, delay);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator>();
    allocator->Add(Vector(0, 0, 0));       // ED 0: at origin (close)
    allocator->Add(Vector(5000, 0, 0));    // ED 1: 5 km east (far)
    allocator->Add(Vector(3000, 2000, 0)); // ED 2: ~3.6 km NE (medium)
    allocator->Add(Vector(0, 500, 0));     // Gateway: 500 m north
    mobility.SetPositionAllocator(allocator);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    LoraPhyHelper phyHelper;
    phyHelper.SetChannel(channel);
    LorawanMacHelper macHelper;
    LoraHelper helper;

    uint8_t nwkId = 54;
    uint32_t nwkAddr = 1864;
    Ptr<LoraDeviceAddressGenerator> addrGen =
        CreateObject<LoraDeviceAddressGenerator>(nwkId, nwkAddr);

    g_endDevices.Create(3);
    mobility.Install(g_endDevices);

    phyHelper.SetDeviceType(LoraPhyHelper::ED);
    macHelper.SetDeviceType(LorawanMacHelper::ED_A);
    macHelper.SetAddressGenerator(addrGen);
    macHelper.SetRegion(LorawanMacHelper::EU);
    g_edNetDevices = helper.Install(phyHelper, macHelper, g_endDevices);

    for (uint32_t i = 0; i < g_endDevices.GetN(); i++)
    {
        Ptr<LoraNetDevice> lnd = DynamicCast<LoraNetDevice>(g_edNetDevices.Get(i));
        Ptr<EndDeviceLorawanMac> edMac = DynamicCast<EndDeviceLorawanMac>(lnd->GetMac());
        edMac->SetMType(LorawanMacHeader::CONFIRMED_DATA_UP);
    }

    NodeContainer gateways;
    gateways.Create(1);
    mobility.Install(gateways);

    phyHelper.SetDeviceType(LoraPhyHelper::GW);
    macHelper.SetDeviceType(LorawanMacHelper::GW);
    NetDeviceContainer gwNetDevices = helper.Install(phyHelper, macHelper, gateways);

    LorawanMacHelper::SetSpreadingFactorsUp(g_endDevices, gateways, channel);

    // --- Network Server + P2P backhaul --- whatever this is bruh
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
    nsHelper.SetEndDevices(g_endDevices);
    nsHelper.Install(networkServer);

    ForwarderHelper forwarderHelper;
    forwarderHelper.Install(gateways);

    for (uint32_t i = 0; i < gateways.GetN(); i++)
    {
        Ptr<LoraNetDevice> lnd = DynamicCast<LoraNetDevice>(gateways.Get(i)->GetDevice(0));
        Ptr<GatewayLorawanMac> gwMac = DynamicCast<GatewayLorawanMac>(lnd->GetMac());
        gwMac->TraceConnectWithoutContext("ReceivedPacket", MakeCallback(&OnGatewayUplinkReceived));
    }

    // ED MAC + PHY loss callbacks
    for (uint32_t i = 0; i < g_endDevices.GetN(); i++)
    {
        Ptr<LoraNetDevice> lnd = DynamicCast<LoraNetDevice>(g_edNetDevices.Get(i));
        Ptr<EndDeviceLorawanMac> edMac = DynamicCast<EndDeviceLorawanMac>(lnd->GetMac());
        Ptr<EndDeviceLoraPhy> edPhy = DynamicCast<EndDeviceLoraPhy>(lnd->GetPhy());

        edMac->TraceConnectWithoutContext("ReceivedPacket", MakeCallback(&OnEndDeviceMacReceive));
        edPhy->TraceConnectWithoutContext("LostPacketBecauseWrongSpreadingFactor",
                                          MakeCallback(&OnLostWrongSf));
        edPhy->TraceConnectWithoutContext("LostPacketBecauseUnderSensitivity",
                                          MakeCallback(&OnLostUnderSensitivity));
    }

    PeriodicSenderHelper senderHelper;
    senderHelper.SetPeriod(Seconds(sendPeriod));
    senderHelper.SetPacketSize(10);
    ApplicationContainer apps = senderHelper.Install(g_endDevices);
    for (uint32_t i = 0; i < apps.GetN(); i++)
    {
        apps.Get(i)->SetStartTime(Seconds(2 + i * 5));
    }

    NS_LOG_UNCOND("\n=== Initial Device State ===");
    for (uint32_t i = 0; i < g_edNetDevices.GetN(); i++)
    {
        Ptr<LoraNetDevice> lnd = DynamicCast<LoraNetDevice>(g_edNetDevices.Get(i));
        Ptr<EndDeviceLorawanMac> mac = DynamicCast<EndDeviceLorawanMac>(lnd->GetMac());
        uint32_t addr = mac->GetDeviceAddress().Get();
        uint8_t dr = mac->GetDataRate();
        uint8_t sf = DataRateToSf(dr);

        PidState s;
        s.sf = sf;
        s.txPower = P_INITIAL;
        g_pidState[addr] = s;

        NS_LOG_UNCOND("  ED " << i << " addr=0x" << std::hex << addr << std::dec << " SF="
                              << (int)sf << " DR=" << (int)dr << " TxPow=" << P_INITIAL << " dBm");
    }

    PrintDevicePositions(gateways, "Gateway");
    PrintDevicePositions(g_endDevices, "End Device");

    Simulator::Stop(Seconds(simTime));
    NS_LOG_UNCOND("\nRunning simulation for " << simTime << " s  (send period=" << sendPeriod
                                              << " s) ...\n");
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_UNCOND("\n=== Final PID State ===");
    for (auto& [addr, s] : g_pidState)
    {
        int idx = FindDeviceIndex(addr);
        NS_LOG_UNCOND("  ED " << idx << " addr=0x" << std::hex << addr << std::dec
                              << "  SF=" << (int)s.sf << "  TxPow=" << s.txPower << " dBm");
    }
    NS_LOG_UNCOND("\nSimulation finished!");
    return 0;
}
