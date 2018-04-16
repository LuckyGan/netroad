
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "netroad-ap-application.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NetroadApApplication");

NS_OBJECT_ENSURE_REGISTERED (NetroadApApplication);

TypeId
NetroadApApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetroadApApplication")
    .SetParent<Application> ()
    .AddConstructor<NetroadApApplication> ()
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&NetroadApApplication::m_peer),
                   MakeAddressChecker ())
  ;
  return tid;
}


NetroadApApplication::NetroadApApplication ()
  : m_socket (0),
    m_connected (false)
{
  NS_LOG_FUNCTION (this);
  m_tid = TypeId::LookupByName ("ns3::LinuxTcpSocketFactory");
}

NetroadApApplication::~NetroadApApplication ()
{
  NS_LOG_FUNCTION (this);
}

Ptr<Socket> NetroadApApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

void
NetroadApApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;

  Application::DoDispose ();
}

void NetroadApApplication::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);

      m_socket->Bind ();
      m_socket->Connect (m_peer);
      m_socket->ShutdownRecv ();
      m_socket->SetConnectCallback (
        MakeCallback (&NetroadApApplication::ConnectionSucceeded, this),
        MakeCallback (&NetroadApApplication::ConnectionFailed, this));
      m_socket->SetSendCallback (
        MakeCallback (&NetroadApApplication::DataSend, this));
    }
  if (m_connected)
    {
      SendData ();
    }
}

void NetroadApApplication::StopApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_connected = false;
    }
  else
    {
      NS_LOG_WARN ("NetroadApApplication found null socket to close in StopApplication");
    }
}

void NetroadApApplication::SendData (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice> (GetNode () -> GetDevice (1));
  Ptr<MobilityModel > m = GetNode ()-> GetObject<MobilityModel >();

  std::ostringstream oss;
  oss << "mac:" << wifiDev->GetMac ()->GetAddress() << ", position:" << m->GetPosition () << ", velocity:" << m->GetVelocity ();
  NS_LOG_INFO (oss.str());

  NS_LOG_LOGIC ("sending packet at " << Simulator::Now ());


  int size = sizeof(Mac48Address) + 4 * sizeof(double) + sizeof (uint8_t);
  uint8_t buffer[size];
  int offset = 0;
  wifiDev->GetMac ()->GetAddress().CopyTo (buffer);
  offset += 6;
  double pX = m->GetPosition().x;
  memcpy (buffer+offset, &pX, sizeof(double));
  offset += sizeof(double);
  double pY = m->GetPosition().y;
  memcpy (buffer+offset, &pY, sizeof(double));
  offset += sizeof(double);
  double vX = m->GetVelocity().x;
  memcpy (buffer+offset, &vX, sizeof(double));
  offset += sizeof (double);
  double vY = m->GetVelocity().y;
  memcpy (buffer+offset, &vY, sizeof(double));
  offset += sizeof (double);
  uint8_t channelNumber = wifiDev->GetPhy ()->GetChannelNumber ();
  memcpy (buffer+offset, &channelNumber, sizeof (uint8_t));

  NS_LOG_INFO ("send:");
  for(int j = 0; j < size; j++)
    printf("%02X", buffer[j]);
  printf("\n");

  int actual = m_socket->Send (buffer, size, 0);

  NS_LOG_INFO ("send " << actual << " bytes, expected " << size << " bytes");
}

void NetroadApApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("NetroadApApplication Connection succeeded");
  m_connected = true;
  SendData ();
}

void NetroadApApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("NetroadApApplication, Connection Failed");
}

void NetroadApApplication::DataSend (Ptr<Socket>, uint32_t)
{
  NS_LOG_FUNCTION (this);

  if (m_connected)
    {
      Simulator::Schedule (Seconds(0.5), &NetroadApApplication::SendData, this);
    }
}

}
