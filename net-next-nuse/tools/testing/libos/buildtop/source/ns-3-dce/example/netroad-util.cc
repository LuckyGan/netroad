#include "netroad-util.h"

namespace ns3 {
  void RegisterAssocCallback(Ptr<NetDevice> device,const CallbackBase &cb) {
    std::ostringstream oss;
  	oss << "/NodeList/" << device->GetNode()->GetId()
        << "/DeviceList/" << device->GetIfIndex()
        << "/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/Assoc";
  	Config::ConnectWithoutContext(oss.str().c_str(), cb);
  }
}
