#ifndef NETROAD_UTIL_H
#define NETROAD_UTIL_H

#include "ns3/core-module.h"
#include "ns3/dce-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"

namespace ns3 {

  struct APInfo {
  	Mac48Address m_mac;
  	Ipv4Address m_gw, m_ip, m_net;

  	APInfo (Mac48Address mac, Ipv4Address gw, Ipv4Address ip, Ipv4Address net):
      m_mac(mac), m_gw(gw), m_ip(ip), m_net(net) {}
  };

  Ipv4Address BuildIpv4Address (const uint32_t byte0, const uint32_t byte1, const uint32_t byte2, const uint32_t byte3);
  void SetIpv4Address (const Ptr<NetDevice> device, const Ipv4Address address, const Ipv4Mask mask);
  Ipv4Address GetIpv4Address (const Ptr<NetDevice> device);

  void SetPosition (const Ptr<Node> node, const Vector& position);
  void SetPositionVelocity (const Ptr<Node> node, const Vector& position, const Vector& velocity);

  void RegisterAssocCallback(Ptr<NetDevice> device,const CallbackBase &cb);
  void RegisterMonitorSnifferRxCallback (const Ptr<NetDevice> device, const CallbackBase &cb);

  void RouteAddDefaultWithGatewayIfIndex (const Ptr<Node> node, const Ipv4Address gateway, const uint32_t ifIndex);
  void RouteAddWithNetworkGatewayIfIndex(const Ptr<Node> node, const Ipv4Address network, const Ipv4Address gateway, const uint32_t ifIndex);
  void AddrAddAndLinkUpWithIpIface (const Ptr<Node> node, const Ipv4Address ip, const std::string iface);
  void UpdateRuleRouteWithTableIfaceIpNetworkGateway(const Ptr<Node> node, const std::string table, const std::string iface,
    const Ipv4Address ip, const Ipv4Address net, const Ipv4Address gw);
  void RouteAddGlobalWithGatewayIface(const Ptr<Node> node, const Ipv4Address gw, const std::string iface);
  void ShowRuleRoute(const Ptr<Node> node);

  void NewAssociation (const Ptr<StaWifiMac> staWifiMac, const Mac48Address address);
  void DoIperf (const Ptr<Node> node);
}

#endif
