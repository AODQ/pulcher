/* pulcher | aodq.net */

#include <pulcher-core/config.hpp>
#include <pulcher-core/log.hpp>
#include <pulcher-gfx/context.hpp>

#include <cxxopts.hpp>

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

auto StartupOptions() -> cxxopts::Options {
  auto options = cxxopts::Options("core-of-pulcher", "2D shooter game");
  options.add_options()
    (
      "i,ip-address", "ip address to connect to"
    , cxxopts::value<std::string>()->default_value("69.243.92.93")
    ) (
      "r,resolution", "window resolution (0x0 means display resolution)"
    , cxxopts::value<std::string>()->default_value("0x0")
    ) (
      "p,port", "port number to connect to"
    , cxxopts::value<uint16_t>()->default_value("6599")
    ) (
      "h,help", "print usage"
    )
  ;

  return options;
}

auto CreateUserConfig(cxxopts::ParseResult const & userResults)
  -> pulcher::core::Config
{
  pulcher::core::Config config;

  std::string windowResolution;

  try {
    config.networkIpAddress   = userResults["ip-address"].as<std::string>();
    config.networkPortAddress = userResults["port"].as<uint16_t>();
    windowResolution = userResults["resolution"].as<std::string>();
  } catch (cxxopts::OptionException & parseException) {
    spdlog::critical("{}", parseException.what());
  }

  // -- window resolution
  if (
      auto xLen = windowResolution.find("x");
      xLen != std::string::npos && xLen < windowResolution.size()-1ul
  ) {
    config.windowWidth =
      static_cast<uint16_t>(std::stoi(windowResolution.substr(0ul, xLen)));
    config.windowHeight =
      static_cast<uint16_t>(
        std::stoi(windowResolution.substr(xLen, windowResolution.size()))
      );
  } else {
    spdlog::error("window resolution incorrect format, must use WxH");
  }

  return config;
}

void PrintUserConfig(pulcher::core::Config const & config) {
  if (config.networkIpAddress != "") {
    spdlog::info(
      "will connect to '{}:{}'"
    , config.networkIpAddress, config.networkPortAddress
    );
  }
}

} // -- anon namespace

int main(int argc, char const ** argv) {

  pulcher::core::Config userConfig;

  { // -- collect user options
    auto options = ::StartupOptions();

    auto userResults = options.parse(argc, argv);

    if (userResults.count("help")) {
      printf("%s\n", options.help().c_str());
      return 0;
    }

    userConfig = ::CreateUserConfig(userResults);
  }

  spdlog::info("initializing pulcher");

  ::PrintUserConfig(userConfig);

  pulcher::gfx::InitializeContext(userConfig);

  return 0;
}
