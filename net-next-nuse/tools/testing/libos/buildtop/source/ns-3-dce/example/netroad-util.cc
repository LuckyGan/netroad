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

  void RegisterCourseChangeCallback (const Ptr<Node> node, const CallbackBase &cb) {
    std::ostringstream oss;
    oss << "/NodeList/" << node->GetId()
        << "/$ns3::MobilityModel/CourseChange";
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

    LinuxStackHelper::RunIp (node, Seconds (timeOffset), "addr show dev sim0");
    timeOffset += 0.01;

    LinuxStackHelper::RunIp (node, Seconds (timeOffset), "addr show dev sim1");
    timeOffset += 0.01;

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
    if(oldAP.device != NULL) {
      timeOffset = RemoveIpv4Address (node, ifIndex, oldAP.m_ip, timeOffset + 0.01);
    }

    timeOffset = AddrAddLinkUpWithIpIfIndex (node, ap.m_ip, ifIndex, timeOffset + 0.01);
    timeOffset = RuleRouteUpdateWithIfIndexAp (node, ifIndex, ap, timeOffset + 0.01);
    timeOffset = RouteAddGlobalWithGatewayIface (node, ap.m_gw, ifIndex, timeOffset + 0.01);
    timeOffset = ShowRuleRoute(node, timeOffset + 0.01);

    return timeOffset;
  }

  double RemoveIpv4Address(const Ptr<Node> node, const uint32_t ifIndex, const Ipv4Address ip, double timeOffset) {
    std::ostringstream oss;
    oss << "addr del " << ip << "/24 dev sim" << ifIndex;
    LinuxStackHelper::RunIp (node, Seconds(timeOffset), oss.str ());

    return timeOffset;
  }

  double GetThroughput(double d) {
    double rssi = -37.2894;
    if(d > 1) {
      rssi = -37.2894 - 30 * log10 (d);
    }

    return m_B * log2 (1 + pow (10, (100+rssi)/10));
  }

  double GetDistance (double x1, double y1, double x2, double y2) {
    return sqrt ((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
  }

  APStats CalculateApStats(Ptr<NetDevice> device, Ptr<Node> sta) {

    Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice> (device);

    Ptr<Node> ap = wifiDev->GetNode();
    Ptr <ConstantVelocityMobilityModel> apModel = ap->GetObject<ConstantVelocityMobilityModel>();
    Vector pAp = apModel->GetPosition ();
    Vector vAp = apModel->GetVelocity ();

    Ptr <ConstantPositionMobilityModel> staModel = sta->GetObject<ConstantPositionMobilityModel>();
    Vector pSta = staModel->GetPosition ();

    double x1 = pAp.x, y1 = pAp.y, x2 = pSta.x, y2 = pSta.y;
    double distance = GetDistance (x1, y1, x2, y2);

    if(distance > m_R) {
      APStats stats = APStats(wifiDev->GetMac ()->GetAddress(), 0, 0);
      return stats;
    }

    APStats stats = APStats(wifiDev->GetMac ()->GetAddress(), 0, 0);
    do {
      stats.m_throughput += 0.05 * GetThroughput (distance);
      stats.m_time += 0.05;
      x1 += vAp.x * 0.05;
      y1 += vAp.y * 0.05;
      distance = GetDistance (x1, y1, x2, y2);
    } while(distance < m_R);

    return stats;
  }
}
