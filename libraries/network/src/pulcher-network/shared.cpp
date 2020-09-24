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

pulcher::network::Network::Network() {}

pulcher::network::Network::~Network() {
  if (valid) { enet_deinitialize(); }
}

pulcher::network::Network::Network(pulcher::network::Network && rval) {
  this->valid = rval.valid;
  rval.valid = false;
}

pulcher::network::Network & pulcher::network::Network::operator=(
  pulcher::network::Network && rval
) {
  this->valid = rval.valid;
  rval.valid = false;
  return *this;
}


pulcher::network::Network pulcher::network::Network::Construct() {
  pulcher::network::Network network;
  network.valid = !enet_initialize();
  return network;
}

pulcher::network::Address pulcher::network::Address::Construct(
  char const * addressHost
, uint32_t port
) {
  pulcher::network::Address address;

  if (addressHost) {
    enet_address_set_host(&address.enetAddress, addressHost);
  } else {
    address.enetAddress.host = ENET_HOST_ANY;
  }
  address.enetAddress.port = port;

  return address;
}

pulcher::network::IncomingPacket::IncomingPacket() {}

pulcher::network::IncomingPacket::~IncomingPacket() {
  if (this->enetPacket)
    { enet_packet_destroy(this->enetPacket); }
  this->enetPacket = nullptr;
}

pulcher::network::IncomingPacket::IncomingPacket(
  pulcher::network::IncomingPacket && rval
) {
  this->enetPacket = rval.enetPacket;
  rval.enetPacket = nullptr;
}

pulcher::network::IncomingPacket & pulcher::network::IncomingPacket::operator=(
  pulcher::network::IncomingPacket && rval
) {
  this->enetPacket = rval.enetPacket;
  rval.enetPacket = nullptr;
  return *this;
}

pulcher::network::Host::Host() {}

pulcher::network::Host::~Host() {
  if (this->enetHost) {
    enet_host_destroy(this->enetHost);
  }
  this->enetHost = nullptr;
}

pulcher::network::Host::Host(pulcher::network::Host && rval) {
  this->enetHost = rval.enetHost;

  this->fnConnect    = rval.fnConnect;
  this->fnReceive    = rval.fnReceive;
  this->fnDisconnect = rval.fnDisconnect;

  rval.enetHost = nullptr;
}

pulcher::network::Host & pulcher::network::Host::operator=(
  pulcher::network::Host && rval
) {
  this->enetHost = rval.enetHost;

  this->fnConnect    = rval.fnConnect;
  this->fnReceive    = rval.fnReceive;
  this->fnDisconnect = rval.fnDisconnect;

  rval.enetHost = nullptr;

  return *this;
}

