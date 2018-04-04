#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "netroad-util.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NETROAD_HANDOFF");

std::vector<struct APInfo> aps;
struct APInfo ap1 = APInfo(Mac48Address(), Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"));
struct APInfo ap2 = APInfo(Mac48Address(), Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"));
ApplicationContainer apps;
NodeContainer staNodes;
NetDeviceContainer sta2apDevs;

static void If1Assoc (Mac48Address address);
static void If2Assoc (Mac48Address address);
static void If1MonitorSnifferRx	(Ptr<const Packet> packet,
	uint16_t channelFreqMhz, uint16_t channelNumber, uint32_t rate,
	bool isShortPreamble, double signalDbm, double noiseDbm);
static void If2MonitorSnifferRx	(Ptr<const Packet> packet,
	uint16_t channelFreqMhz, uint16_t channelNumber, uint32_t rate,
	bool isShortPreamble, double signalDbm, double noiseDbm);

int main(int argc, char* argv[]){
  uint32_t nAPs = 4;

  CommandLine cmd;
	cmd.Parse(argc, argv);

  LogComponentEnable("NETROAD_HANDOFF", LOG_LEVEL_ALL);
  LogComponentEnable("NETROAD_UTIL", LOG_LEVEL_ALL);

  NS_LOG_INFO ("create nodes");

  NodeContainer srvNodes, swNodes, apNodes;
  srvNodes.Create(1);
  swNodes.Create(1);
  apNodes.Create(nAPs);
  staNodes.Create(1);

  NS_LOG_INFO ("mobility");

  MobilityHelper mobility;
  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
																"MinX", DoubleValue(0.0),
																"MinY", DoubleValue(0.0),
																"DeltaX", DoubleValue(5.0),
																"DeltaY", DoubleValue(5.0),
																"GridWidth", UintegerValue(5),
																"LayoutType", StringValue("RowFirst"));

  NS_LOG_INFO ("mobility1");
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

  mobility.Install(srvNodes);
  mobility.Install(swNodes);
  mobility.Install(apNodes);
  mobility.Install(staNodes);

  SetPosition(srvNodes.Get(0), Vector(10, 0, 0));
  SetPosition(swNodes.Get(0), Vector(10, 5, 0));

  SetPosition(apNodes.Get(0), Vector(5, 5, 0));
  SetPosition(apNodes.Get(1), Vector(5, 15, 0));
  SetPosition(apNodes.Get(2), Vector(15, 15, 0));
  SetPosition(apNodes.Get(3), Vector(15, 5, 0));

  SetPosition(staNodes.Get(0), Vector(10, 10, 0));

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

  for(uint32_t i = 0; i<nAPs; i++) {
    p2pDevs = p2p.Install(NodeContainer(apNodes.Get(i), swNodes.Get(0)));
		ap2swDevs.Add(p2pDevs.Get(0));
		sw2apDevs.Add(p2pDevs.Get(1));
  }

  p2p.EnablePcapAll ("netroad-handoff-p2p");

  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
	wifiPhy.SetChannel(wifiChannel.Create());

	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();

	WifiHelper wifi = WifiHelper::Default();
	wifi.SetStandard(WIFI_PHY_STANDARD_80211g);

  Ssid ssid = Ssid("NETROAD");
	wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid)
		// "BeaconGeneration", BooleanValue (false)
	);

	for(uint32_t i = 0; i < nAPs; i++) {
		wifiPhy.Set("ChannelNumber", UintegerValue(1 + (i % 3) * 5));
		ap2staDevs.Add(wifi.Install(wifiPhy, wifiMac, apNodes.Get(i)));

		Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice> (ap2staDevs.Get (i));
		NS_LOG_INFO ("ap: " << wifiDev->GetMac ()->GetAddress() << ", channel: " << wifiDev->GetPhy ()->GetChannelNumber ());
	}

	wifiMac.SetType("ns3::StaWifiMac",
									"Ssid", SsidValue (ssid),
									"ScanType", EnumValue(StaWifiMac::NOTSUPPORT),
									"ActiveProbing", BooleanValue(false));

	for(uint32_t i = 0; i < 2; i++) {
		wifiPhy.Set("ChannelNumber", UintegerValue(1 + (i % 3) * 5));
		sta2apDevs.Add(wifi.Install(wifiPhy, wifiMac, staNodes.Get(0)));

		Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice> (sta2apDevs.Get (i));
		NS_LOG_INFO ("sta: " << wifiDev->GetMac ()->GetAddress() << ", channel: " << wifiDev->GetPhy ()->GetChannelNumber ());
	}

	RegisterAssocCallback(sta2apDevs.Get(0), MakeCallback(&If1Assoc));
	// RegisterMonitorSnifferRxCallback(sta2apDevs.Get(0), MakeCallback(&If1MonitorSnifferRx));

	RegisterAssocCallback(sta2apDevs.Get(1), MakeCallback(&If2Assoc));
	// RegisterMonitorSnifferRxCallback(sta2apDevs.Get(1), MakeCallback(&If2MonitorSnifferRx));

	wifiPhy.EnablePcapAll("netroad-handoff-wifi", false);

  NS_LOG_INFO ("assign ip");

  SetIpv4Address(srv2swDevs.Get(0), "10.1.1.1", "/24");
  SetIpv4Address(sw2srvDevs.Get(0), "10.1.1.2", "/24");

  for(uint32_t i = 0; i < nAPs; i++)	{
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

	for(uint32_t i = 0; i < nAPs; i ++){
		RouteAddWithNetworkGatewayIfIndex(swNodes.Get(0), BuildIpv4Address(192, 168, i+1, 0), GetIpv4Address (ap2swDevs.Get(i)), i+1);
		RouteAddWithNetworkGatewayIfIndex(apNodes.Get(i), Ipv4Address("10.1.1.0"), GetIpv4Address (sw2apDevs.Get(i)), 0);

		LinuxStackHelper::RunIp (apNodes.Get(i), Seconds(10), "route show");
	}

	NS_LOG_INFO ("apps");

  DceApplicationHelper dce;
	dce.SetStackSize (1 << 30);

	dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-s");
  dce.AddArgument ("-P");
  dce.AddArgument ("1");

  apps = dce.Install (srvNodes.Get (0));
	apps.Start (Seconds (3.0));

	Ptr<WifiNetDevice> wifiNetDevice1 = DynamicCast<WifiNetDevice> (sta2apDevs.Get (0));
	Ptr<StaWifiMac> staWifiMac1 = DynamicCast<StaWifiMac> (wifiNetDevice1->GetMac ());
	Mac48Address address1 = Mac48Address ("00:00:00:00:00:0e");
	Simulator::ScheduleWithContext(staNodes.Get (0)->GetId (), Seconds(2), &StaWifiMac::SetNewAssociation, staWifiMac1, address1);

	Ptr<WifiNetDevice> wifiNetDevice2 = DynamicCast<WifiNetDevice> (sta2apDevs.Get (1));
	Ptr<StaWifiMac> staWifiMac2 = DynamicCast<StaWifiMac> (wifiNetDevice2->GetMac ());
	Mac48Address address2 = Mac48Address ("00:00:00:00:00:0c");
	Simulator::ScheduleWithContext(staNodes.Get (0)->GetId (), Seconds(1), &StaWifiMac::SetNewAssociation, staWifiMac2, address2);

	// Mac48Address address3 = Mac48Address ("00:00:00:00:00:0b");
	// Simulator::ScheduleWithContext(staNodes.Get (0)->GetId (), Seconds(50), &StaWifiMac::SetNewAssociation, staWifiMac1, address3);


	dce.SetBinary ("iperf");
	dce.ResetArguments ();
	dce.ResetEnvironment ();
	dce.AddArgument ("-c");
	dce.AddArgument ("10.1.1.1");
	dce.AddArgument ("-i");
	dce.AddArgument ("1");
	dce.AddArgument ("--time");
	dce.AddArgument ("200");

	apps = dce.Install (staNodes.Get (0));
	apps.Start (Seconds(6));

  NS_LOG_INFO ("animation");

	AnimationInterface anim("netroad-handoff.xml");

	Simulator::Stop(Seconds(300));
	Simulator::Run();
	Simulator::Destroy();
	return 0;
}

static void
If1Assoc (Mac48Address address)
{
  NS_LOG_INFO(Simulator::Now() << " if1: " << address);
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

	NS_LOG_INFO(Simulator::Now() << " if1: ok");
}

static void
If2Assoc (Mac48Address address)
{
  NS_LOG_INFO(Simulator::Now() << " if2: " << address);

	if(address == ap1.m_mac || address == ap2.m_mac)
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

	NS_LOG_INFO(Simulator::Now() << " if2: ok");
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
