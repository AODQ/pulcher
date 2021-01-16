#pragma once

#include <enet/enet.h>

#include <array>
#include <cstdint>
#include <functional>

namespace pul::network {

  enum struct ChannelType : uint8_t {
    Reliable, Unreliable, Streaming
  , Size
  };

  enum struct OperatingSystem : uint8_t {
    Linux, macOS, Win32, Win64
  };

  enum struct PacketType : uint16_t {
    SystemInfo
  , NetworkClientUpdate
  , StreamStart
  , StreamBlock
  };

  struct Network {
    Network();
    ~Network();
    Network(Network && rval);
    Network(Network const &) = delete;
    Network & operator=(Network && rval);
    Network & operator=(Network const &) = delete;

    bool valid = false;

    static pul::network::Network Construct();
  };

  struct Address {
    ENetAddress enetAddress;

    static Address Construct(char const * addressHost, uint32_t port);
  };

  struct IncomingPacket {
    IncomingPacket();
    ~IncomingPacket();
    IncomingPacket(IncomingPacket && rval);
    IncomingPacket(IncomingPacket const &) = delete;
    IncomingPacket & operator=(IncomingPacket && rval);
    IncomingPacket & operator=(IncomingPacket const &) = delete;

    ENetPacket * enetPacket = nullptr;
  };

  struct Event {
    IncomingPacket packet;
    ENetPeer * peer = nullptr;
    ENetEventType eventType;
    size_t pollResult = 0ul;

    PacketType type;
    void * data = nullptr;

    bool Valid() { return pollResult > 0ul; }
  };

  struct Host {
    Host();
    ~Host();
    Host(Host && rval);
    Host(Host const &) = delete;
    Host & operator=(Host && rval);
    Host & operator=(Host const &) = delete;

    ENetHost * enetHost = nullptr;

    std::function<void(pul::network::Event & event)>
      fnConnect, fnReceive, fnDisconnect;

    struct ConstructInfo {
      Address address;
      bool isServer;
      size_t maxConnections;
      size_t maxChannels;
      size_t incomingBandwidth, outgoingBandwidth;
      std::function<void(pul::network::Event & event)>
        fnConnect, fnReceive, fnDisconnect;
    };

    static pul::network::Host Construct(ConstructInfo const & ci);

    bool Valid() const { return static_cast<bool>(this->enetHost); }

    void PollEvents(size_t timeout = 0ul, size_t maxEventPoll = 20ul);

    pul::network::Event ManualPollEvent(size_t timeout = 0ul);

    void Flush();
  };

  struct ClientHostConnection {
    ClientHostConnection();
    ~ClientHostConnection();
    ClientHostConnection(ClientHostConnection && rval);
    ClientHostConnection(ClientHostConnection const &) = delete;
    ClientHostConnection & operator=(ClientHostConnection && rval);
    ClientHostConnection & operator=(ClientHostConnection const &) = delete;

    ENetPeer * enetPeer = nullptr;

    static ClientHostConnection Construct(Host & host, Address const & address);

    bool Valid() const { return static_cast<bool>(this->enetPeer); }
  };

  struct ClientHost {
    Host host;
    ClientHostConnection connection;

    struct ConstructInfo {
      Address address;
      std::function<void(pul::network::Event & event)>
        fnConnect, fnReceive, fnDisconnect;
    };

    static pul::network::ClientHost Construct(ConstructInfo const & ci);

    bool Valid() const {
      return this->host.Valid() && this->connection.Valid();
    }
  };

  struct ServerHost {
    Host host;

    struct ConstructInfo {
      uint32_t port;
      std::function<void(pul::network::Event & event)>
        fnConnect, fnReceive, fnDisconnect;
    };

    static pul::network::ServerHost Construct(ConstructInfo const & ci);

    bool Valid() const { return this->host.Valid(); }
  };

  struct OutgoingPacket {
    // memory dealloc managed by enet
    ENetPacket * enetPacket = nullptr;
    pul::network::ChannelType channel;

    template <typename T> static OutgoingPacket Construct(
      T const & data, pul::network::ChannelType channel
    );

    void Send(ClientHost & client);
    void Broadcast(ServerHost & server);
  };

  constexpr OperatingSystem currentOperatingSystem =
    #if defined(__unix__)
      pul::network::OperatingSystem::Linux
    #elif defined(__APPLE__)
      pul::network::OperatingSystem::macOS
    #elif defined(_WIN64)
      pul::network::OperatingSystem::Win64
    #elif defined(_WIN32)
      pul::network::OperatingSystem::Win32
    #endif
  ;


  // -- packets

  struct PacketSystemInfo {
    PacketType const packetType = PacketType::SystemInfo;
    pul::network::OperatingSystem operatingSystem;
  };

  struct PacketNetworkClientUpdate {
    PacketType const packetType = PacketType::NetworkClientUpdate;

    enum struct Type : uint8_t {
      ApplicationUpdate
    , AssetUpdate
    , ForceRestart
    , FullUpdate
    , LibraryUpdate
    };

    Type type;
  };

  struct PacketFileStreamStart {
    PacketType const packetType = PacketType::StreamStart;

    std::array<char, 1024ul> filename;
    size_t incomingPacketLength;
    uint16_t lastPacketLength;
  };

  constexpr size_t StreamBlockByteLength = 4096ul;

  struct PacketFileStreamBlock {
    PacketType const packetType = PacketType::StreamBlock;

    std::array<uint8_t, StreamBlockByteLength> data;
  };
}

char const * ToString(pul::network::OperatingSystem os);
char const * ToString(pul::network::PacketType packetType);
char const * ToString(pul::network::PacketNetworkClientUpdate::Type type);