pulcher::network::Host
pulcher::network::Host::Construct(
  pulcher::network::Host::ConstructInfo const & ci
) {
  pulcher::network::Host host;

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

void pulcher::network::Host::PollEvents(size_t timeout, size_t maxEventPoll) {
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

pulcher::network::Event
pulcher::network::Host::ManualPollEvent(size_t timeout) {
  pulcher::network::Event event;

  ENetEvent enetEvent;
  event.pollResult =
    enet_host_service(this->enetHost, &enetEvent, timeout);
  event.peer = enetEvent.peer;
  event.eventType = enetEvent.type;

  event.packet.enetPacket = enetEvent.packet;

  // get type/data if receive packet
  if (event.eventType == ENET_EVENT_TYPE_RECEIVE) {
    event.type =
      *reinterpret_cast<pulcher::network::PacketType *>(
        event.packet.enetPacket->data
      );
    event.data = event.packet.enetPacket->data;
  }

  return event;
}

void pulcher::network::Host::Flush() {
  enet_host_flush(this->enetHost);
}

pulcher::network::ClientHostConnection::ClientHostConnection() {}
pulcher::network::ClientHostConnection::~ClientHostConnection() {
  if (enetPeer != nullptr) {
    enet_peer_reset(enetPeer);
  }
  enetPeer = nullptr;
}

pulcher::network::ClientHostConnection::ClientHostConnection(
  pulcher::network::ClientHostConnection && rval
) {
  this->enetPeer = rval.enetPeer;
  rval.enetPeer = nullptr;
}

pulcher::network::ClientHostConnection &
pulcher::network::ClientHostConnection::operator=(
  pulcher::network::ClientHostConnection && rval
) {
  this->enetPeer = rval.enetPeer;
  rval.enetPeer = nullptr;
  return *this;
}

pulcher::network::ClientHostConnection
pulcher::network::ClientHostConnection::Construct(
  pulcher::network::Host & host
, pulcher::network::Address const & address
) {
  pulcher::network::ClientHostConnection connection;
  connection.enetPeer =
    enet_host_connect(host.enetHost, &address.enetAddress, 2, 0);

  if (!connection.Valid())
    { printf("network error - could not create client peer connection\n"); }

  return connection;
}

pulcher::network::ClientHost pulcher::network::ClientHost::Construct(
  pulcher::network::ClientHost::ConstructInfo const & ci
) {
  pulcher::network::ClientHost client;

  { // -- construct host
    pulcher::network::Host::ConstructInfo constructInfo;
    constructInfo.address = ci.address;
    constructInfo.isServer = false;
    constructInfo.maxConnections = 1ul;
    constructInfo.maxChannels = Idx(pulcher::network::ChannelType::Size);
    constructInfo.incomingBandwidth = constructInfo.outgoingBandwidth = 0ul;
    constructInfo.fnConnect    = ci.fnConnect;
    constructInfo.fnDisconnect = ci.fnDisconnect;
    constructInfo.fnReceive    = ci.fnReceive;
    client.host = pulcher::network::Host::Construct(constructInfo);
  }

  if (!client.host.Valid()) { return client; }

  client.connection =
    pulcher::network::ClientHostConnection::Construct(client.host, ci.address);

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

pulcher::network::ServerHost pulcher::network::ServerHost::Construct(
  pulcher::network::ServerHost::ConstructInfo const & ci
) {
  pulcher::network::ServerHost server;

  auto address = pulcher::network::Address::Construct(ENET_HOST_ANY, ci.port);

  { // -- construct host
    pulcher::network::Host::ConstructInfo constructInfo;
    constructInfo.address = address;
    constructInfo.isServer = true;
    constructInfo.maxConnections = 1ul;
    constructInfo.maxChannels = Idx(pulcher::network::ChannelType::Size);
    constructInfo.incomingBandwidth = constructInfo.outgoingBandwidth = 0ul;
    constructInfo.fnConnect = ci.fnConnect;
    constructInfo.fnDisconnect = ci.fnDisconnect;
    constructInfo.fnReceive = ci.fnReceive;
    server.host = pulcher::network::Host::Construct(constructInfo);
  }

  if (!server.host.Valid()) { return server; }

  return server;
}

template <typename T>
pulcher::network::OutgoingPacket pulcher::network::OutgoingPacket::Construct(
  T const & data
, pulcher::network::ChannelType channelType
) {
  pulcher::network::OutgoingPacket packet;

  ENetPacketFlag flag = ENET_PACKET_FLAG_UNSEQUENCED;
  switch (channelType) {
    default: break;
    case pulcher::network::ChannelType::Streaming:
      flag = ENET_PACKET_FLAG_UNSEQUENCED;
    break;
    case pulcher::network::ChannelType::Reliable:
      flag = ENET_PACKET_FLAG_RELIABLE;
    break;
    case pulcher::network::ChannelType::Unreliable:
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
pulcher::network::OutgoingPacket pulcher::network::OutgoingPacket::Construct(
  pulcher::network::PacketSystemInfo const & data
, pulcher::network::ChannelType channelType
);

template
pulcher::network::OutgoingPacket pulcher::network::OutgoingPacket::Construct(
  pulcher::network::PacketNetworkClientUpdate const & data
, pulcher::network::ChannelType channelType
);

template
pulcher::network::OutgoingPacket pulcher::network::OutgoingPacket::Construct(
  pulcher::network::PacketFileStreamStart const & data
, pulcher::network::ChannelType channelType
);

template
pulcher::network::OutgoingPacket pulcher::network::OutgoingPacket::Construct(
  pulcher::network::PacketFileStreamBlock const & data
, pulcher::network::ChannelType channelType
);

void pulcher::network::OutgoingPacket::Send(
  pulcher::network::ClientHost & client
) {
  if (!this->enetPacket) {
    printf("network error - Trying to send nullptr / unconstructed packet\n");
    return;
  }

  enet_peer_send(
    client.connection.enetPeer, Idx(this->channel), this->enetPacket
  );
}

void pulcher::network::OutgoingPacket::Broadcast(
  pulcher::network::ServerHost & server
) {
  if (!this->enetPacket) {
    printf("network error - Trying to send nullptr / unconstructed packet\n");
    return;
  }

  enet_host_broadcast(
    server.host.enetHost, Idx(this->channel), this->enetPacket
  );
}

char const * ToString(pulcher::network::OperatingSystem os) {
  switch (os) {
    default: return "N/A";
    case pulcher::network::OperatingSystem::Linux:
      return "Linux";
    case pulcher::network::OperatingSystem::Win64:
      return "Win64";
    case pulcher::network::OperatingSystem::Win32:
      return "Win32";
  }
}

char const * ToString(pulcher::network::PacketType packetType) {
  using PT = pulcher::network::PacketType;
  switch (packetType) {
    default: return "N/A";
    case PT::SystemInfo:          return "SystemInfo";
    case PT::NetworkClientUpdate: return "NetworkClientUpdate";
    case PT::StreamStart:         return "StreamStart";
    case PT::StreamBlock:         return "StreamBlock";
  }
}

char const * ToString(pulcher::network::PacketNetworkClientUpdate::Type type) {
  using PT = pulcher::network::PacketNetworkClientUpdate::Type;
  switch (type) {
    default: return "N/A";
    case PT::ApplicationUpdate: return "ApplicationUpdate";
    case PT::AssetUpdate:       return "AssetUpdate";
    case PT::ForceRestart:      return "ForceRestart";
    case PT::FullUpdate:        return "FullUpdate";
    case PT::LibraryUpdate:     return "LibraryUpdate";
  }
}
