// a better RAII version of std::any

namespace pulcher::util {
  struct Any {
    Any() = default;
    ~Any();
    Any(Any const &) = delete;
    Any(Any &&);

    void (* Deallocate)(void * userdata) = nullptr;

    void * userdata = nullptr;
  };
}
