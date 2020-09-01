#pragma once

#pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wpedantic"
  #include <yojimbo/yojimbo.hpp>
#pragma GCC diagnostic pop

int GetNumBitsForMessage(uint16_t sequence);

namespace pulcher::network {

  enum struct Channel : uint32_t {
    Unreliable
  , Reliable
  , Size
  };

  enum struct MessageType : uint32_t {
    Test
  , Size
  };

  yojimbo::ClientServerConfig CreateClientServerConfig();

  struct TestMessage final : public yojimbo::Message {
    uint32_t sequence = 0u;

    template <typename Stream> bool Serialize(Stream & stream) {
      serialize_int(stream, sequence, 0, 512);

      return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
  };

  YOJIMBO_MESSAGE_FACTORY_START(TestMessageFactory, (size_t)MessageType::Size);
    YOJIMBO_DECLARE_MESSAGE_TYPE((size_t)MessageType::Test, TestMessage);
  YOJIMBO_MESSAGE_FACTORY_FINISH()

  struct Adapter final : public yojimbo::Adapter {
    inline yojimbo::MessageFactory * CreateMessageFactory(
      yojimbo::Allocator & allocator
    ) override {
      return YOJIMBO_NEW(allocator, TestMessageFactory, allocator);
    }
  };
}
