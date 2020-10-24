#include <pulcher-network/shared.hpp>

#include <stdio.h>

// copied from pulcher-util/enum.hpp

template <typename EnumType> constexpr auto Idx(EnumType const & v) {
  return static_cast<typename std::underlying_type<EnumType>::type>(v);
}

template <typename EnumType>
constexpr typename std::underlying_type<EnumType>::type & Idx(EnumType & v) {
  return reinterpret_cast<typename std::underlying_type<EnumType>::type &>(v);
}

pul::network::Network::Network() {}

pul::network::Network::~Network() {
  if (valid) { enet_deinitialize(); }
}

pul::network::Network::Network(pul::network::Network && rval) {
  this->valid = rval.valid;
  rval.valid = false;
}

pul::network::Network & pul::network::Network::operator=(
  pul::network::Network && rval
) {
  this->valid = rval.valid;
  rval.valid = false;
  return *this;
}


pul::network::Network pul::network::Network::Construct() {
  pul::network::Network network;
  network.valid = !enet_initialize();
  return network;
}

pul::network::Address pul::network::Address::Construct(
  char const * addressHost
, uint32_t port
) {
  pul::network::Address address;

  if (addressHost) {
    enet_address_set_host(&address.enetAddress, addressHost);
  } else {
    address.enetAddress.host = ENET_HOST_ANY;
  }
  address.enetAddress.port = port;

  return address;
}

pul::network::IncomingPacket::IncomingPacket() {}

pul::network::IncomingPacket::~IncomingPacket() {
  if (this->enetPacket)
    { enet_packet_destroy(this->enetPacket); }
  this->enetPacket = nullptr;
}

pul::network::IncomingPacket::IncomingPacket(
  pul::network::IncomingPacket && rval
) {
  this->enetPacket = rval.enetPacket;
  rval.enetPacket = nullptr;
}

pul::network::IncomingPacket & pul::network::IncomingPacket::operator=(
  pul::network::IncomingPacket && rval
) {
  this->enetPacket = rval.enetPacket;
  rval.enetPacket = nullptr;
  return *this;
}

pul::network::Host::Host() {}

pul::network::Host::~Host() {
  if (this->enetHost) {
    enet_host_destroy(this->enetHost);
  }
  this->enetHost = nullptr;
}

pul::network::Host::Host(pul::network::Host && rval) {
  this->enetHost = rval.enetHost;

  this->fnConnect    = rval.fnConnect;
  this->fnReceive    = rval.fnReceive;
  this->fnDisconnect = rval.fnDisconnect;

  rval.enetHost = nullptr;
}

pul::network::Host & pul::network::Host::operator=(
  pul::network::Host && rval
) {
  this->enetHost = rval.enetHost;

  this->fnConnect    = rval.fnConnect;
  this->fnReceive    = rval.fnReceive;
  this->fnDisconnect = rval.fnDisconnect;

  rval.enetHost = nullptr;

  return *this;
}

pul::network::Host
pul::network::Host::Construct(
  pul::network::Host::ConstructInfo const & ci
) {
  pul::network::Host host;

  host.enetHost =
    enet_host_create(
      (ci.isServer ? &ci.address.enetAddress : nullptr)
    , ci.maxConnections, ci.maxChannels
    , ci.incomingBandwidth, ci.outgoingBandwidth
    );

  if (!host.enetHost) {
    printf("network error - failed to create enet host\n");
    return host;
  }

  host.fnConnect    = ci.fnConnect;
  host.fnDisconnect = ci.fnDisconnect;
  host.fnReceive    = ci.fnReceive;

  return host;
}

void pul::network::Host::PollEvents(size_t timeout, size_t maxEventPoll) {
  for (size_t eventPoll = 0ul; eventPoll < maxEventPoll; ++ eventPoll) {
    auto event = this->ManualPollEvent(timeout);
    if (!event.Valid()) { break; }

    switch (event.eventType) {
        default: break;
        case ENET_EVENT_TYPE_CONNECT:    this->fnConnect(event);    break;
        case ENET_EVENT_TYPE_DISCONNECT: this->fnDisconnect(event); break;
        case ENET_EVENT_TYPE_RECEIVE:    this->fnReceive(event);    break;
    }
  }
}

