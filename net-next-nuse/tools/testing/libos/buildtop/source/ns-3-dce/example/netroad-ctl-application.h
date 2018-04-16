#ifndef NETROAD_CTL_APPLICATION_H
#define NETROAD_CTL_APPLICATION_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"

namespace ns3 {

class Address;
class Socket;
class Packet;

class ApStats {
  Mac48Address m_mac;
  double m_x, m_y, m_vx, m_vy;
  uint8_t m_channelNumber;

  ApStats (Mac48Address mac, double x, double y, double vx, double vy, uint8_t channelNumber):
    m_mac(mac), m_x(x), m_y(y), m_vx(vx), m_vy(vy), m_channelNumber(channelNumber){}
};

class NetroadCtlApplication : public Application
{
public:
  static TypeId GetTypeId (void);
  NetroadCtlApplication ();

  virtual ~NetroadCtlApplication ();

  uint32_t GetTotalRx () const;

  Ptr<Socket> GetListeningSocket (void) const;

  std::list<Ptr<Socket> > GetAcceptedSockets (void) const;

protected:
  virtual void DoDispose (void);
private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);

  void HandleAccept (Ptr<Socket> socket, const Address& from);
  void HandlePeerClose (Ptr<Socket> socket);
  void HandlePeerError (Ptr<Socket> socket);

  Ptr<Socket>     m_socket;
  std::list<Ptr<Socket> > m_socketList;

  Address         m_local;
  uint32_t        m_totalRx;
  TypeId          m_tid;

  std::vector<ApStats> apStats;

  TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;
};

}

#endif
