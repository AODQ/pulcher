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

void ProcessTestMessage(
  size_t clientIdx,
  pulcher::network::TestMessage & message
) {
  spdlog::info("idx: {} Message: {}", clientIdx, message.sequence);
}

void ProcessMessage(size_t clientIdx, yojimbo::Message & message) {
  switch (message.GetType()) {
    case static_cast<int>(pulcher::network::MessageType::Test):
      ProcessTestMessage(
        clientIdx
      , dynamic_cast<pulcher::network::TestMessage &>(message)
      );
    break;
  }
}

void ProcessMessages(yojimbo::Server & server) {
  for (size_t clientIdx = 0ul; clientIdx < 64ul; ++ clientIdx) {
    if (server.IsClientConnected(clientIdx)) { continue; }

    for (
        size_t channelIdx = 0ul;
        channelIdx < Idx(pulcher::network::Channel::Size);
        ++ channelIdx
    ) {
      while (auto message = server.ReceiveMessage(clientIdx, channelIdx)) {
        ProcessMessage(clientIdx, *message);
        server.ReleaseMessage(channelIdx, message);
      }
    }
  }
}

} // -- anon namespace


int main(int argc, char const ** argv) {
  (void)argc; (void)argv;

  spdlog::info("initializing pulcher server");

  if (!InitializeYojimbo()) {
    spdlog::error("failed to initialize Yojimbo");
    return 0;
  }

  yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);
  srand(static_cast<uint32_t>(time(nullptr)));

  std::array<uint8_t, yojimbo::KeyBytes> privateKey;
  std::fill(privateKey.begin(), privateKey.end(), 0u);

  pulcher::network::Adapter adapter;

  auto server =
    yojimbo::Server(
      yojimbo::GetDefaultAllocator()
    , privateKey.data()
    , yojimbo::Address("127.0.0.1", 40000u)
    , pulcher::network::CreateClientServerConfig(), adapter, 100.0
    );

  server.Start(64u);

  if (!server.IsRunning()) {
    spdlog::critical("Failed to start server");
    return 0;
  }

  auto time = 100.0;

  // allow ctrl-c to end process properly by shutting server down
  signal(SIGINT, ::InterruptHandler);

  while (!::quit && server.IsRunning())
  {
    time += 0.01;

    // update server
    server.AdvanceTime(time);
    server.ReceivePackets();

    // process messages
    ::ProcessMessages(server);

    // process client inputs

    // send info out to clients

    server.SendPackets();

    /* std::this_thread::sleep_for(std::chrono::milliseconds(10u)); */
    yojimbo_sleep(0.01);
  }

  printf("\n");
  spdlog::info("shutting server down");

  server.Stop();

  ShutdownYojimbo();
}
