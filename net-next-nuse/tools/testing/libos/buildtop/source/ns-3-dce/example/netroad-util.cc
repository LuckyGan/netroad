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
    NS_LOG_INFO ("node " << node->GetId() << ": " << oss.str());
    LinuxStackHelper::RunIp (node, Seconds(0.1), oss.str());
  }
  void RouteAddWithNetworkGatewayIfIndex (const Ptr<Node> node, const Ipv4Address network, const Ipv4Address gateway, const uint32_t ifIndex) {
    std::ostringstream oss;
    oss << "route add " << network << "/24 via " << gateway << " dev sim" << ifIndex;
    NS_LOG_INFO ("node " << node->GetId() << ": " << oss.str());
    LinuxStackHelper::RunIp (node, Seconds(0.1), oss.str());
  }

  void AddrAddAndLinkUpWithIpIface (const Ptr<Node> node, const Ipv4Address ip, const std::string iface) {
  	std::ostringstream oss;
  	oss << "-f inet addr add " << ip << "/24 dev " << iface;
  	LinuxStackHelper::RunIp (node, Seconds(0), oss.str());

  	oss.str("");
  	oss << "link set " << iface << " up arp on";
  	LinuxStackHelper::RunIp (node, Seconds(0), oss.str());
  }

  void UpdateRuleRouteWithTableIfaceIpNetworkGateway(const Ptr<Node> node, const std::string table, const std::string iface,
    const Ipv4Address ip, const Ipv4Address net, const Ipv4Address gw) {

  	std::ostringstream oss;
  	oss << "rule del lookup " << table;
  	LinuxStackHelper::RunIp (node, Seconds(0), oss.str());

		oss.str("");
		oss << "route flush table " << table;
		LinuxStackHelper::RunIp (node, Seconds(0), oss.str());

		oss.str ("");
		oss << "rule add from " << ip << " table " << table;
		LinuxStackHelper::RunIp (node, Seconds(0), oss.str ());

		oss.str ("");
		oss << "route add " << net << "/24 dev " << iface << " scope link table " << table;
		LinuxStackHelper::RunIp (node, Seconds(0), oss.str ());

		oss.str ("");
		oss << "route add default via " << gw << " dev " << iface <<" table " << table;
		LinuxStackHelper::RunIp (node, Seconds(0), oss.str ());
	}

  void RouteAddGlobalWithGatewayIface(const Ptr<Node> node, const Ipv4Address gw, const std::string iface) {
  	LinuxStackHelper::RunIp (node, Seconds(0), "route del default");

  	std::ostringstream oss;
  	oss << "route add default scope global nexthop via "<< gw <<" dev " << iface;
    NS_LOG_INFO ("node " << node->GetId() << ": " << oss.str());
  	LinuxStackHelper::RunIp (node, Seconds(0), oss.str());
  }

  void ShowRuleRoute(const Ptr<Node> node) {
  	LinuxStackHelper::RunIp (node, Seconds(0.1), "rule show");
  	LinuxStackHelper::RunIp (node, Seconds(0.1), "route show");
  	LinuxStackHelper::RunIp (node, Seconds(0.1), "route show table 1");
  	LinuxStackHelper::RunIp (node, Seconds(0.1), "route show table 2");
  }

  void NewAssociation (const Ptr<StaWifiMac> staWifiMac, const Mac48Address address) {
    NS_LOG_INFO (Simulator::Now());
    NS_LOG_INFO (staWifiMac->GetAddress ());
    NS_LOG_INFO (address);
    staWifiMac->SetNewAssociation (address);
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
