/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * NIST-developed software is provided by NIST as a public
 * service. You may use, copy and distribute copies of the software in
 * any medium, provided that you keep intact this entire notice. You
 * may improve, modify and create derivative works of the software or
 * any portion of the software, and you may copy and distribute such
 * modifications or works. Modified works should carry a notice
 * stating that you changed the software and should note the date and
 * nature of any such change. Please explicitly acknowledge the
 * National Institute of Standards and Technology as the source of the
 * software.
 *
 * NIST-developed software is expressly provided "AS IS." NIST MAKES
 * NO WARRANTY OF ANY KIND, EXPRESS, IMPLIED, IN FACT OR ARISING BY
 * OPERATION OF LAW, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTY OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * NON-INFRINGEMENT AND DATA ACCURACY. NIST NEITHER REPRESENTS NOR
 * WARRANTS THAT THE OPERATION OF THE SOFTWARE WILL BE UNINTERRUPTED
 * OR ERROR-FREE, OR THAT ANY DEFECTS WILL BE CORRECTED. NIST DOES NOT
 * WARRANT OR MAKE ANY REPRESENTATIONS REGARDING THE USE OF THE
 * SOFTWARE OR THE RESULTS THEREOF, INCLUDING BUT NOT LIMITED TO THE
 * CORRECTNESS, ACCURACY, RELIABILITY, OR USEFULNESS OF THE SOFTWARE.
 *
 * You are solely responsible for determining the appropriateness of
 * using and distributing the software and you assume all risks
 * associated with its use, including but not limited to the risks and
 * costs of program errors, compliance with applicable laws, damage to
 * or loss of data, programs or equipment, and the unavailability or
 * interruption of operation. This software is not intended to be used
 * in any situation where a failure could cause risk of injury or
 * damage to property. The software developed by NIST employees is not
 * subject to copyright protection within the United States.
 */

#include "ns3/applications-module.h"
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include <bits/stdc++.h>
#include <cfloat>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <time.h>

#define shard_num 3
#define node_num 10

using namespace ns3;
using namespace std;

struct NodeData
{
    double x, y;
    int cluster;
    uint32_t node_ID1;
    uint32_t node_ID2;
    int node_type;
    Ipv4Address node_Ip1;
    Ipv4Address node_Ip2;
    double minDist;

    NodeData(double x, double y, int t, uint32_t node1, uint32_t node2 = -1)
        : x(x), y(y), cluster(-1), node_ID1(node1), node_ID2(node2), node_type(t), minDist(__DBL_MAX__)
    {
    }
    double distance(NodeData p)
    {
        return (p.x - x) * (p.x - x) + (p.y - y) * (p.y - y);
    }
};

void kMeansClustering(vector<NodeData> *points)
{
    vector<NodeData> centroids;
    srand(time(0));
    for (int i = 0; i < shard_num; ++i)
    {
        for (vector<NodeData>::iterator x = points->begin(); x != points->end(); ++x)
        {
            if (x->node_type == 1 && x->cluster == -1)
            {
                centroids.push_back(*x);
            }
        }
        for (vector<NodeData>::iterator c = begin(centroids); c != end(centroids); ++c)
        {
            int clusterId = c - begin(centroids);
            for (vector<NodeData>::iterator it = points->begin(); it != points->end(); ++it)
            {
                NodeData p = *it;
                double dist = c->distance(p);
                if (dist < p.minDist)
                {
                    p.minDist = dist;
                    p.cluster = clusterId;
                }
                *it = p;
            }
        }
    }
    vector<int> nPoints;
    vector<double> sumX, sumY;

    for (int j = 0; j < shard_num; ++j)
    {
        nPoints.push_back(0);
        sumX.push_back(0.0);
        sumY.push_back(0.0);
    }

    for (vector<NodeData>::iterator it = points->begin(); it != points->end(); ++it)
    {
        int clusterId = it->cluster;
        nPoints[clusterId] += 1;
        sumX[clusterId] += it->x;
        sumY[clusterId] += it->y;

        it->minDist = __DBL_MAX__;
    }

    for (vector<NodeData>::iterator c = begin(centroids); c != end(centroids); ++c)
    {
        int clusterId = c - begin(centroids);
        c->x = sumX[clusterId] / nPoints[clusterId];
        c->y = sumY[clusterId] / nPoints[clusterId];
    }

    cout << "node_type \t node_id \t node_ip_address \t x \t y \t shard" << endl;

    for (vector<NodeData>::iterator it = points->begin(); it != points->end(); ++it)
    {
        cout << it->node_type << " \t " << it->node_ID1 << " \t " << it->node_Ip1 << " \t " << it->x << " \t " << it->y << " \t " << it->cluster << endl;
        if (it->node_type == 0)
        {
            cout << it->node_type << " \t " << it->node_ID2 << " \t " << it->node_Ip2 << " \t " << it->x << " \t " << it->y << " \t " << it->cluster << endl;
        }
    }
}

