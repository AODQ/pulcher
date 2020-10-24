#pragma once

// include this only in source files, use pimpl.hpp for declaration of pimpl
// must specialize with the type of pimpl using PIMPL_SPECIALIZE

#ifndef PIMPL_SPECIALIZE
#error please specialize pimpl with PIMPL_SPECIALIZE
#endif

#include <utility>

template <> pul::util::pimpl<PIMPL_SPECIALIZE>::pimpl()
  : m{std::make_unique<PIMPL_SPECIALIZE>()}
{}

template <> template <typename ... Args>
pul::util::pimpl<PIMPL_SPECIALIZE>::pimpl(Args && ... args)
  : m{std::make_unique<PIMPL_SPECIALIZE>(std::forward<Args>(args)...)}
{}

template <> pul::util::pimpl<PIMPL_SPECIALIZE>::~pimpl() {}

template <>
PIMPL_SPECIALIZE * pul::util::pimpl<PIMPL_SPECIALIZE>::operator->()
  { return this->m.get(); }

template <>
PIMPL_SPECIALIZE & pul::util::pimpl<PIMPL_SPECIALIZE>::operator*()
  { return *this->m.get(); }

template <>
PIMPL_SPECIALIZE const * pul::util::pimpl<PIMPL_SPECIALIZE>::operator->()
  const { return this->m.get(); }

template <>
PIMPL_SPECIALIZE const & pul::util::pimpl<PIMPL_SPECIALIZE>::operator*()
  const { return *this->m.get(); }

#undef PIMPL_SPECIALIZE
