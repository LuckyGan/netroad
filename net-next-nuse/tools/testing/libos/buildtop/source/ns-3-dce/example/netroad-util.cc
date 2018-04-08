#include "netroad-util.h"

namespace ns3 {
  NS_LOG_COMPONENT_DEFINE ("NETROAD_UTIL");

  Ipv4Address BuildIpv4Address (const uint32_t byte0, const uint32_t byte1, const uint32_t byte2, const uint32_t byte3) {
    std::ostringstream oss;
    oss << byte0 << "." << byte1 << "." << byte2 << "." << byte3;
    return Ipv4Address (oss.str().c_str());
  }

  void SetIpv4Address (const Ptr<NetDevice> device, const Ipv4Address address, const Ipv4Mask mask) {
    Ptr<Node> node = device->GetNode ();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    int32_t interface = ipv4->GetInterfaceForDevice (device);

    if(interface == -1) {
      interface = ipv4->AddInterface (device);
    }

    ipv4->AddAddress (interface, Ipv4InterfaceAddress (address, mask));
    ipv4->SetMetric (interface, 1);
    ipv4->SetForwarding (interface, true);
    ipv4->SetUp (interface);
  }

  Ipv4Address GetIpv4Address (const Ptr<NetDevice> device) {
    Ptr<Node> node = device->GetNode ();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    int32_t interface = ipv4->GetInterfaceForDevice (device);

    return ipv4->GetAddress (interface, 0).GetLocal ();
  }

  void SetPosition (const Ptr<Node> node, const Vector& position) {
  	Ptr<MobilityModel> m = node->GetObject<MobilityModel>();
  	m->SetPosition(position);
  }

  void SetPositionVelocity (const Ptr<Node> node, const Vector& position, const Vector& velocity) {
  	Ptr<ConstantVelocityMobilityModel> m = node->GetObject<ConstantVelocityMobilityModel>();
  	m->SetPosition(position);
  	m->SetVelocity(velocity);
  }

  void RegisterAssocCallback (const Ptr<NetDevice> device,const CallbackBase &cb) {
    std::ostringstream oss;
  	oss << "/NodeList/" << device->GetNode()->GetId()
        << "/DeviceList/" << device->GetIfIndex()
        << "/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/Assoc";
  	Config::ConnectWithoutContext(oss.str().c_str(), cb);
  }

  void RegisterMonitorSnifferRxCallback (const Ptr<NetDevice> device, const CallbackBase &cb) {
    std::ostringstream oss;
  	oss << "/NodeList/" << device->GetNode()->GetId()
        << "/DeviceList/" << device->GetIfIndex()
        << "/$ns3::WifiNetDevice/Phy/MonitorSnifferRx";
  	Config::ConnectWithoutContext(oss.str().c_str(), cb);
  }

  void RouteAddDefaultWithGatewayIfIndex (const Ptr<Node> node, const Ipv4Address gateway, const uint32_t ifIndex) {
    std::ostringstream oss;
    oss << "route add default via " << gateway << " dev sim" << ifIndex;
    LinuxStackHelper::RunIp (node, Seconds(0.01), oss.str());
  }

  void RouteAddWithNetworkGatewayIfIndex (const Ptr<Node> node, const Ipv4Address network, const Ipv4Address gateway, const uint32_t ifIndex) {
    std::ostringstream oss;
    oss << "route add " << network << "/24 via " << gateway << " dev sim" << ifIndex;
    LinuxStackHelper::RunIp (node, Seconds(0.01), oss.str());
  }

  double AddrAddLinkUpWithIpIfIndex (const Ptr<Node> node, const Ipv4Address ip, const uint32_t ifIndex, double timeOffset) {
  	std::ostringstream oss;
  	oss << "-f inet addr add " << ip << "/24 dev sim" << ifIndex;
  	LinuxStackHelper::RunIp (node, Seconds (timeOffset), oss.str());

    timeOffset += 0.01;
  	oss.str("");
  	oss << "link set sim" << ifIndex << " up arp on";
  	LinuxStackHelper::RunIp (node, Seconds (timeOffset), oss.str());

    return timeOffset;
  }

  double RuleRouteUpdateWithIfIndexAp (const Ptr<Node> node, const uint32_t ifIndex, const struct APInfo ap, double timeOffset) {
    uint32_t nodeId = node->GetId ();

  	std::ostringstream oss;
  	oss << "rule del lookup " << ifIndex +1;
  	LinuxStackHelper::RunIp (node, Seconds (timeOffset), oss.str());

    timeOffset += 0.01;
		oss.str("");
    oss << "route flush table " << ifIndex +1;
		LinuxStackHelper::RunIp (node, Seconds (timeOffset), oss.str());

    timeOffset += 0.01;
		oss.str ("");
		oss << "rule add from " << ap.m_ip << " table " << ifIndex + 1;
		LinuxStackHelper::RunIp (node, Seconds (timeOffset), oss.str ());

    timeOffset += 0.01;
		oss.str ("");
		oss << "route add " << ap.m_net << "/24 dev sim" << ifIndex << " scope link table " << ifIndex + 1;
		LinuxStackHelper::RunIp (node, Seconds (timeOffset), oss.str ());

    timeOffset += 0.01;
		oss.str ("");
		oss << "route add default via " << ap.m_gw << " dev sim" << ifIndex <<" table " << ifIndex + 1;
		LinuxStackHelper::RunIp (node, Seconds(timeOffset), oss.str ());

    return timeOffset;
	}

