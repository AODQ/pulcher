/* pulcher | aodq.net */

#include <pulcher-network/shared.hpp>

#include <cxxopts.hpp>
#include <process.hpp>

#pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wshadow"
  #include <spdlog/spdlog.h>
#pragma GCC diagnostic pop

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <thread>

#ifdef __unix__
  #include <sys/stat.h> // for chmod
#endif

namespace {

bool quit = false;
bool forceRestart = false;
bool quitForUpdate = false;

#ifdef __unix__
  std::string const pulcherExecutableFilename = "pulcher-client";
  std::string const launcherExecutableFilename = "pulcher-launcher";
#else
  std::string const pulcherExecutableFilename = "pulcher-client.exe";
  std::string const launcherExecutableFilename = "pulcher-launcher.exe";
#endif

extern "C" {
  void InterruptHandler(int) {
    ::quit = true;
  }
}

std::string networkIpAddress = "localhost";
uint16_t networkPort = 6599u;
std::shared_ptr<TinyProcessLib::Process> pulcherProcess;

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

std::ofstream streamFile;
std::string streamFilename;
size_t streamRemainingBlocks = -1ul;
size_t streamOriginalRemainingBlocks = -1ul;
size_t lastPacketLength = 0u;

void FileStreamStart(pulcher::network::PacketFileStreamStart stream) {
  streamFilename = std::string{stream.filename.data()};

  // check if need to close down application
  if (streamFilename == pulcherExecutableFilename && pulcherProcess) {
    ::pulcherProcess->kill();
    ::pulcherProcess->get_exit_status();
    ::pulcherProcess = {};
  }

  if (streamFilename == launcherExecutableFilename) {
    streamFilename += "-emit";
  }

  spdlog::info(
    "streaming file '{}' using {} packets, last packet will be {} bytes"
  , streamFilename
  , stream.incomingPacketLength
  , stream.lastPacketLength
  );

  if (streamRemainingBlocks != -1ul) {
    spdlog::critical(" -- another file is already being transmitted");
    return;
  }

  { // create necessary directories
    auto directory = std::filesystem::path(streamFilename);
    directory.remove_filename();
    if (!directory.empty())
      { std::filesystem::create_directories(directory); }
  }

  streamFile = std::ofstream(streamFilename, std::ios::out | std::ios::binary);

  streamRemainingBlocks         = stream.incomingPacketLength;
  streamOriginalRemainingBlocks = stream.incomingPacketLength;
  lastPacketLength              = stream.lastPacketLength;

  if (!streamFile.good()) {
    spdlog::critical(" -- failed to open file for writing");
    streamRemainingBlocks = -1ul;
    lastPacketLength      = 0u;
  }
}

void FileStreamBlock(pulcher::network::PacketFileStreamBlock stream) {
  if (!streamFile.good()) {
    spdlog::critical(" -- file no longer in a good condition to write to");
    streamRemainingBlocks = -1ul;
    lastPacketLength      = 0u;
    streamFile = {};
    return;
  }

  { // -- display progress
    printf("\r[");

    float const progressPercent =
      1.0f
    - (streamRemainingBlocks/static_cast<float>(streamOriginalRemainingBlocks))
    ;

    auto pos = static_cast<size_t>(20.0f * progressPercent);

    for (size_t i = 0; i < 20; ++ i) {
      if (i < pos)       printf("=");
      else if (i == pos) printf(">");
      else               printf(" ");
    }

    printf(
      "] %s %.2f%%"
    , streamFilename.c_str()
    , static_cast<double>(progressPercent*100.0f)
    );

    fflush(stdout);
  }

  // first locate the end of packet data
  size_t streamLen = stream.data.size();
  if (streamRemainingBlocks == 1ul) { streamLen = lastPacketLength; }

  if (streamLen == 0ul) {
    printf("\n");
    spdlog::critical("file stream block has a length of 0");
    streamRemainingBlocks = -1ul;
    return;
  }

  // write out data
  streamFile.write(reinterpret_cast<char *>(stream.data.data()), streamLen);

  // iterate & check if EOF
  if (-- streamRemainingBlocks == 0ul) {
    streamFile.close();
    streamRemainingBlocks = -1ul;
    lastPacketLength      = 0u;
    streamFile            = {};
    printf("\n");
    spdlog::info("-- finished writing to file");

    #ifdef __unix__
      spdlog::info("setting file permission");
      chmod(streamFilename.c_str(), 755);
    #endif

    if (streamFilename == launcherExecutableFilename+"-emit") {
      spdlog::info("renaming file");
      std::filesystem::rename(
        launcherExecutableFilename+"-emit"
      , launcherExecutableFilename
      );
    }
  }
}

void NetworkConnect(pulcher::network::Event &) {
  spdlog::info("Connected to server");
}

void NetworkDisconnect(pulcher::network::Event &) {
  spdlog::info("Disconnected from server");
}

void NetworkReceive(pulcher::network::Event & event) {
  switch (event.type) {
    case pulcher::network::PacketType::SystemInfo: break;
    case pulcher::network::PacketType::StreamStart:
      FileStreamStart(
        *reinterpret_cast<pulcher::network::PacketFileStreamStart *>(event.data)
      );
    break;
    case pulcher::network::PacketType::StreamBlock:
      FileStreamBlock(
        *reinterpret_cast<pulcher::network::PacketFileStreamBlock *>(event.data)
      );
    break;
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

  // will attempt to launch client
  ::forceRestart = true;

  // allow ctrl-c to end process properly by shutting applications down
  signal(SIGINT, ::InterruptHandler);

  while (!::quit) {

    // check for exit
    if (
      int exitStatus;
      ::pulcherProcess && ::pulcherProcess->try_get_exit_status(exitStatus)
    ) {
      break;
    }

    // update network
    networkClient.host.PollEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(11));

    // check for reboot
    if (::forceRestart) {
      spdlog::info("rebooting pulcher process");

      if (::pulcherProcess) {
        ::pulcherProcess->kill();
        ::pulcherProcess->get_exit_status();
      }

      if (std::filesystem::exists(pulcherExecutableFilename)) {
        ::pulcherProcess =
          std::make_shared<TinyProcessLib::Process>(
            "./"+pulcherExecutableFilename
          );
      }
      ::forceRestart = false;
    }

    // check for update
    if (::quitForUpdate) {
      spdlog::info("shutting pulcher process down");
      if (::pulcherProcess) {
        ::pulcherProcess->kill();
        ::pulcherProcess->get_exit_status();
        ::pulcherProcess = {};
      }

      ::quitForUpdate = false;
    }
  }

  printf("\n");
  spdlog::info("shutting pulcher process down");

  if (::pulcherProcess) {
    ::pulcherProcess->kill();
    ::pulcherProcess = {};
  }

  printf("\n");
  spdlog::info("shutting pulcher launcher down");
}