// This trace will log packet transmissions and receptions from the application
// layer.  The parameter 'localAddrs' is passed to this trace in case the
// address passed by the trace is not set (i.e., is '0.0.0.0' or '::').  The
// trace writes to a file stream provided by the first argument; by default,
// this trace file is 'UePacketTrace.tr'
void UePacketTrace(Ptr<OutputStreamWrapper> stream, const Address &localAddrs, std::string context, Ptr<const Packet> p, const Address &srcAddrs, const Address &dstAddrs)
{
    std::ostringstream oss;
    *stream->GetStream() << Simulator::Now().GetNanoSeconds() / (double)1e9 << "\t" << context << "\t" << p->GetSize() << "\t";
    if (InetSocketAddress::IsMatchingType(srcAddrs))
    {
        oss << InetSocketAddress::ConvertFrom(srcAddrs).GetIpv4();
        if (!oss.str().compare("0.0.0.0")) //srcAddrs not set
        {
            *stream->GetStream() << Ipv4Address::ConvertFrom(localAddrs) << ":" << InetSocketAddress::ConvertFrom(srcAddrs).GetPort() << "\t" << InetSocketAddress::ConvertFrom(dstAddrs).GetIpv4() << ":" << InetSocketAddress::ConvertFrom(dstAddrs).GetPort() << std::endl;
        }
        else
        {
            oss.str("");
            oss << InetSocketAddress::ConvertFrom(dstAddrs).GetIpv4();
            if (!oss.str().compare("0.0.0.0")) //dstAddrs not set
            {
                *stream->GetStream() << InetSocketAddress::ConvertFrom(srcAddrs).GetIpv4() << ":" << InetSocketAddress::ConvertFrom(srcAddrs).GetPort() << "\t" << Ipv4Address::ConvertFrom(localAddrs) << ":" << InetSocketAddress::ConvertFrom(dstAddrs).GetPort() << std::endl;
            }
            else
            {
                *stream->GetStream() << InetSocketAddress::ConvertFrom(srcAddrs).GetIpv4() << ":" << InetSocketAddress::ConvertFrom(srcAddrs).GetPort() << "\t" << InetSocketAddress::ConvertFrom(dstAddrs).GetIpv4() << ":" << InetSocketAddress::ConvertFrom(dstAddrs).GetPort() << std::endl;
            }
        }
    }
    else if (Inet6SocketAddress::IsMatchingType(srcAddrs))
    {
        oss << Inet6SocketAddress::ConvertFrom(srcAddrs).GetIpv6();
        if (!oss.str().compare("::")) //srcAddrs not set
        {
            *stream->GetStream() << Ipv6Address::ConvertFrom(localAddrs) << ":" << Inet6SocketAddress::ConvertFrom(srcAddrs).GetPort() << "\t" << Inet6SocketAddress::ConvertFrom(dstAddrs).GetIpv6() << ":" << Inet6SocketAddress::ConvertFrom(dstAddrs).GetPort() << std::endl;
        }
        else
        {
            oss.str("");
            oss << Inet6SocketAddress::ConvertFrom(dstAddrs).GetIpv6();
            if (!oss.str().compare("::")) //dstAddrs not set
            {
                *stream->GetStream() << Inet6SocketAddress::ConvertFrom(srcAddrs).GetIpv6() << ":" << Inet6SocketAddress::ConvertFrom(srcAddrs).GetPort() << "\t" << Ipv6Address::ConvertFrom(localAddrs) << ":" << Inet6SocketAddress::ConvertFrom(dstAddrs).GetPort() << std::endl;
            }
            else
            {
                *stream->GetStream() << Inet6SocketAddress::ConvertFrom(srcAddrs).GetIpv6() << ":" << Inet6SocketAddress::ConvertFrom(srcAddrs).GetPort() << "\t" << Inet6SocketAddress::ConvertFrom(dstAddrs).GetIpv6() << ":" << Inet6SocketAddress::ConvertFrom(dstAddrs).GetPort() << std::endl;
            }
        }
    }
    else
    {
        *stream->GetStream() << "Unknown address type!" << std::endl;
    }
}

