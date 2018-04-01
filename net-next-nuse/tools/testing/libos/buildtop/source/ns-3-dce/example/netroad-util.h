#ifndef NETROAD_UTIL_H
#define NETROAD_UTIL_H

#include "ns3/network-module.h"
#include "ns3/core-module.h"

using namespace ns3;

namespace ns3 {
  void RegisterAssocCallback(Ptr<NetDevice> device,const CallbackBase &cb);
}

#endif // NETROAD_UTIL_H
