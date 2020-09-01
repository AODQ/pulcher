#include <pulcher-network/shared.hpp>

#include <pulcher-util/enum.hpp>

int GetNumBitsForMessage(uint16_t sequence)
{
    static int messageBitsArray[] = {
      1, 320, 120, 4, 256, 45, 11, 13, 101, 100, 84, 95, 203, 2, 3, 8, 512
    , 5, 3, 7, 50
    };

    const int modulus = sizeof( messageBitsArray ) / sizeof( int );
    const int index = sequence % modulus;
    return messageBitsArray[index];
}

yojimbo::ClientServerConfig pulcher::network::CreateClientServerConfig() {
  yojimbo::ClientServerConfig config;
  config.numChannels = 2;
  config.channel[Idx(pulcher::network::Channel::Reliable)].type =
    yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
  config.channel[Idx(pulcher::network::Channel::Unreliable)].type =
    yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
  return config;
}
