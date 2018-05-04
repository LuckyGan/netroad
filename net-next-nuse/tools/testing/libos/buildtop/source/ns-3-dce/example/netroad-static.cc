#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "netroad-util.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NETROAD_STATIC");

std::vector<struct APInfo> aps;
struct APInfo ap1 = APInfo(NULL, Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"));
struct APInfo ap2 = APInfo(NULL, Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"), Ipv4Address("0.0.0.0"));

Ptr<StaWifiMac> staWifiMac1 = NULL;
Ptr<StaWifiMac> staWifiMac2 = NULL;

NodeContainer staNodes;

static void If1Assoc (Mac48Address address);
static void If2Assoc (Mac48Address address);

int main(int argc, char* argv[]){
	Packet::EnablePrinting ();
  uint32_t nAPs = 3;

  CommandLine cmd;
	cmd.Parse(argc, argv);

  LogComponentEnable("NETROAD_STATIC", LOG_LEVEL_ALL);
  LogComponentEnable("NETROAD_UTIL", LOG_LEVEL_ALL);

  NS_LOG_INFO ("create nodes");

  NodeContainer srvNodes, swNodes, apNodes;
  srvNodes.Create(1);
  swNodes.Create(1);
  apNodes.Create(nAPs);
  staNodes.Create(1);

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
  mobility.Install(apNodes);
  mobility.Install(staNodes);

  SetPosition(srvNodes.Get(0), Vector(10, 0, 0));
  SetPosition(swNodes.Get(0), Vector(10, 5, 0));
  SetPosition(apNodes.Get(0), Vector(5, 5, 0));
  SetPosition(apNodes.Get(1), Vector(5, 15, 0));
  SetPosition(apNodes.Get(2), Vector(15, 15, 0));
  SetPosition(staNodes.Get(0), Vector(10, 10, 0));

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

  NetDeviceContainer srv2swDevs, sw2srvDevs, sw2apDevs, ap2swDevs,sta2apDevs, ap2staDevs;

  PointToPointHelper p2p;
	p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevs = p2p.Install(NodeContainer(srvNodes.Get(0), swNodes.Get(0)));
  srv2swDevs.Add(p2pDevs.Get(0));
	sw2srvDevs.Add(p2pDevs.Get(1));

  for(uint32_t i = 0; i<nAPs; i++) {
    p2pDevs = p2p.Install(NodeContainer(apNodes.Get(i), swNodes.Get(0)));
		ap2swDevs.Add(p2pDevs.Get(0));
		sw2apDevs.Add(p2pDevs.Get(1));
  }

  p2p.EnablePcapAll ("netroad-static-p2p");

  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
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

	for(uint32_t i = 0; i < nAPs; i++) {
		wifiPhy.Set("ChannelNumber", UintegerValue(1 + (i % 3) * 5));
		ap2staDevs.Add(wifi.Install(wifiPhy, wifiMac, apNodes.Get(i)));

		Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice> (ap2staDevs.Get (i));
		NS_LOG_INFO ("ap: " << wifiDev->GetMac ()->GetAddress() << ", channel: " << wifiDev->GetPhy ()->GetChannelNumber ());
		NS_LOG_INFO ("sta: " << wifiDev->GetMac ()->GetAddress() << ", frequency: " << wifiDev->GetPhy ()->GetFrequency ());
	}

	wifiMac.SetType(
		"ns3::StaWifiMac",
		"Ssid", SsidValue (ssid),
		"ScanType", EnumValue(StaWifiMac::NOTSUPPORT),
		"ActiveProbing", BooleanValue(false),
		"MaxMissedBeacons", UintegerValue (10086));

	for(uint32_t i = 0; i < 2; i++) {
		wifiPhy.Set("ChannelNumber", UintegerValue(1 + (i % 3) * 5));
		sta2apDevs.Add(wifi.Install(wifiPhy, wifiMac, staNodes.Get(0)));

		Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice> (sta2apDevs.Get (i));
		NS_LOG_INFO ("sta: " << wifiDev->GetMac ()->GetAddress() << ", channel: " << wifiDev->GetPhy ()->GetChannelNumber ());
	}

	RegisterAssocCallback(sta2apDevs.Get(0), MakeCallback(&If1Assoc));

	RegisterAssocCallback(sta2apDevs.Get(1), MakeCallback(&If2Assoc));

	wifiPhy.EnablePcapAll("netroad-static-wifi", false);

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
		Ipv4Address broadcast = BuildIpv4Address(192, 168, i+1, 255);

    Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(ap2staDevs.Get(i));
    aps.push_back(APInfo(wifiDev, gw, ip, net, broadcast));
  }

  NS_LOG_INFO ("routing");

	RouteAddDefaultWithGatewayIfIndex(srvNodes.Get(0), GetIpv4Address (sw2srvDevs.Get(0)), 0);
	RouteAddWithNetworkGatewayIfIndex(swNodes.Get(0), Ipv4Address("10.1.1.0"), Ipv4Address("10.1.1.2"), 0);

	for(uint32_t i = 0; i < nAPs; i ++){
		RouteAddWithNetworkGatewayIfIndex(swNodes.Get(0), BuildIpv4Address(192, 168, i+1, 0), GetIpv4Address (ap2swDevs.Get(i)), i+1);
		RouteAddWithNetworkGatewayIfIndex(apNodes.Get(i), Ipv4Address("10.1.1.0"), GetIpv4Address (sw2apDevs.Get(i)), 0);
	}

	NS_LOG_INFO ("handoff");

	Ptr<WifiNetDevice> wifiNetDevice1 = DynamicCast<WifiNetDevice> (sta2apDevs.Get (0));
	staWifiMac1 = DynamicCast<StaWifiMac> (wifiNetDevice1->GetMac ());

	Ptr<WifiNetDevice> wifiNetDevice2 = DynamicCast<WifiNetDevice> (sta2apDevs.Get (1));
	staWifiMac2 = DynamicCast<StaWifiMac> (wifiNetDevice2->GetMac ());

	Simulator::ScheduleWithContext(staNodes.Get (0)->GetId (), Seconds(0.0), &StaWifiMac::SetNewAssociation, staWifiMac1,
		aps[0].device->GetMac ()->GetAddress(), aps[0].device->GetPhy ()->GetChannelNumber ());

	Simulator::ScheduleWithContext(staNodes.Get (0)->GetId (), Seconds(0.0), &StaWifiMac::SetNewAssociation, staWifiMac2,
		aps[1].device->GetMac ()->GetAddress(), aps[1].device->GetPhy ()->GetChannelNumber ());

	Simulator::ScheduleWithContext(staNodes.Get (0)->GetId (), Seconds(10), &StaWifiMac::SetNewAssociation, staWifiMac1,
		aps[2].device->GetMac ()->GetAddress(), aps[2].device->GetPhy ()->GetChannelNumber ());

	Simulator::ScheduleWithContext(staNodes.Get (0)->GetId (), Seconds(20), &StaWifiMac::SetNewAssociation, staWifiMac2,
		aps[0].device->GetMac ()->GetAddress(), aps[0].device->GetPhy ()->GetChannelNumber ());

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
  sinkApp.Start(Seconds(1.0));

	BulkSendHelper bulk = BulkSendHelper("ns3::LinuxTcpSocketFactory", InetSocketAddress("10.1.1.1", 9999));
  bulk.SetAttribute("MaxBytes", UintegerValue(0));
  bulk.SetAttribute("SendSize", UintegerValue(1400));
  ApplicationContainer srcApp = bulk.Install(staNodes.Get(0));
  srcApp.Start(Seconds(2.0));

  NS_LOG_INFO ("animation");

	AnimationInterface anim("netroad-static.xml");

	Simulator::Stop(Seconds(30.0));
	Simulator::Run();
	Simulator::Destroy();
	return 0;
}

static void
If1Assoc (Mac48Address address) {
  NS_LOG_INFO(Simulator::Now() << " if1: " << address);
	if(ap1.device != NULL && address == ap1.device->GetMac ()->GetAddress()){
		return;
	}

	for(int i = 0; i < aps.size(); i ++) {
		if(aps[i].device->GetMac ()->GetAddress() != address){
			continue;
		}

		double timeOffset = UpdateNewAp (staNodes.Get(0), 0, ap1, aps[i]);
		ap1 = aps[i];
		break;
	}
	NS_LOG_INFO(Simulator::Now() << " if1: ok");
}

static void
If2Assoc (Mac48Address address)
{
  NS_LOG_INFO(Simulator::Now() << " if2: " << address);

	if(ap2.device != NULL && address == ap2.device->GetMac ()->GetAddress())
		return;

	for(int i = 0; i < aps.size(); i ++)
	{
		if(aps[i].device->GetMac ()->GetAddress() != address) {
			continue;
		}

		double timeOffset = UpdateNewAp (staNodes.Get(0), 1, ap2, aps[i]);
		ap2 = aps[i];
		break;
	}

	NS_LOG_INFO(Simulator::Now() << " if2: ok");
}
