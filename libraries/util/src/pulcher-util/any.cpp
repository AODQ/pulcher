#include <pulcher-util/any.hpp>

pul::util::Any::~Any() {
  if (Deallocate)
    { this->Deallocate(userdata); }
}

pul::util::Any::Any(Any && other) {
  this->userdata = other.userdata;
  other.Deallocate = nullptr;
  other.userdata = nullptr;
}
