#pragma once

#include <memory>

// adapted from herbsutter.com/gotw/_101

// include this in the header where a pointer implementation is used, then
//   include pimpl_impl.hpp in the corresponding source implementation

namespace pul::util {
  template <typename T>
  class pimpl {
  public:
    pimpl();
    template <typename ... Args> pimpl(Args && ...);
    ~pimpl();
    T * operator->();
    T & operator*();
    T const * operator->() const;
    T const & operator*() const;
  private:
    std::unique_ptr<T> m;
  };
}