pul::network::Event
pul::network::Host::ManualPollEvent(size_t timeout) {
  pul::network::Event event;

  ENetEvent enetEvent;
  event.pollResult =
    enet_host_service(this->enetHost, &enetEvent, timeout);
  event.peer = enetEvent.peer;
  event.eventType = enetEvent.type;

  event.packet.enetPacket = enetEvent.packet;

  // get type/data if receive packet
  if (event.eventType == ENET_EVENT_TYPE_RECEIVE) {
    event.type =
      *reinterpret_cast<pul::network::PacketType *>(
        event.packet.enetPacket->data
      );
    event.data = event.packet.enetPacket->data;
  }

  return event;
}

void pul::network::Host::Flush() {
  enet_host_flush(this->enetHost);
}

pul::network::ClientHostConnection::ClientHostConnection() {}
pul::network::ClientHostConnection::~ClientHostConnection() {
  if (enetPeer != nullptr) {
    enet_peer_reset(enetPeer);
  }
  enetPeer = nullptr;
}

pul::network::ClientHostConnection::ClientHostConnection(
  pul::network::ClientHostConnection && rval
) {
  this->enetPeer = rval.enetPeer;
  rval.enetPeer = nullptr;
}

pul::network::ClientHostConnection &
pul::network::ClientHostConnection::operator=(
  pul::network::ClientHostConnection && rval
) {
  this->enetPeer = rval.enetPeer;
  rval.enetPeer = nullptr;
  return *this;
}

pul::network::ClientHostConnection
pul::network::ClientHostConnection::Construct(
  pul::network::Host & host
, pul::network::Address const & address
) {
  pul::network::ClientHostConnection connection;
  connection.enetPeer =
    enet_host_connect(host.enetHost, &address.enetAddress, 2, 0);

  if (!connection.Valid())
    { printf("network error - could not create client peer connection\n"); }

  return connection;
}

pul::network::ClientHost pul::network::ClientHost::Construct(
  pul::network::ClientHost::ConstructInfo const & ci
) {
  pul::network::ClientHost client;

  { // -- construct host
    pul::network::Host::ConstructInfo constructInfo;
    constructInfo.address = ci.address;
    constructInfo.isServer = false;
    constructInfo.maxConnections = 1ul;
    constructInfo.maxChannels = Idx(pul::network::ChannelType::Size);
    constructInfo.incomingBandwidth = constructInfo.outgoingBandwidth = 0ul;
    constructInfo.fnConnect    = ci.fnConnect;
    constructInfo.fnDisconnect = ci.fnDisconnect;
    constructInfo.fnReceive    = ci.fnReceive;
    client.host = pul::network::Host::Construct(constructInfo);
  }

  if (!client.host.Valid()) { return client; }

  client.connection =
    pul::network::ClientHostConnection::Construct(client.host, ci.address);

  if (!client.connection.Valid()) { return client; }

  // wait for connection to succeed
  if (
    auto event = client.host.ManualPollEvent(5000u);
    event.Valid() && event.eventType == ENET_EVENT_TYPE_CONNECT
  ) {
    printf("connection to server successful\n");
  } else {
    // TODO clear host connection
    printf("network error - connection to server failed\n");
  }

  return client;
}

pul::network::ServerHost pul::network::ServerHost::Construct(
  pul::network::ServerHost::ConstructInfo const & ci
) {
  pul::network::ServerHost server;

  auto address = pul::network::Address::Construct(ENET_HOST_ANY, ci.port);

  { // -- construct host
    pul::network::Host::ConstructInfo constructInfo;
    constructInfo.address = address;
    constructInfo.isServer = true;
    constructInfo.maxConnections = 1ul;
    constructInfo.maxChannels = Idx(pul::network::ChannelType::Size);
    constructInfo.incomingBandwidth = constructInfo.outgoingBandwidth = 0ul;
    constructInfo.fnConnect = ci.fnConnect;
    constructInfo.fnDisconnect = ci.fnDisconnect;
    constructInfo.fnReceive = ci.fnReceive;
    server.host = pul::network::Host::Construct(constructInfo);
  }

  if (!server.host.Valid()) { return server; }

  return server;
}

