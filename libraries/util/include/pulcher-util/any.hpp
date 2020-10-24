// a better RAII version of std::any

namespace pul::util {
  struct Any {
    Any() = default;
    ~Any();
    Any(Any const &) = delete;
    Any(Any &&);

    void (* Deallocate)(void * userdata) = nullptr;

    void * userdata = nullptr;
  };
}
