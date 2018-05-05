#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "netroad-util.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NETROAD_DYNAMIC_SINGLE");

std::vector<struct APInfo> aps;
Ptr<BulkSendApplication> sender;
struct APInfo ap = APInfo(NULL, Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"));

Ptr<StaWifiMac> staWifiMac = NULL;

NodeContainer staNodes;
NetDeviceContainer ap2staDevs;
ApplicationContainer container;

double velocity = 10;
uint32_t nBusPerRoad = 3;

static void IfAssoc (Mac48Address address);
static void IfMonitorSnifferRx	(Ptr<const Packet> packet,
	uint16_t channelFreqMhz, uint16_t channelNumber, uint32_t rate,
	bool isShortPreamble, double signalDbm, double noiseDbm);

static void CourseChanged(Ptr<const MobilityModel> model);
static void CheckHorizonPosition(Ptr<Node> node);

static bool CompareAP (APStats a, APStats b) { return (a.m_rank > b.m_rank); }

static uint32_t getIndexByMac (const std::vector<struct APInfo>& aps, const Mac48Address mac);

static void GuidedAssociate(){
	std::vector<APStats> stats;
	for (uint32_t i = 0; i < ap2staDevs.GetN (); i++) {
		APStats s = CalculateApStats(ap2staDevs.Get(i), staNodes.Get(0));
		if (s.m_time > 1 && s.m_throughput > 1000000) {
			s.m_rank = s.m_throughput / m_B / 100 +  s.m_time;
			stats.push_back(s);
		}
	}

	std::sort (stats.begin(), stats.end(), CompareAP);

	NS_LOG_INFO ("avalialbe");
	for(uint32_t i=0; i<stats.size(); i++){
		NS_LOG_INFO ("mac:" << stats[i].m_mac << ", thruput:" << stats[i].m_throughput << ", time:" << stats[i].m_time << ", rank:" << stats[i].m_rank);
	}

	if(ap.device == NULL){
		uint32_t newIdx = getIndexByMac(aps, stats[0].m_mac);
		Simulator::ScheduleWithContext(staNodes.Get (0)->GetId (), Seconds(0.5), &StaWifiMac::SetNewAssociation, staWifiMac,
			aps[newIdx].device->GetMac ()->GetAddress(), aps[newIdx].device->GetPhy ()->GetChannelNumber ());
		Simulator::Schedule (Seconds (2.0), &GuidedAssociate);
		return;
	}

	for(uint32_t i=0; i<stats.size(); i++){
		if(stats[0].m_mac == ap.device->GetMac ()->GetAddress()) {
			Simulator::Schedule (Seconds (2.0), &GuidedAssociate);
			return;
		}
	}

	if (stats.size() > 0 && stats[0].m_mac != ap.device->GetMac ()->GetAddress()) {
			uint32_t newIdx = getIndexByMac(aps, stats[0].m_mac);

			Simulator::ScheduleWithContext(staNodes.Get (0)->GetId (), Seconds(0.5), &StaWifiMac::SetNewAssociation, staWifiMac,
				aps[newIdx].device->GetMac ()->GetAddress(), aps[newIdx].device->GetPhy ()->GetChannelNumber ());
	}

	Simulator::Schedule (Seconds (2.0), &GuidedAssociate);
}

int main(int argc, char* argv[]){

	Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (53.31));
  Config::SetDefault ("ns3::LogDistancePropagationLossModel::Exponent", DoubleValue (3));

	Packet::EnablePrinting ();
	uint32_t nApsH = nBusPerRoad;
	uint32_t nAps = nApsH;
	bool manual = true;

  CommandLine cmd;
	cmd.Parse(argc, argv);

  LogComponentEnable("NETROAD_DYNAMIC_SINGLE", LOG_LEVEL_ALL);
  LogComponentEnable("NETROAD_UTIL", LOG_LEVEL_ALL);
	// LogComponentEnable("YansWifiChannel", LOG_LEVEL_ALL);
	// LogComponentEnable("StaWifiMac", LOG_LEVEL_ALL);

  NS_LOG_INFO ("create nodes");

	NodeContainer srvNodes, swNodes, apNodesH, apNodes;
  srvNodes.Create(1);
  swNodes.Create(1);
	apNodes.Create(nAps);
  staNodes.Create(1);

	for(uint32_t i = 0; i < nApsH; i ++) {
		apNodesH.Add(apNodes.Get(i));
	}

  NS_LOG_INFO ("mobility");

  MobilityHelper mobility;
  mobility.SetPositionAllocator(
		"ns3::GridPositionAllocator",
		"MinX", DoubleValue(0.0),
		"MinY", DoubleValue(0.0),
		"DeltaX", DoubleValue(5.0),
		"DeltaY", DoubleValue(5.0),
		"GridWidth", UintegerValue(5),
		"LayoutType", StringValue("RowFirst"));

	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(srvNodes);
	mobility.Install(swNodes);
	mobility.Install(staNodes);
	SetPosition(srvNodes.Get(0), Vector(0, 0, 0));
	SetPosition(swNodes.Get(0), Vector(5, 5, 0));
	SetPosition(staNodes.Get(0), Vector(210, 210, 0));

	mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
	mobility.Install(apNodes);

	for(uint32_t i = 0; i < nApsH; i ++) {
		SetPositionVelocity(apNodesH.Get(i), Vector(i * 420 / nBusPerRoad, 210, 0), Vector(velocity, 0, 0));
		RegisterCourseChangeCallback(apNodesH.Get(i), MakeCallback(&CourseChanged));
		Simulator::Schedule (Seconds (1.0), &CheckHorizonPosition, apNodesH.Get(i));
	}

  NS_LOG_INFO ("install stack");

  LinuxStackHelper stack;
  stack.Install(srvNodes);
  stack.Install(swNodes);
  stack.Install(apNodes);
  stack.Install(staNodes);

  DceManagerHelper dceManager;
  dceManager.SetTaskManagerAttribute (
		"FiberManagerType",
		StringValue("UcontextFiberManager"));

  dceManager.SetNetworkStack (
		"ns3::LinuxSocketFdFactory",
		"Library", StringValue ("liblinux.so"));

  dceManager.Install(srvNodes);
  dceManager.Install(swNodes);
  dceManager.Install(apNodes);
  dceManager.Install(staNodes);

  NS_LOG_INFO ("install devices");

  NetDeviceContainer srv2swDevs, sw2srvDevs, sw2apDevs, ap2swDevs,sta2apDevs;

  PointToPointHelper p2p;
	p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevs = p2p.Install(NodeContainer(srvNodes.Get(0), swNodes.Get(0)));
  srv2swDevs.Add(p2pDevs.Get(0));
	sw2srvDevs.Add(p2pDevs.Get(1));

  for(uint32_t i = 0; i<nAps; i++) {
    p2pDevs = p2p.Install(NodeContainer(apNodes.Get(i), swNodes.Get(0)));
		ap2swDevs.Add(p2pDevs.Get(0));
		sw2apDevs.Add(p2pDevs.Get(1));
  }

  p2p.EnablePcapAll ("netroad-dynamic-single-p2p");

  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();

	Ptr<YansWifiChannel> channel = wifiChannel.Create();

	wifiPhy.SetChannel(wifiChannel.Create());
	wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");

	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
	WifiHelper wifi = WifiHelper::Default();
	wifi.SetStandard(WIFI_PHY_STANDARD_80211g);

  Ssid ssid = Ssid("NETROAD");
	wifiMac.SetType(
		"ns3::ApWifiMac",
		"Ssid", SsidValue(ssid),
		"EnableBeaconJitter", BooleanValue(true)
	);

	for(uint32_t i = 0; i < nAps; i++) {
		wifiPhy.Set("ChannelNumber", UintegerValue(1 + (i % 3) * 5));
		ap2staDevs.Add(wifi.Install(wifiPhy, wifiMac, apNodes.Get(i)));

		Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice> (ap2staDevs.Get (i));
		NS_LOG_INFO ("ap mac:" << wifiDev->GetMac ()->GetAddress() << ", channel: " << wifiDev->GetPhy ()->GetChannelNumber () << ", frequency: " << wifiDev->GetPhy ()->GetFrequency ());
	}

	if(manual) {
		wifiMac.SetType(
			"ns3::StaWifiMac",
			"Ssid", SsidValue (ssid),
			"ScanType", EnumValue(StaWifiMac::NOTSUPPORT),
			"ActiveProbing", BooleanValue(false),
			"MaxMissedBeacons", UintegerValue (10086));
	} else {
		wifiMac.SetType(
			"ns3::StaWifiMac",
			"Ssid", SsidValue (ssid),
			"ScanType", EnumValue(StaWifiMac::ACTIVE),
			"ActiveProbing", BooleanValue(true),
			"MaxMissedBeacons", UintegerValue (8));
	}

	for(uint32_t i = 0; i < 1; i++) {
		wifiPhy.Set("ChannelNumber", UintegerValue(6));
		sta2apDevs.Add(wifi.Install(wifiPhy, wifiMac, staNodes.Get(0)));

		Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice> (sta2apDevs.Get (i));
		NS_LOG_INFO ("sta: " << wifiDev->GetMac ()->GetAddress() << ", channel: " << wifiDev->GetPhy ()->GetChannelNumber ());
	}

	RegisterAssocCallback(sta2apDevs.Get(0), MakeCallback(&IfAssoc));
	// RegisterMonitorSnifferRxCallback(sta2apDevs.Get(0), MakeCallback(&IfMonitorSnifferRx));

	wifiPhy.EnablePcapAll("netroad-dynamic-single-wifi", false);

  NS_LOG_INFO ("assign ip");

  SetIpv4Address(srv2swDevs.Get(0), "10.1.1.1", "/24");
  SetIpv4Address(sw2srvDevs.Get(0), "10.1.1.2", "/24");

  for(uint32_t i = 0; i < nAps; i++)	{
    SetIpv4Address(sw2apDevs.Get(i), BuildIpv4Address(10, 1, i+2, 1), Ipv4Mask("/24"));
    SetIpv4Address(ap2swDevs.Get(i), BuildIpv4Address(10, 1, i+2, 2), Ipv4Mask("/24"));

    SetIpv4Address(ap2staDevs.Get(i), BuildIpv4Address(192, 168, i+1, 1), Ipv4Mask("/24"));

    Ipv4Address gw = BuildIpv4Address(192, 168, i+1, 1);
    Ipv4Address ip = BuildIpv4Address(192, 168, i+1, 2);
    Ipv4Address net = ip.CombineMask(Ipv4Mask("/24"));
		Ipv4Address broadcast = BuildIpv4Address(192, 168, i+1, 255);

    Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(ap2staDevs.Get(i));
    aps.push_back(APInfo(wifiDev, gw, ip, net, broadcast));
  }

  NS_LOG_INFO ("routing");

	RouteAddDefaultWithGatewayIfIndex(srvNodes.Get(0), GetIpv4Address (sw2srvDevs.Get(0)), 0);
	RouteAddWithNetworkGatewayIfIndex(swNodes.Get(0), Ipv4Address("10.1.1.0"), Ipv4Address("10.1.1.2"), 0);

	for(uint32_t i = 0; i < nAps; i ++){
		RouteAddWithNetworkGatewayIfIndex(swNodes.Get(0), BuildIpv4Address(192, 168, i+1, 0), GetIpv4Address (ap2swDevs.Get(i)), i+1);
		RouteAddWithNetworkGatewayIfIndex(apNodes.Get(i), Ipv4Address("10.1.1.0"), GetIpv4Address (sw2apDevs.Get(i)), 0);
	}

	NS_LOG_INFO ("handoff");

	Ptr<WifiNetDevice> wifiNetDevice1 = DynamicCast<WifiNetDevice> (sta2apDevs.Get (0));
	staWifiMac = DynamicCast<StaWifiMac> (wifiNetDevice1->GetMac ());

	NS_LOG_INFO ("sysctl");

	stack.SysctlSet (srvNodes, ".net.ipv6.conf.all.disable_ipv6", "1");
	stack.SysctlSet (staNodes, ".net.ipv6.conf.all.disable_ipv6", "1");
	stack.SysctlSet (srvNodes, ".net.ipv6.conf.default.disable_ipv6", "1");
	stack.SysctlSet (staNodes, ".net.ipv6.conf.default.disable_ipv6", "1");

	stack.SysctlSet (srvNodes, ".net.mptcp.mptcp_debug", "1");
	stack.SysctlSet (staNodes, ".net.mptcp.mptcp_debug", "1");
	stack.SysctlSet (srvNodes, ".net.mptcp.mptcp_scheduler", "roundrobin");
	stack.SysctlSet (staNodes, ".net.mptcp.mptcp_scheduler", "roundrobin");
	stack.SysctlSet (srvNodes, ".net.mptcp.mptcp_path_manager", "fullmesh");
	stack.SysctlSet (staNodes, ".net.mptcp.mptcp_path_manager", "fullmesh");
  stack.SysctlSet (srvNodes, ".net.mptcp.mptcp_checksum", "0");
	stack.SysctlSet (staNodes, ".net.mptcp.mptcp_checksum", "0");

	NS_LOG_INFO ("apps");

	PacketSinkHelper sinkHelper("ns3::LinuxTcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9999));
	ApplicationContainer sinkApp = sinkHelper.Install(srvNodes.Get(0));
	sinkApp.Start(Seconds(0.1));

	BulkSendHelper bulk = BulkSendHelper("ns3::LinuxTcpSocketFactory", InetSocketAddress("10.1.1.1", 9999));
	bulk.SetAttribute("MaxBytes", UintegerValue(0));
	bulk.SetAttribute("SendSize", UintegerValue(1400));
	container = bulk.Install(staNodes.Get(0));
	container.Start(Seconds(10086));

	sender = DynamicCast<BulkSendApplication> (container.Get (0));

	if(manual) {
		Simulator::Schedule (Seconds (2.0), &GuidedAssociate);
	}


	NS_LOG_INFO ("animation");
	// AnimationInterface anim("netroad-dynamic-single.xml");

	Simulator::Stop(Seconds(40.0));
	Simulator::Run();
	Simulator::Destroy();
	return 0;
}

static void
IfAssoc (Mac48Address address) {
  NS_LOG_INFO(Simulator::Now() << " if1: " << address);
	if(ap.device != NULL && address == ap.device->GetMac ()->GetAddress()){
		return;
	}

	uint32_t newIdx = getIndexByMac(aps, address);
	NS_LOG_INFO (aps[newIdx].m_ip);
	double timeOffset = UpdateNewAp (staNodes.Get(0), 0, ap, aps[newIdx]);
	ap = aps[newIdx];

	sender->Pause();
	sender->SetStartTime(Seconds(timeOffset));
	sender->Restart();

	NS_LOG_INFO(Simulator::Now() << " if: ok");
}

static void
IfMonitorSnifferRx	(Ptr<const Packet> packet, uint16_t channelFreqMhz, uint16_t channelNumber, uint32_t rate, 	bool isShortPreamble, double signalDbm, double noiseDbm) {
	NS_LOG_INFO ("noise" << noiseDbm);
}

static uint32_t getIndexByMac (const std::vector<struct APInfo>& aps, const Mac48Address mac) {
	for(uint32_t i = 0; i < aps.size(); i ++) {
		if(aps[i].device->GetMac ()->GetAddress() == mac) {
			return i;
		}
	}
	return aps.size();
}

static void CourseChanged(Ptr<const MobilityModel> model) {
	Ptr<Node> node = model->GetObject<Node>();
	Vector pos = model->GetPosition ();
	NS_LOG_INFO (node->GetId() << " x:" << pos.x << ",y:" << pos.y << ", time:" << Simulator::Now());
}

static void CheckHorizonPosition(Ptr<Node> node){
	Ptr <ConstantVelocityMobilityModel> model = node->GetObject<ConstantVelocityMobilityModel>();
	Vector pos = model->GetPosition ();
	if(abs(pos.x - 420) < 1) {
		SetPositionVelocity(node, Vector3D (0, 210, 0), Vector(velocity, 0, 0));
	}
	Simulator::Schedule (Seconds (1.0), &CheckHorizonPosition, node);
}
