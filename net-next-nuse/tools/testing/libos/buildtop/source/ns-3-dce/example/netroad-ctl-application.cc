#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "netroad-ctl-application.h"
#include "stdio.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NetroadCtlApplication");

NS_OBJECT_ENSURE_REGISTERED (NetroadCtlApplication);

TypeId
NetroadCtlApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetroadCtlApplication")
    .SetParent<Application> ()
    .AddConstructor<NetroadCtlApplication> ()
    .AddAttribute ("Local",
                   "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&NetroadCtlApplication::m_local),
                   MakeAddressChecker ())
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&NetroadCtlApplication::m_rxTrace),
                     "ns3::Packet::PacketAddressTracedCallback")
  ;
  return tid;
}

NetroadCtlApplication::NetroadCtlApplication ()
{
  NS_LOG_FUNCTION (this);
  m_tid = TypeId::LookupByName ("ns3::LinuxTcpSocketFactory");

  m_socket = 0;
  m_totalRx = 0;
}

NetroadCtlApplication::~NetroadCtlApplication()
{
  NS_LOG_FUNCTION (this);
}

uint32_t NetroadCtlApplication::GetTotalRx () const
{
  NS_LOG_FUNCTION (this);
  return m_totalRx;
}

Ptr<Socket>
NetroadCtlApplication::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

std::list<Ptr<Socket> >
NetroadCtlApplication::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void NetroadCtlApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socketList.clear ();

  // chain up
  Application::DoDispose ();
}


// Application Methods
void NetroadCtlApplication::StartApplication ()    // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      m_socket->Bind (m_local);
      m_socket->Listen ();
      m_socket->ShutdownSend ();
    }

  m_socket->SetRecvCallback (MakeCallback (&NetroadCtlApplication::HandleRead, this));
  m_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&NetroadCtlApplication::HandleAccept, this));
  m_socket->SetCloseCallbacks (
    MakeCallback (&NetroadCtlApplication::HandlePeerClose, this),
    MakeCallback (&NetroadCtlApplication::HandlePeerError, this));
}

void NetroadCtlApplication::StopApplication ()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
      Ptr<Socket> acceptedSocket = m_socketList.front ();
      m_socketList.pop_front ();
      acceptedSocket->Close ();
    }
  if (m_socket)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void NetroadCtlApplication::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;

  // int doubleSize = sizeof(double);
  // int size = 6 + 4 * doubleSize + 1;
  // NS_LOG_INFO ("packet size " << size << "bytes");
  // uint8_t buffer[size];
  // int cnt = socket->Recv (buffer, size, 0);
  // NS_LOG_INFO ("recieved " << cnt << "bytes");
  // Mac48Address mac;
  // mac.CopyFrom (buffer);
  // NS_LOG_INFO ("mac:" << mac);
  // NS_LOG_INFO ("recv:");
  // for(int j = 0; j < size; j++)
  //   printf("%02X", buffer[j]);
  //   printf("\n");

  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }


        int doubleSize = sizeof(double);
        int size = 6 + 4 * doubleSize + 1;
        NS_LOG_INFO ("packet size " << size << "bytes");
        uint8_t buffer[size];
        packet->CopyData (buffer, size);

        NS_LOG_INFO ("recv:");
        for(int j = 0; j < size; j++)
          printf("%02X", buffer[j]);
        printf("\n");



      m_totalRx += packet->GetSize ();
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      m_rxTrace (packet, from);
    }
}


void NetroadCtlApplication::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void NetroadCtlApplication::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}


void NetroadCtlApplication::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&NetroadCtlApplication::HandleRead, this));
  m_socketList.push_back (s);
}

}
