#pragma once

#ifdef PULCHER_GFX_IMGUI_H
#error \
  "multiple includes of pulcher-gfx/imgui.hpp; can only be included in source "
  "files"
#endif
#define PULCHER_GFX_IMGUI_H

// file can not be included in headers

#include <imgui/imgui.hpp>
#include <pulcher-core/log.hpp>

namespace pul::imgui {
  template <typename... T> void Text(char const * label, T && ... fmts) {
    ImGui::TextUnformatted(
      fmt::format(label, std::forward<T>(fmts)...).c_str()
    );
  }
}
