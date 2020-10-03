/* pulcher | aodq.net */

#pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wshadow"
  #include <argparse/argparse.hpp>
#pragma GCC diagnostic pop

#include <process.hpp>

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

std::shared_ptr<TinyProcessLib::Process> pulcherProcess;

auto StartupOptions() -> argparse::ArgumentParser {
  auto options = argparse::ArgumentParser("pulcher-launcher");
  options
    .add_argument("-i")
    .help("ip address to connect to")
    .default_value(std::string("192.168.0.1"))
  ;

  return options;
}

void CreateLauncherConfig(argparse::ArgumentParser const & parseResult) {
}

} // -- namespace

int main(int argc, char const ** argv) {
  #ifdef __unix__
    printf("-- running on Linux platform --\n");
  #elif _WIN64
    printf("-- running on Windows 64 platform --\n");
  #elif _WIN32
    printf("-- running on Windows 32 platform --\n");
  #endif

  { // -- collect launcher options
    auto options = ::StartupOptions();

    options.parse_args(argc, argv);

    /* if (userResults.count("help")) { */
    /*   printf("%s\n", options.help().c_str()); */
    /*   return 0; */
    /* } */

    ::CreateLauncherConfig(options);
  }

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

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // check for reboot
    if (::forceRestart) {
      printf("rebooting pulcher process\n");

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
      printf("shutting pulcher process down\n");
      if (::pulcherProcess) {
        ::pulcherProcess->kill();
        ::pulcherProcess->get_exit_status();
        ::pulcherProcess = {};
      }

      ::quitForUpdate = false;
    }
  }

  printf("\n");
  printf("shutting pulcher process down\n");

  if (::pulcherProcess) {
    ::pulcherProcess->kill();
    ::pulcherProcess = {};
  }

  printf("\n");
  printf("shutting pulcher launcher down\n");
}
