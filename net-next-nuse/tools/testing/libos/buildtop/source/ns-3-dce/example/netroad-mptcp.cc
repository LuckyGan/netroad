#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"

#include "netroad-util.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NETROAD_MPTCP");

NodeContainer staNodes;
NetDeviceContainer sta2apDevs;
std::vector<struct APInfo> aps;
struct APInfo ap1 = APInfo(Mac48Address(), Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"));
struct APInfo ap2 = APInfo(Mac48Address(), Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"));
Ssid ssidV = Ssid("NETROAD-V");
Ssid ssidH = Ssid("NETROAD-H");

ApplicationContainer apps;

static void If1Assoc (Mac48Address address);
static void If2Assoc (Mac48Address address);

static void If1MonitorSnifferRx	(Ptr<const Packet> packet,
	uint16_t channelFreqMhz, uint16_t channelNumber, uint32_t rate,
	bool isShortPreamble, double signalDbm, double noiseDbm);

static void If2MonitorSnifferRx	(Ptr<const Packet> packet,
	uint16_t channelFreqMhz, uint16_t channelNumber, uint32_t rate,
	bool isShortPreamble, double signalDbm, double noiseDbm);

int main(int argc, char* argv[]) {
	uint32_t nApsV = 2;
	uint32_t nApsH = 2;

	std::string file = "test.mp4";

	CommandLine cmd;
	cmd.Parse(argc, argv);

	LogComponentEnable("NETROAD_MPTCP", LOG_LEVEL_ALL);
	LogComponentEnable("NETROAD_UTIL", LOG_LEVEL_ALL);


	std::string cmdCP = "cp " + file + " files-0/mytest.mp4";

	int ret = system (cmdCP.c_str());
	chmod ("files-0/mytest.mp4", 0744);

	NS_LOG_INFO ("create nodes");

	NodeContainer srvNodes, swNodes, apNodes, apNodesV, apNodesH;
	srvNodes.Create(1);
	swNodes.Create(1);
	apNodes.Create(nApsV + nApsH);
	staNodes.Create(1);

	for(uint32_t i = 0; i < nApsV; i++) {
		apNodesV.Add(apNodes.Get(i));
	}

	for(uint32_t i = nApsV; i < nApsV+nApsH; i++) {
		apNodesH.Add(apNodes.Get(i));
	}

	NS_LOG_INFO ("mobility");

	MobilityHelper mobility;
	mobility.SetPositionAllocator("ns3::GridPositionAllocator",
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
	SetPosition(srvNodes.Get(0), Vector(40, 40, 0));
	SetPosition(swNodes.Get(0), Vector(80, 80, 0));
	SetPosition(staNodes.Get(0), Vector(300, 300, 0));

	mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
	mobility.Install(apNodes);
	for(uint32_t i = 0; i < nApsV; i++) {
		SetPositionVelocity(apNodesV.Get(i), Vector(270, i * 300, 0), Vector(0, 10, 0));
	}

	for(uint32_t i = 0; i < nApsH; i++) {
		SetPositionVelocity(apNodesH.Get(i), Vector(i * 300, 270, 0), Vector(10, 0, 0));
	}

	NS_LOG_INFO ("install stack");

	LinuxStackHelper stack;
	stack.Install(srvNodes);
	stack.Install(swNodes);
	stack.Install(apNodes);
	stack.Install(staNodes);

	DceManagerHelper dceManager;
	dceManager.SetTaskManagerAttribute ("FiberManagerType",
																			 StringValue("UcontextFiberManager"));

	dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
															"Library", StringValue ("liblinux.so"));
	dceManager.Install(srvNodes);
	dceManager.Install(swNodes);
	dceManager.Install(apNodes);
	dceManager.Install(staNodes);

	NS_LOG_INFO ("install devices");

	NetDeviceContainer srv2swDevs, sw2srvDevs, sw2apDevs, ap2swDevs, ap2staDevs;

	PointToPointHelper p2p;

	NetDeviceContainer p2pDevs = p2p.Install(NodeContainer(srvNodes.Get(0), swNodes.Get(0)));
	srv2swDevs.Add(p2pDevs.Get(0));
	sw2srvDevs.Add(p2pDevs.Get(1));

	for(uint32_t i = 0; i < nApsV+nApsH; i++) {
		p2pDevs = p2p.Install(NodeContainer(apNodes.Get(i), swNodes.Get(0)));
		ap2swDevs.Add(p2pDevs.Get(0));
		sw2apDevs.Add(p2pDevs.Get(1));
	}

	p2p.EnablePcapAll ("netroad-mptcp-p2p");

	YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();

	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
	wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
	wifiPhy.SetChannel(wifiChannel.Create());

	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();

	WifiHelper wifi = WifiHelper::Default();
	wifi.SetRemoteStationManager ("ns3::ArfWifiManager");
	wifi.SetStandard(WIFI_PHY_STANDARD_80211g);

	wifiMac.SetType("ns3::ApWifiMac",
									"Ssid", SsidValue(ssidV));
	for(uint32_t i = 0; i < nApsV; i++) {
		wifiPhy.Set("ChannelNumber", UintegerValue(1 + (i % 3) * 5));
		ap2staDevs.Add(wifi.Install(wifiPhy, wifiMac, apNodesV.Get(i)));
	}

	wifiMac.SetType("ns3::ApWifiMac",
									"Ssid", SsidValue(ssidH));
	for(uint32_t i = 0; i < nApsH; i++) {
		wifiPhy.Set("ChannelNumber", UintegerValue(1 + (i % 3) * 5));
		ap2staDevs.Add(wifi.Install(wifiPhy, wifiMac, apNodesH.Get(i)));
	}

	wifiMac.SetType("ns3::StaWifiMac",
									"Ssid", SsidValue (ssidV),
									"MaxMissedBeacons", UintegerValue (8),
									"ScanType", EnumValue(StaWifiMac::ACTIVE),
									"ActiveProbing", BooleanValue(true));
	sta2apDevs.Add(wifi.Install(wifiPhy, wifiMac, staNodes.Get(0)));

	RegisterAssocCallback(sta2apDevs.Get(0), MakeCallback(&If1Assoc));
	// RegisterMonitorSnifferRxCallback(sta2apDevs.Get(0), MakeCallback(&If1MonitorSnifferRx));

	wifiMac.SetType("ns3::StaWifiMac",
									"Ssid", SsidValue (ssidH),
									"MaxMissedBeacons", UintegerValue (8),
									"ScanType", EnumValue(StaWifiMac::ACTIVE),
									"ActiveProbing", BooleanValue(true));
	sta2apDevs.Add(wifi.Install(wifiPhy, wifiMac, staNodes.Get(0)));

	RegisterAssocCallback(sta2apDevs.Get(1), MakeCallback(&If2Assoc));
	// RegisterMonitorSnifferRxCallback(sta2apDevs.Get(1), MakeCallback(&If2MonitorSnifferRx));

	wifiPhy.EnablePcapAll("netroad-mptcp-wifi", false);

	NS_LOG_INFO ("assign ip");

	SetIpv4Address(srv2swDevs.Get(0), "10.1.1.1", "/24");
	SetIpv4Address(sw2srvDevs.Get(0), "10.1.1.2", "/24");

	for(uint32_t i = 0; i < nApsV + nApsH; i++)	{
		SetIpv4Address(sw2apDevs.Get(i), BuildIpv4Address(10, 1, i+2, 1), Ipv4Mask("/24"));
		SetIpv4Address(ap2swDevs.Get(i), BuildIpv4Address(10, 1, i+2, 2), Ipv4Mask("/24"));

		SetIpv4Address(ap2staDevs.Get(i), BuildIpv4Address(192, 168, i+1, 1), Ipv4Mask("/24"));

		Ipv4Address gw = BuildIpv4Address(192, 168, i+1, 1);
		Ipv4Address ip = BuildIpv4Address(192, 168, i+1, 2);
		Ipv4Address net = ip.CombineMask(Ipv4Mask("/24"));

		Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(ap2staDevs.Get(i));
		aps.push_back(APInfo(wifiDev->GetMac()->GetAddress(), gw, ip, net));
	}

	NS_LOG_INFO ("routing");

	RouteAddDefaultWithGatewayIfIndex(srvNodes.Get(0), GetIpv4Address (sw2srvDevs.Get(0)), 0);
	RouteAddWithNetworkGatewayIfIndex(swNodes.Get(0),Ipv4Address("10.1.1.0"), Ipv4Address("10.1.1.2"), 0);

	for(uint32_t i = 0; i < nApsV + nApsH; i ++){
		RouteAddWithNetworkGatewayIfIndex(swNodes.Get(0), BuildIpv4Address(192, 168, i+1, 0), GetIpv4Address (ap2swDevs.Get(i)), i+1);
		RouteAddWithNetworkGatewayIfIndex(apNodes.Get(i), Ipv4Address("10.1.1.0"), GetIpv4Address (sw2apDevs.Get(i)), 0);

		LinuxStackHelper::RunIp (apNodes.Get(i), Seconds(10), "route show");
	}

	DceApplicationHelper dce;
	dce.SetStackSize (1 << 30);

	// dce.SetBinary ("iperf");
	// dce.ResetArguments ();
	// dce.ResetEnvironment ();
	// dce.AddArgument ("-s");
	// dce.AddArgument ("-P");
	// dce.AddArgument ("1");

	dce.SetBinary ("thttpd");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  //  dce.AddArgument ("-D");
  dce.SetUid (1);
  dce.SetEuid (1);

	apps = dce.Install (srvNodes.Get (0));
	apps.Start (Seconds (1.0));

	// dce.SetBinary ("iperf");
	// dce.ResetArguments ();
	// dce.ResetEnvironment ();
	// dce.AddArgument ("-c");
	// dce.AddArgument ("10.1.1.1");
	// dce.AddArgument ("-i");
	// dce.AddArgument ("0.5");
	// dce.AddArgument ("--time");
	// dce.AddArgument ("100");

	dce.SetBinary ("wget");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  // dce.AddArgument ("-r");
  dce.AddArgument ("http://10.1.1.1:80/mytest.mp4");

	apps = dce.Install (staNodes.Get (0));
	apps.Start (Seconds (5.0));
	apps.Stop (Seconds (200));

	NS_LOG_INFO ("animation");

	AnimationInterface anim("netroad-mptcp.xml");

	Simulator::Stop(Seconds(30));
	Simulator::Run();
	Simulator::Destroy();
	return 0;
}

static void
If1Assoc (Mac48Address address)
{
	if(address == ap1.m_mac)
		return;

	for(int i = 0; i < aps.size(); i ++)
	{
		if(aps[i].m_mac != address)
			continue;

		AddrAddAndLinkUpWithIpIface(staNodes.Get(0), aps[i].m_ip, "sim0");
		UpdateRuleRouteWithTableIfaceIpNetworkGateway(staNodes.Get(0), "1", "sim0", aps[i].m_ip, aps[i].m_net, aps[i].m_gw);
		RouteAddGlobalWithGatewayIface(staNodes.Get(0), aps[i].m_gw, "sim0");
		ShowRuleRoute (staNodes.Get(0));

		ap1 = aps[i];
		break;
	}
}

static void
If2Assoc (Mac48Address address)
{
	if(address == ap2.m_mac)
		return;

	for(int i = 0; i < aps.size(); i ++)
	{
		if(aps[i].m_mac != address)
			continue;

		AddrAddAndLinkUpWithIpIface(staNodes.Get(0), aps[i].m_ip, "sim1");
		UpdateRuleRouteWithTableIfaceIpNetworkGateway(staNodes.Get(0), "2", "sim1", aps[i].m_ip, aps[i].m_net, aps[i].m_gw);
		RouteAddGlobalWithGatewayIface(staNodes.Get(0), aps[i].m_gw, "sim1");
		ShowRuleRoute(staNodes.Get(0));

		ap2 = aps[i];
		break;
	}
}

static void
If1MonitorSnifferRx	(Ptr<const Packet> packet,
	uint16_t channelFreqMhz, uint16_t channelNumber, uint32_t rate,
	bool isShortPreamble, double signalDbm, double noiseDbm)
	{
		Ptr<Packet> p = packet->Copy();

		WifiMacHeader macHeader;
		p->RemoveHeader(macHeader);

		if (macHeader.IsBeacon())
		{
			MgtBeaconHeader beaconHeader;
			p->RemoveHeader (beaconHeader);

			NS_LOG_INFO ("ssid:" << beaconHeader.GetSsid() << ",bssid:" << macHeader.GetAddr3()
						<< ",signalDbm:" << signalDbm << ",noiseDbm:" << noiseDbm);
		}
	}

static void
If2MonitorSnifferRx	(Ptr<const Packet> packet,
	uint16_t channelFreqMhz, uint16_t channelNumber, uint32_t rate,
	bool isShortPreamble, double signalDbm, double noiseDbm)
	{
		Ptr<Packet> p = packet->Copy();

		WifiMacHeader macHeader;
		p->RemoveHeader(macHeader);

		if (macHeader.IsBeacon())
		{
			MgtBeaconHeader beaconHeader;
			p->RemoveHeader (beaconHeader);

			NS_LOG_INFO ("ssid:" << beaconHeader.GetSsid() << ",bssid:" << macHeader.GetAddr3()
						<< ",signalDbm:" << signalDbm << ",noiseDbm:" << noiseDbm);
		}
	}