  double RouteAddGlobalWithGatewayIface (const Ptr<Node> node, const Ipv4Address gw, const uint32_t ifIndex, double timeOffset) {
  	LinuxStackHelper::RunIp (node, Seconds (timeOffset), "route del default");

    timeOffset += 0.01;
  	std::ostringstream oss;
  	oss << "route add default scope global nexthop via "<< gw <<" dev sim" << ifIndex;
  	LinuxStackHelper::RunIp (node, Seconds (timeOffset), oss.str());

    return timeOffset;
  }

  double ShowRuleRoute(const Ptr<Node> node, double timeOffset) {

  	LinuxStackHelper::RunIp (node, Seconds (timeOffset), "rule show");
    timeOffset += 0.01;
  	LinuxStackHelper::RunIp (node, Seconds (timeOffset), "route show");
    timeOffset += 0.01;
    LinuxStackHelper::RunIp (node, Seconds (timeOffset), "route show table 0");
    timeOffset += 0.01;
    LinuxStackHelper::RunIp (node, Seconds (timeOffset), "route show table 1");
    timeOffset += 0.01;
    LinuxStackHelper::RunIp (node, Seconds (timeOffset), "route show table 2");
    timeOffset += 0.01;
    LinuxStackHelper::RunIp (node, Seconds (timeOffset), "route show table local");
    timeOffset += 0.01;
    LinuxStackHelper::RunIp (node, Seconds (timeOffset), "route show table main");
    timeOffset += 0.01;
    LinuxStackHelper::RunIp (node, Seconds (timeOffset), "route show table default");

    return timeOffset;
  }

  double UpdateNewAp (const Ptr<Node> node, const uint32_t ifIndex, const struct APInfo oldAP, const struct APInfo ap) {

    double timeOffset = 0.0;
    // if(oldAP.m_mac != Mac48Address ()) {
    //   timeOffset = RemoveOldRuleRoute (node, ifIndex, oldAP, timeOffset);
    // }

    timeOffset = AddrAddLinkUpWithIpIfIndex (node, ap.m_ip, ifIndex, timeOffset + 0.01);
    timeOffset = RuleRouteUpdateWithIfIndexAp (node, ifIndex, ap, timeOffset + 0.01);
    timeOffset = RouteAddGlobalWithGatewayIface (node, ap.m_gw, ifIndex, timeOffset + 0.01);
    timeOffset = ShowRuleRoute(node, timeOffset + 0.01);

    return timeOffset;
  }

  double RemoveOldRuleRoute (const Ptr<Node> node, const uint32_t ifIndex, const struct APInfo ap, double timeOffset) {
    uint32_t nodeId = node->GetId ();

  	std::ostringstream oss;
    oss << "route del " << ap.m_net << "/24 dev sim" << ifIndex << " proto kernel scope link src " << ap.m_ip;
    LinuxStackHelper::RunIp (node, Seconds(timeOffset), oss.str ());

    timeOffset += 0.01;
    oss.str ("");
    oss << "route del broadcast " << ap.m_net << " dev sim" << ifIndex << " table local proto kernel scope link src " << ap.m_ip;
    LinuxStackHelper::RunIp (node, Seconds(timeOffset), oss.str ());

    timeOffset += 0.01;
    oss.str ("");
    oss << "route del local " << ap.m_ip << " dev sim" << ifIndex << " table local proto kernel scope host src " << ap.m_ip;
    LinuxStackHelper::RunIp (node, Seconds(timeOffset), oss.str ());

    timeOffset += 0.01;
    oss.str ("");
    oss << "route del broadcast " << ap.m_broadcast << " dev sim" << ifIndex << " table local proto kernel scope link src " << ap.m_ip;
    LinuxStackHelper::RunIp (node, Seconds(timeOffset), oss.str ());

    timeOffset += 0.01;
    LinuxStackHelper::RunIp (node, Seconds(timeOffset), "route del local fe80::200:ff:fe00:f dev lo  table local  proto none  metric 0  pref medium");

    timeOffset += 0.01;
    LinuxStackHelper::RunIp (node, Seconds(timeOffset), "route del local fe80::200:ff:fe00:10 dev lo  table local  proto none  metric 0  pref medium");

    return timeOffset;
  }

  void DoIperf (const Ptr<Node> node) {
    DceApplicationHelper dce;
    dce.SetStackSize (1 << 30);

    dce.SetBinary ("iperf");
    dce.ResetArguments ();
    dce.ResetEnvironment ();
    dce.AddArgument ("-c");
    dce.AddArgument ("10.1.1.1");
    dce.AddArgument ("-i");
    dce.AddArgument ("1");
    dce.AddArgument ("--time");
    dce.AddArgument ("20");

    // ApplicationContainer apps = dce.Install (node);
    // NS_LOG_INFO (Simulator::Now());
    // apps.Start (Simulator::Now());
  }
}
