
#ifndef NETROAD_AP_APPLICATION_H
#define NETROAD_AP_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

namespace ns3 {

class Address;
class Socket;

class NetroadApApplication : public Application
{
public:
  static TypeId GetTypeId (void);

  NetroadApApplication ();

  virtual ~NetroadApApplication ();

  void SetMaxBytes (uint32_t maxBytes);

  Ptr<Socket> GetSocket (void) const;

protected:
  virtual void DoDispose (void);
private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void SendData ();

  Ptr<Socket>     m_socket;
  Address         m_peer;
  bool            m_connected;
  TypeId          m_tid;

  TracedCallback<Ptr<const Packet> > m_txTrace;

private:

  void ConnectionSucceeded (Ptr<Socket> socket);
  void ConnectionFailed (Ptr<Socket> socket);
  void DataSend (Ptr<Socket>, uint32_t);
};

}

#endif
