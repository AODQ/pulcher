#include <pulcher-util/any.hpp>

pulcher::util::Any::~Any() {
  if (Deallocate)
    { this->Deallocate(userdata); }
}

pulcher::util::Any::Any(Any && other) {
  this->userdata = other.userdata;
  other.Deallocate = nullptr;
  other.userdata = nullptr;
}
