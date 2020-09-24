/* pulcher server | aodq.net */

#include <pulcher-network/shared.hpp>

#pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wshadow"
  #include <spdlog/spdlog.h>
#pragma GCC diagnostic pop

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <string>
#include <thread>

namespace {

bool quit = false;
std::vector<std::string> filesToUpload;
std::ifstream streamFile;
std::string streamFilename;
size_t streamRemainingBlocks = -1ul;
size_t streamOriginalRemainingBlocks = -1ul;
size_t lastPacketLength = 0u;

extern "C" {
  void InterruptHandler(int) {
    ::quit = true;

    if (streamRemainingBlocks != -1ul) {
      spdlog::error("stopping client file upload");
      filesToUpload = {};
      streamFile = {};
      streamRemainingBlocks = -1ul;
    }
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

void UpdateFileStream(pulcher::network::ServerHost & server) {
  if (filesToUpload.size() == 0ul && !streamFile.is_open()) { return; }

  // -- load file for streaming
  if (!streamFile.is_open()) {
    std::string filename = filesToUpload.back();
    filesToUpload.pop_back();
    streamFile = std::ifstream(filename, std::ios::binary);
    streamFilename = filename;
    if (!streamFile.good()) {
      spdlog::critical("Could not open {} for writing", filename);
      streamFile = {};
    }

    { // setup starting information
      auto byteLength = std::filesystem::file_size(filename);

      auto const streamLen = pulcher::network::StreamBlockByteLength;

      // calculate number of blocks necessary and round up
      streamRemainingBlocks =
        byteLength / streamLen + (byteLength % streamLen ? 1ul : 0ul);
      streamOriginalRemainingBlocks = streamRemainingBlocks;

      // get remaining packets, though if it's 0 then streamLen is necessary
      lastPacketLength = byteLength % streamLen;
      if (lastPacketLength == 0ul) { lastPacketLength = streamLen; }
    }

    spdlog::info(
      "sending file {} with {} blocks and last packet length {}",
      filename, streamRemainingBlocks, lastPacketLength
    );

    if (filename.size() > 1023ul) {
      spdlog::critical("filename is too large to transmit over network");
      return;
    }

    { // send file start packet over network
      pulcher::network::PacketFileStreamStart packetStream;

      // copy string over to char array
      memset(packetStream.filename.data(), '\0', packetStream.filename.size());
      memcpy(packetStream.filename.data(), filename.data(), filename.size());

      // misc settings
      packetStream.incomingPacketLength = streamRemainingBlocks;
      packetStream.lastPacketLength = lastPacketLength;

      // send packet over network
      pulcher::network::OutgoingPacket::Construct(
        packetStream
      , pulcher::network::ChannelType::Reliable
      ).Broadcast(server);

      server.host.Flush();
    }

    return;
  }

  for (size_t it = 0ul; it < 20ul; ++ it) {

    if (!streamFile.good()) {
      spdlog::critical(" -- file no longer in a good condition to write to");
      streamRemainingBlocks = -1ul;
      lastPacketLength      = 0u;
      streamFile = {};
      return;
    }

    // continue sending data over network
    size_t streamLen = pulcher::network::StreamBlockByteLength;
    if (streamRemainingBlocks == 1ul) { streamLen = lastPacketLength; }

    if (streamLen == 0ul) {
      spdlog::critical("file stream block has a length of 0");
      streamRemainingBlocks = -1ul;
      return;
    }

    { // -- display progress
      printf("\r[");

      float const progressPercent =
        1.0f
      - (
          streamRemainingBlocks
        / static_cast<float>(streamOriginalRemainingBlocks)
        )
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

    pulcher::network::PacketFileStreamBlock packetStream;
    memset(packetStream.data.data(), 0ul, packetStream.data.size());

    // read in data
    streamFile
      .read(reinterpret_cast<char *>(packetStream.data.data()), streamLen);

    // send packet over network
    pulcher::network::OutgoingPacket::Construct(
      packetStream
    , pulcher::network::ChannelType::Reliable
    ).Broadcast(server);

    server.host.Flush();

    // check if finished streaming
    if (-- streamRemainingBlocks == 0ul) {
      streamRemainingBlocks = -1ul;
      lastPacketLength      = 0u;
      streamFile            = {};
      printf("\n");
      spdlog::info(" -- finished streaming file");
      break;
    }

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
    ci.port         = 6599u;
    ci.fnConnect    = ::EventConnect;
    ci.fnDisconnect = ::EventDisconnect;
    ci.fnReceive    = ::EventReceive;
    server = pulcher::network::ServerHost::Construct(ci);
  }

  // allow ctrl-c to end process properly by shutting server down
  signal(SIGINT, ::InterruptHandler);

  while (true) {
    while (!::quit) {

      // update server
      server.host.PollEvents();

      ::UpdateFileStream(server);

      std::this_thread::sleep_for(std::chrono::milliseconds(11u));
    }

    if (filesToUpload.size()) {
      spdlog::critical("can not exit while files are uploading");
      ::quit = false;
    }

    spdlog::info("Input a command (quit, force-client-restart, file-reload)");

    std::string operation;
    std::cin >> operation;

    if (operation == "\0" || operation == "quit") { break; }
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
    } else if (operation == "file-reload") {
      pulcher::network::PacketNetworkClientUpdate clientUpdate;
      clientUpdate.type =
        pulcher::network::PacketNetworkClientUpdate::Type::ApplicationUpdate;

      pulcher::network::OutgoingPacket::Construct(
        clientUpdate
      , pulcher::network::ChannelType::Reliable
      ).Broadcast(server);

      server.host.Flush();

      ::quit = false;

      filesToUpload = {
        "pulcher-client"
      , "plugins/ui-base.pulcher-plugin"
      , "plugins/plugin-map.pulcher-plugin"
      , "plugins/plugin-entity.pulcher-plugin"
      , "plugins/plugin-physics.pulcher-plugin"
      , "lib/libglad.so", "lib/libimgui.so", "lib/libspdlog.so"
      , "lib/libsokol.so"
      , "base/spritesheets/player/nygelstromn.png"
      , "base/spritesheets/player/data.json"
      , "base/spritesheets/map/calamity.png"
      , "base/spritesheets/map/lupanar.png"
      , "base/spritesheets/map/lupanar.png"
      , "base/spritesheets/map/lupanar.png"
      , "base/map/test.json"
      , "base/map/lupanar-tileset.json"
      , "base/map/calamity.json"
      , "pulcher-launcher"
      };

    } else {
      spdlog::error("unknown command");
      ::quit = false;
    }
  }

  printf("\n");
  spdlog::info("shutting server down");
}
