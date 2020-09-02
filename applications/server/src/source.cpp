/* pulcher server | aodq.net */

#include <pulcher-core/config.hpp>
#include <pulcher-core/log.hpp>
#include <pulcher-network/shared.hpp>
#include <pulcher-util/enum.hpp>

#include <cxxopts.hpp>

#include <chrono>
#include <signal.h>
#include <string>
#include <thread>

namespace {

bool quit = false;

extern "C" {
  void InterruptHandler(int) {
    ::quit = true;
  }
}

} // -- anon namespace


int main(int argc, char const ** argv) {
  (void)argc; (void)argv;

  spdlog::info("initializing pulcher server");

  auto time = 100.0;

  if (enet_initialize()) {
    spdlog::critical("could not initialize enet");
    return 0;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 6599u;

  ENetHost * host =
    enet_host_create(
      &address
    , 64u // max connections
    , 2 // max channels (reliable / unreliable)
    , 0, 0 // incoming/outgoing bandwidth assumptions
    );
  std::map<uint32_t, ENetPeer*> clients;

  // allow ctrl-c to end process properly by shutting server down
  signal(SIGINT, ::InterruptHandler);

  while (!::quit) {
    time += 0.01;

    // update server
    ENetEvent event;
    if (enet_host_service(host, &event, 0) > 0) {
      switch (event.type) {
        default: break;
        case ENET_EVENT_TYPE_CONNECT:
          spdlog::info(
            "user connected from {}:{}"
          , event.peer->address.host
          , event.peer->address.port
          );
          event.peer->data = nullptr;
        break;

        case ENET_EVENT_TYPE_RECEIVE:
          spdlog::info(
            "received patch length {} data '{}' from {}:{} channel {}"
          , event.packet->dataLength
          , event.packet->data
          , event.peer->address.host, event.peer->address.port
          , event.channelID
          );


          enet_packet_destroy(event.packet);
        break;

        case ENET_EVENT_TYPE_DISCONNECT:
          spdlog::info(
            "{}:{} disconnected"
          , event.peer->address.host, event.peer->address.port
          );
          // TODO free data if applicable
          event.peer->data = nullptr;
        break;
      }
    }

    // process messages

    // process client inputs

    // send info out to clients

    std::this_thread::sleep_for(std::chrono::milliseconds(11u));
  }

  printf("\n");
  spdlog::info("shutting server down");

  enet_host_destroy(host);
  enet_deinitialize();
}
