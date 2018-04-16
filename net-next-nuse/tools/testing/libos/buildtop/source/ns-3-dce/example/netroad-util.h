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
  	Ipv4Address m_gw, m_ip, m_net, m_broadcast;

  	APInfo (Mac48Address mac, Ipv4Address gw, Ipv4Address ip, Ipv4Address net, Ipv4Address broadcast):
      m_mac(mac), m_gw(gw), m_ip(ip), m_net(net), m_broadcast(broadcast) {}
  };

  class ByteBuffer {
  public:
    ByteBuffer (int size){
      buffer = std::vector<uint8_t> ();
      buffer.reserve(size);
      clear();
    }

    ByteBuffer (uint8_t* arr, uint32_t size) {
      buffer = std::vector<uint8_t> ();

      if (arr == NULL) {
        buffer.reserve(size);
        clear();
      } else {
        buffer.reserve(size);
        clear();
        for (int i=0;i<size;i++){
          put<uint8_t> (arr[i]);
        }
      }
    }

    template<typename T> void put (T data){
      uint32_t dataSize = sizeof(data);

      if(buffer.size() < pos + dataSize) {
        buffer.resize(pos + dataSize);
      }

      memcpy(&buffer[pos], (uint8_t*) &data, dataSize);
      pos += dataSize;
    }

    template<typename T> T read() const {
      if (pos + sizeof(T) <= buffer.size()) {
        return *((T*) &buffer[pos]);
      }
      return 0;
    }

    std::vector<uint8_t> getVector() {
      return buffer;
    }

    ~ByteBuffer() { }


  private:
    std::vector<uint8_t> buffer;
    uint32_t pos;

    void clear() {
      pos = 0;
      buffer.clear();
    }
  };

  Ipv4Address BuildIpv4Address (const uint32_t byte0, const uint32_t byte1, const uint32_t byte2, const uint32_t byte3);
  void SetIpv4Address (const Ptr<NetDevice> device, const Ipv4Address address, const Ipv4Mask mask);
  Ipv4Address GetIpv4Address (const Ptr<NetDevice> device);

  void SetPosition (const Ptr<Node> node, const Vector& position);
  void SetPositionVelocity (const Ptr<Node> node, const Vector& position, const Vector& velocity);

  void RegisterAssocCallback(Ptr<NetDevice> device,const CallbackBase &cb);
  void RegisterMonitorSnifferRxCallback (const Ptr<NetDevice> device, const CallbackBase &cb);

  void RouteAddDefaultWithGatewayIfIndex (const Ptr<Node> node, const Ipv4Address gateway, const uint32_t ifIndex);
  void RouteAddWithNetworkGatewayIfIndex (const Ptr<Node> node, const Ipv4Address network, const Ipv4Address gateway, const uint32_t ifIndex);

  double AddrAddLinkUpWithIpIfIndex (const Ptr<Node> node, const Ipv4Address ip, const uint32_t ifIndex, double timeOffset);
  double RuleRouteUpdateWithIfIndexAp (const Ptr<Node> node, const uint32_t ifIndex, const struct APInfo ap, double timeOffset);
  double RouteAddGlobalWithGatewayIface (const Ptr<Node> node, const Ipv4Address gw, const uint32_t ifIndex, double timeOffset);
  double ShowRuleRoute(const Ptr<Node> node, double timeOffset);
  double RemoveIpv4Address(const Ptr<Node> node, const uint32_t ifIndex, const Ipv4Address ip, double timeOffset);
  double UpdateNewAp (const Ptr<Node> node, const uint32_t ifIndex, const struct APInfo oldAP, const struct APInfo ap);
  double RemoveOldRuleRoute (const Ptr<Node> node, const uint32_t ifIndex, const struct APInfo ap, double timeOffset);
  void DoIperf (const Ptr<Node> node);
}

#endif
