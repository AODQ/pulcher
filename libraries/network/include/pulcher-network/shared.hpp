#pragma once

#include <enet/enet.h>

#include <functional>

namespace pulcher::network {

  enum struct ChannelType : uint8_t {
    Reliable, Unreliable
  };

  enum struct OperatingSystem : uint8_t {
    Linux, Win32, Win64
  };

  enum struct PQHacketType : uint16_t {
    SystemInfo
  };

  struct Network {
    Network();
    ~Network();
    Network(Network && rval);
    Network(Network const &) = delete;
    Network & operator=(Network && rval);
    Network & operator=(Network const &) = delete;

    bool valid = false;

    static pulcher::network::Network Construct();
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

    std::function<void(pulcher::network::Event & event)>
      fnConnect, fnReceive, fnDisconnect;

    struct ConstructInfo {
      Address address;
      bool isServer;
      size_t maxConnections;
      size_t maxChannels;
      size_t incomingBandwidth, outgoingBandwidth;
      std::function<void(pulcher::network::Event & event)>
        fnConnect, fnReceive, fnDisconnect;
    };

    static pulcher::network::Host Construct(ConstructInfo const & ci);

    bool Valid() const { return static_cast<bool>(this->enetHost); }

    void PollEvents(size_t timeout = 0ul, size_t maxEventPoll = 5ul);

    pulcher::network::Event ManualPollEvent(size_t timeout = 0ul);

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

    static pulcher::network::ClientHost Construct(
      char const * addressHost, uint32_t port
    );

    bool Valid() const {
      return this->host.Valid() && this->connection.Valid();
    }
  };

  struct ServerHost {
    Host host;

    struct ConstructInfo {
      char const * addressHost;
      uint32_t port;
      std::function<void(pulcher::network::Event & event)>
        fnConnect, fnReceive, fnDisconnect;
    };

    static pulcher::network::ServerHost Construct(ConstructInfo const & ci);

    bool Valid() const { return this->host.Valid(); }
  };

  struct OutgoingPacket {
    // memory dealloc managed by enet
    ENetPacket * enetPacket = nullptr;
    pulcher::network::ChannelType channel;

    template <typename T> static OutgoingPacket Construct(
      T const & data, pulcher::network::ChannelType channel
    );

    void Send(ClientHost & client);
  };

  constexpr OperatingSystem currentOperatingSystem =
    #if defined(__unix__)
      pulcher::network::OperatingSystem::Linux
    #elif defined(_WIN64)
      pulcher::network::OperatingSystem::Win64
    #elif defined(_WIN32)
      pulcher::network::OperatingSystem::Win32
    #endif
  ;


  // -- packets

  struct PacketSystemInfo {
    PacketType const packetType = PacketType::SystemInfo;
    pulcher::network::OperatingSystem operatingSystem;
  };
}

char const * ToString(pulcher::network::OperatingSystem os);
char const * ToString(pulcher::network::PacketType packetType);