void monitor(std::string context, uint16_t cellId, uint16_t rnti, double rsrp, double sinr, uint8_t componentCarrierId)
{
    std::cerr << "\n";
    std::cerr << "context: " << context << "\n";
    std::cerr << "sinr" << sinr << "\n";
    std::cerr << "\n";
}

NS_LOG_COMPONENT_DEFINE("LteSlInCovrgCommMode1");

int main(int argc, char *argv[])
{
    Time simTime = Seconds(6);
    bool enableNsLogs = false;
    bool useIPv6 = false;

    CommandLine cmd;
    cmd.AddValue("simTime", "Total duration of the simulation", simTime);
    cmd.AddValue("enableNsLogs", "Enable ns-3 logging (debug builds)", enableNsLogs);
    cmd.AddValue("useIPv6", "Use IPv6 instead of IPv4", useIPv6);
    cmd.Parse(argc, argv);

    // Configure the scheduler
    Config::SetDefault("ns3::RrSlFfMacScheduler::Itrp", UintegerValue(0));
    //The number of RBs allocated per UE for Sidelink
    Config::SetDefault("ns3::RrSlFfMacScheduler::SlGrantSize", UintegerValue(5));

    //Set the frequency

    Config::SetDefault("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue(100));
    Config::SetDefault("ns3::LteUeNetDevice::DlEarfcn", UintegerValue(100));
    Config::SetDefault("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue(18100));
    Config::SetDefault("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(50));
    Config::SetDefault("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue(50));

    // Set error models
    Config::SetDefault("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue(true));
    Config::SetDefault("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue(true));
    Config::SetDefault("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue(false));

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();
    // parse again so we can override input file default values via command line
    cmd.Parse(argc, argv);

    if (enableNsLogs)
    {
        LogLevel logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);

        LogComponentEnable("LteUeRrc", logLevel);
        LogComponentEnable("LteUeMac", logLevel);
        LogComponentEnable("LteSpectrumPhy", logLevel);
        LogComponentEnable("LteUePhy", logLevel);
        LogComponentEnable("LteEnbPhy", logLevel);
    }

    //Set the UEs power in dBm
    Config::SetDefault("ns3::LteUePhy::TxPower", DoubleValue(23.0));
    //Set the eNBs power in dBm
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(30.0));

    //Sidelink bearers activation time
    Time slBearersActivationTime = Seconds(2.0);

    //Create the helpers
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();

    //Create and set the EPC helper
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    ////Create Sidelink helper and set lteHelper
    Ptr<LteSidelinkHelper> proseHelper = CreateObject<LteSidelinkHelper>();
    proseHelper->SetLteHelper(lteHelper);
    //Set pathloss model
    lteHelper->SetAttribute("PathlossModel", StringValue("ns3::Cost231PropagationLossModel"));

    //Enable Sidelink
    lteHelper->SetAttribute("UseSidelink", BooleanValue(true));

    //Sidelink Round Robin scheduler
    lteHelper->SetSchedulerType("ns3::RrSlFfMacScheduler");

    //Create nodes (eNb + UEs)
    NodeContainer enbNode;
    enbNode.Create(1);
    NS_LOG_UNCOND("eNb node id = [" << enbNode.Get(0)->GetId() << "]");

    NodeContainer ueNodes;
    NodeContainer ueClients;
    NodeContainer ueServers;
    std::vector<NodeContainer> uePairs(node_num);

    for (uint32_t i = 0; i < node_num; ++i)
    {
        NodeContainer pair;
        pair.Create(2);

        ueClients.Add(pair.Get(0));
        ueServers.Add(pair.Get(1));

        ueNodes.Add(pair);

        uePairs[i].Add(pair);
    }

    NodeContainer cuNodes;
    cuNodes.Create(shard_num);

    for (int i = 0; i < shard_num; i++)
    {
        NS_LOG_UNCOND("CU" << i + 1 << "node id = [" << cuNodes.Get(i)->GetId() << "]");
    }

    //Position of the nodes
    Ptr<ListPositionAllocator> positionAllocEnb = CreateObject<ListPositionAllocator>();
    positionAllocEnb->Add(Vector(0.0, 0.0, 0.0));

    Ptr<ListPositionAllocator> positionAllocUe[node_num];
    time_t t;
    srand((unsigned)time(&t));
    int x1, y1, x2, y2;
    vector<NodeData> points;
    for (int i = 0; i < node_num; i++)
    {
        x1 = rand() % 100;
        y1 = rand() % 100;

        x2 = rand() % 100;
        y2 = rand() % 100;

        points.push_back(NodeData((x1 + x2) / 2, (y1 + y2) / 2, 0, uePairs[i].Get(0)->GetId(), uePairs[i].Get(1)->GetId()));

        positionAllocUe[i] = CreateObject<ListPositionAllocator>();
        positionAllocUe[i]->Add(Vector(x1, y1, 0.0));
        positionAllocUe[i]->Add(Vector(x2, y2, 0.0));
    }

    Ptr<ListPositionAllocator> positionAllocCu[shard_num];
    for (int i = 0; i < shard_num; i++)
    {
        x1 = rand() % 100;
        y1 = rand() % 100;
        points.push_back(NodeData(x1, y1, 1, cuNodes.Get(i)->GetId()));
        positionAllocCu[i] = CreateObject<ListPositionAllocator>();
        positionAllocCu[i]->Add(Vector(x1, y1, 0.0));
    }

    //Install mobility
    MobilityHelper mobilityeNodeB;
    mobilityeNodeB.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityeNodeB.SetPositionAllocator(positionAllocEnb);
    mobilityeNodeB.Install(enbNode);

    MobilityHelper mobilityUe[node_num];
    for (int i = 0; i < node_num; i++)
    {
        mobilityUe[i].SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobilityUe[i].SetPositionAllocator(positionAllocUe[i]);
        mobilityUe[i].Install(uePairs[i]);
    }

    MobilityHelper mobilityCu[shard_num];
    for (int i = 0; i < shard_num; i++)
    {
        mobilityCu[i].SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobilityCu[i].SetPositionAllocator(positionAllocCu[i]);
        mobilityCu[i].Install(cuNodes.Get(i));
    }

    //Install LTE devices to the nodes and fix the random number stream
    int64_t randomStream = 1;
    NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice(enbNode);
    randomStream += lteHelper->AssignStreams(enbDevs, randomStream);
    NetDeviceContainer ueDevs = lteHelper->InstallUeDevice(ueNodes);
    randomStream += lteHelper->AssignStreams(ueDevs, randomStream);
    NetDeviceContainer cuDevs = lteHelper->InstallUeDevice(cuNodes);
    randomStream += lteHelper->AssignStreams(cuDevs, randomStream);

    //Configure Sidelink Pool
    Ptr<LteSlEnbRrc> enbSidelinkConfiguration = CreateObject<LteSlEnbRrc>();
    enbSidelinkConfiguration->SetSlEnabled(true);

    //Preconfigure pool for the group
    LteRrcSap::SlCommTxResourcesSetup pool;

    pool.setup = LteRrcSap::SlCommTxResourcesSetup::SCHEDULED;
    //BSR timers
    pool.scheduled.macMainConfig.periodicBsrTimer.period = LteRrcSap::PeriodicBsrTimer::sf16;
    pool.scheduled.macMainConfig.retxBsrTimer.period = LteRrcSap::RetxBsrTimer::sf640;
    //MCS
    pool.scheduled.haveMcs = true;
    pool.scheduled.mcs = 16;
    //resource pool
    LteSlResourcePoolFactory pfactory;
    pfactory.SetHaveUeSelectedResourceConfig(false); //since we want eNB to schedule

    //Control
    pfactory.SetControlPeriod("sf40");
    pfactory.SetControlBitmap(0x00000000FF); //8 subframes for PSCCH
    pfactory.SetControlOffset(0);
    pfactory.SetControlPrbNum(22);
    pfactory.SetControlPrbStart(0);
    pfactory.SetControlPrbEnd(49);

    //Data: The ns3::RrSlFfMacScheduler is responsible to handle the parameters

    pool.scheduled.commTxConfig = pfactory.CreatePool();

    uint32_t groupL2Address = 255;

    enbSidelinkConfiguration->AddPreconfiguredDedicatedPool(groupL2Address, pool);
    lteHelper->InstallSidelinkConfiguration(enbDevs, enbSidelinkConfiguration);

    //pre-configuration for the UEs
    Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc>();
    ueSidelinkConfiguration->SetSlEnabled(false); //changed
    LteRrcSap::SlPreconfiguration preconfiguration;
    ueSidelinkConfiguration->SetSlPreconfiguration(preconfiguration);
    lteHelper->InstallSidelinkConfiguration(ueDevs, ueSidelinkConfiguration);

    NodeContainer allNodes;
    allNodes.Add(ueNodes);
    allNodes.Add(cuNodes);

    NetDeviceContainer allDevs;
    allDevs.Add(ueDevs);
    allDevs.Add(cuDevs);

    InternetStackHelper internet;
    internet.Install(allNodes);
    Ipv4Address groupAddress4("225.0.0.0"); //use multicast address as destination
    Ipv6Address groupAddress6("ff0e::1");   //use multicast address as destination
    Address remoteAddress;
    Address localAddress;
    Ptr<LteSlTft> tft;
    //Install the IP stack on the UEs and assign IP address
    Ipv4InterfaceContainer allIpIface;
    allIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(allDevs));

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    for (uint32_t u = 0; u < allNodes.GetN(); ++u)
    {

        Ptr<Node> allNode = allNodes.Get(u);
        // Set the default gateway for the UEIpv4Address
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(allNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }
    remoteAddress = InetSocketAddress(groupAddress4, 8000);
    localAddress = InetSocketAddress(Ipv4Address::GetAny(), 8000);
    tft = Create<LteSlTft>(LteSlTft::BIDIRECTIONAL, groupAddress4, groupL2Address);

    for (int i = 0; i < node_num; i++)
    {
        points[i].node_Ip1 = uePairs[i].Get(0)->GetObject<Ipv4L3Protocol>()->GetAddress(1, 0).GetLocal();
        points[i].node_Ip2 = uePairs[i].Get(1)->GetObject<Ipv4L3Protocol>()->GetAddress(1, 0).GetLocal();
    }

    for (int i = 0; i < shard_num; i++)
    {
        points[node_num + i].node_Ip1 = cuNodes.Get(i)->GetObject<Ipv4L3Protocol>()->GetAddress(1, 0).GetLocal();
    }

    //Attach each UE to the best available eNB
    lteHelper->Attach(ueDevs);
    lteHelper->Attach(cuDevs);

    kMeansClustering(&points);

    ///*** Configure applications ***///

    //Set Application in the UEs
    OnOffHelper sidelinkClient("ns3::UdpSocketFactory", remoteAddress);
    sidelinkClient.SetConstantRate(DataRate("200b/s"), 200);
    for (int shard = 0; shard < shard_num; ++shard)
    {
        int time = 2;
        for (auto &&uePair : uePairs)
        {
            for (auto &&point : points)
            {
                if (point.cluster == shard && uePair.Get(0)->GetId() == point.node_ID1)
                {
                    ApplicationContainer clientApp = sidelinkClient.Install(uePair.Get(0));
                    clientApp.Start(Seconds(time));
                    clientApp.Stop(simTime);
                    time += 1;
                }
            }
        }
    }

    //onoff application will send the first packet at :
    //(2.9 (App Start Time) + (1600 (Pkt size in bits) / 16000 (Data rate)) = 3.0 sec

    ApplicationContainer serverApps;
    PacketSinkHelper sidelinkSink("ns3::UdpSocketFactory", localAddress);
    serverApps = sidelinkSink.Install(ueServers);
    serverApps.Start(Seconds(2.0));

    proseHelper->ActivateSidelinkBearer(slBearersActivationTime, ueDevs, tft);

    ///*** End of application configuration ***//

    vector<vector<NodeData>> shards(shard_num);
    for (auto &&point : points)
    {
        if (point.node_type == 0)
        {
            shards[point.cluster].push_back(point);
        }
    }

    for (auto &&shard : shards)
    {
        for (auto &&point : shard)
        {
            Config::Connect("/NodeList/" + to_string(point.node_ID2) + "/DeviceList/0/$ns3::LteUeNetDevice/ComponentCarrierMapUe/*/LteUePhy/ReportCurrentCellRsrpSinr", MakeCallback(&monitor));
        }
    }

    NS_LOG_INFO("Enabling Sidelink traces...");
    lteHelper->EnableSidelinkTraces();

    NS_LOG_INFO("Starting simulation...");

    Simulator::Stop(simTime);

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
