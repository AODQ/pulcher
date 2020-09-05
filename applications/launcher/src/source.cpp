/* pulcher | aodq.net */

#include <pulcher-network/shared.hpp>

#include <cxxopts.hpp>
#include <process.hpp>

#pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wshadow"
  #include <spdlog/spdlog.h>
#pragma GCC diagnostic pop

#include <chrono>
#include <memory>
#include <thread>

namespace {

bool quit = false;
bool forceRestart = false;

extern "C" {
  void InterruptHandler(int) {
    ::quit = true;
  }
}

std::string networkIpAddress = "localhost";
uint16_t networkPort = 6599u;

auto StartupOptions() -> cxxopts::Options {
  auto options = cxxopts::Options("core-of-pulcher", "2D shooter game");
  options.add_options()
    (
      "i,ip-address", "ip address to connect to"
    , cxxopts::value<std::string>()->default_value("69.243.92.93")
    ) (
      "p,port", "port number to connect to"
    , cxxopts::value<uint16_t>()->default_value("6599")
    ) (
      "h,help", "print usage"
    )
  ;

  return options;
}

void NetworkClientUpdate(
  pulcher::network::PacketNetworkClientUpdate & networkUpdate
) {
  spdlog::info(" -- applying client update '{}'", ToString(networkUpdate.type));

  using PT = pulcher::network::PacketNetworkClientUpdate::Type;
  switch (networkUpdate.type) {
    case PT::ApplicationUpdate: break;
    case PT::AssetUpdate:       break;
    case PT::ForceRestart:
      forceRestart = true;
    break;
    case PT::FullUpdate:        break;
    case PT::LibraryUpdate:     break;
  }
}

void NetworkConnect(pulcher::network::Event &) {
  spdlog::info("Connected to server");
}

void NetworkDisconnect(pulcher::network::Event &) {
  spdlog::info("Disconnected from server");
}

void NetworkReceive(pulcher::network::Event & event) {
  spdlog::info("received packet type {}", ToString(event.type));

  switch (event.type) {
    case pulcher::network::PacketType::SystemInfo: break;
    case pulcher::network::PacketType::NetworkClientUpdate:
      NetworkClientUpdate(
        *reinterpret_cast<pulcher::network::PacketNetworkClientUpdate *>(
          event.data
        )
      );
    break;
  }
}

pulcher::network::ClientHost CreateNetworkClient() {
  auto clientCi = pulcher::network::ClientHost::ConstructInfo{};
  clientCi.address =
    pulcher::network::Address::Construct(
      ::networkIpAddress.c_str()
    , ::networkPort
    );

  clientCi.fnConnect    = NetworkConnect;
  clientCi.fnDisconnect = NetworkDisconnect;
  clientCi.fnReceive    = NetworkReceive;

  auto client = pulcher::network::ClientHost::Construct(clientCi);

  if (!client.Valid()) { return client; }

  // send which type of operating system this is
  {
    pulcher::network::PacketSystemInfo systemInfo;
    systemInfo.operatingSystem = pulcher::network::currentOperatingSystem;

    pulcher::network::OutgoingPacket::Construct(
      systemInfo
    , pulcher::network::ChannelType::Reliable
    ).Send(client);

    client.host.Flush();
  }

  return client;
}

void CreateLauncherConfig(cxxopts::ParseResult const & parseResult)
{
  try {
    networkIpAddress = parseResult["ip-address"].as<std::string>();
    networkPort      = parseResult["port"].as<uint16_t>();
  } catch (cxxopts::OptionException & parseException) {
    spdlog::critical("{}", parseException.what());
  }
}

} // -- namespace

int main(int argc, char const ** argv) {
  #ifdef __unix__
    spdlog::info("-- running on Linux platform --");
  #elif _WIN64
    spdlog::info("-- running on Windows 64 platform --");
  #elif _WIN32
    spdlog::info("-- running on Windows 32 platform --");
  #endif

  { // -- collect launcher options
    auto options = ::StartupOptions();

    auto userResults = options.parse(argc, argv);

    if (userResults.count("help")) {
      printf("%s\n", options.help().c_str());
      return 0;
    }

    ::CreateLauncherConfig(userResults);
  }

  auto network = pulcher::network::Network::Construct();
  auto networkClient = ::CreateNetworkClient();

  // make process
  auto pulcherProcess =
    std::make_shared<TinyProcessLib::Process>("./pulcher-client");

  // allow ctrl-c to end process properly by shutting applications down
  signal(SIGINT, ::InterruptHandler);

  while (!::quit) {
    int exitStatus;
    if (pulcherProcess->try_get_exit_status(exitStatus)) {
      break;
    }

    networkClient.host.PollEvents();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (::forceRestart) {
      spdlog::info("shutting pulcher process down");
      pulcherProcess->kill();
      pulcherProcess->get_exit_status();

      pulcherProcess =
        std::make_shared<TinyProcessLib::Process>("./pulcher-client");

      ::forceRestart = false;
    }
  }

  printf("\n");
  spdlog::info("shutting pulcher process down");

  pulcherProcess->kill();

  printf("\n");
  spdlog::info("shutting pulcher launcher down");
}
