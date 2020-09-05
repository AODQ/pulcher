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

void EventConnect(pulcher::network::Event & event) {
  spdlog::info(
    "user connected from {}:{}"
  , event.peer->address.host
  , event.peer->address.port
  );
  event.peer->data = nullptr;
}

void EventDisconnect(pulcher::network::Event & event) {
  spdlog::info(
    "{}:{} disconnected"
  , event.peer->address.host, event.peer->address.port
  );
  // TODO free data if applicable
  event.peer->data = nullptr;
}

void EventReceive(pulcher::network::Event & event) {
  spdlog::info(
    "received packet from {}:{}"
  , event.peer->address.host, event.peer->address.port
  );

  spdlog::info(" -- get type {}", ToString(event.type));

  switch (event.type) {
    default: break;
    case pulcher::network::PacketType::SystemInfo:
      auto systemInfo =
        *reinterpret_cast<pulcher::network::PacketSystemInfo*>(
          event.data
        );
      spdlog::info(" -- OS: {}", ToString(systemInfo.operatingSystem));
    break;
  }
}

} // -- anon namespace


int main(int argc, char const ** argv) {
  (void)argc; (void)argv;

  spdlog::info("initializing pulcher server");

  auto network = pulcher::network::Network::Construct();

  pulcher::network::ServerHost server;

  { // -- construct server
    pulcher::network::ServerHost::ConstructInfo ci;
    ci.addressHost = "localhost";
    ci.port = 6599u;
    ci.fnConnect = ::EventConnect;
    ci.fnDisconnect = ::EventDisconnect;
    ci.fnReceive = ::EventReceive;
    server = std::move(pulcher::network::ServerHost::Construct(ci));
  }

  // allow ctrl-c to end process properly by shutting server down
  signal(SIGINT, ::InterruptHandler);

  while (true) {
    while (!::quit) {

      // update server
      server.host.PollEvents();

      std::this_thread::sleep_for(std::chrono::milliseconds(11u));
    }

    spdlog::info("Input a command (quit, force-client-restart)");

    std::string operation;
    std::cin >> operation;

    if (operation == "quit") { break; }
    else if (operation == "force-client-restart") {
      pulcher::network::PacketNetworkClientUpdate clientUpdate;
      clientUpdate.type =
        pulcher::network::PacketNetworkClientUpdate::Type::ForceRestart;

      pulcher::network::OutgoingPacket::Construct(
        clientUpdate
      , pulcher::network::ChannelType::Reliable
      ).Broadcast(server);

      server.host.Flush();

      ::quit = false;
    } else {
      spdlog::error("unknown command");
    }
  }

  printf("\n");
  spdlog::info("shutting server down");
}