template <typename T>
pul::network::OutgoingPacket pul::network::OutgoingPacket::Construct(
  T const & data
, pul::network::ChannelType channelType
) {
  pul::network::OutgoingPacket packet;

  ENetPacketFlag flag = ENET_PACKET_FLAG_UNSEQUENCED;
  switch (channelType) {
    default: break;
    case pul::network::ChannelType::Streaming:
      flag = ENET_PACKET_FLAG_UNSEQUENCED;
    break;
    case pul::network::ChannelType::Reliable:
      flag = ENET_PACKET_FLAG_RELIABLE;
    break;
    case pul::network::ChannelType::Unreliable:
      flag = ENET_PACKET_FLAG_UNSEQUENCED;
    break;
  }

  packet.enetPacket =
    enet_packet_create(reinterpret_cast<void const *>(&data), sizeof(T), flag);
  packet.channel = channelType;

  return packet;
}

// -- explicit instation of OutgoingPacket::Construct
template
pul::network::OutgoingPacket pul::network::OutgoingPacket::Construct(
  pul::network::PacketSystemInfo const & data
, pul::network::ChannelType channelType
);

template
pul::network::OutgoingPacket pul::network::OutgoingPacket::Construct(
  pul::network::PacketNetworkClientUpdate const & data
, pul::network::ChannelType channelType
);

template
pul::network::OutgoingPacket pul::network::OutgoingPacket::Construct(
  pul::network::PacketFileStreamStart const & data
, pul::network::ChannelType channelType
);

template
pul::network::OutgoingPacket pul::network::OutgoingPacket::Construct(
  pul::network::PacketFileStreamBlock const & data
, pul::network::ChannelType channelType
);

void pul::network::OutgoingPacket::Send(
  pul::network::ClientHost & client
) {
  if (!this->enetPacket) {
    printf("network error - Trying to send nullptr / unconstructed packet\n");
    return;
  }

  enet_peer_send(
    client.connection.enetPeer, Idx(this->channel), this->enetPacket
  );
}

void pul::network::OutgoingPacket::Broadcast(
  pul::network::ServerHost & server
) {
  if (!this->enetPacket) {
    printf("network error - Trying to send nullptr / unconstructed packet\n");
    return;
  }

  enet_host_broadcast(
    server.host.enetHost, Idx(this->channel), this->enetPacket
  );
}

char const * ToString(pul::network::OperatingSystem os) {
  switch (os) {
    default: return "N/A";
    case pul::network::OperatingSystem::Linux:
      return "Linux";
    case pul::network::OperatingSystem::Win64:
      return "Win64";
    case pul::network::OperatingSystem::Win32:
      return "Win32";
  }
}

char const * ToString(pul::network::PacketType packetType) {
  using PT = pul::network::PacketType;
  switch (packetType) {
    default: return "N/A";
    case PT::SystemInfo:          return "SystemInfo";
    case PT::NetworkClientUpdate: return "NetworkClientUpdate";
    case PT::StreamStart:         return "StreamStart";
    case PT::StreamBlock:         return "StreamBlock";
  }
}

char const * ToString(pul::network::PacketNetworkClientUpdate::Type type) {
  using PT = pul::network::PacketNetworkClientUpdate::Type;
  switch (type) {
    default: return "N/A";
    case PT::ApplicationUpdate: return "ApplicationUpdate";
    case PT::AssetUpdate:       return "AssetUpdate";
    case PT::ForceRestart:      return "ForceRestart";
    case PT::FullUpdate:        return "FullUpdate";
    case PT::LibraryUpdate:     return "LibraryUpdate";
  }
}
