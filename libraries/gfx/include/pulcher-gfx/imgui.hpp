#pragma once

#ifdef PULCHER_GFX_IMGUI_H
#error \
  "multiple includes of pulcher-gfx/imgui.hpp; can only be included in source "
  "files"
#endif
#define PULCHER_GFX_IMGUI_H

// file can not be included in headers

#include <pulcher-util/log.hpp>

#include <glm/fwd.hpp>
#include <imgui/imgui.hpp>

#include <array>

namespace pul::imgui {
  template <typename ... T> void Text(char const * label, T && ... fmts) {
    ImGui::TextUnformatted(
      fmt::format(label, std::forward<T>(fmts)...).c_str()
    );
  }

  template <typename ... T>
  void ItemTooltip(char const * label, T && ... fmts) {
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip(
        "%s", fmt::format(label, std::forward<T>(fmts)...).c_str()
      );
    }
  }

  template <typename T>
  bool SliderInt(char const * label, T * value, int min, int max) {
    int valueAsInt = static_cast<int>(*value);
    bool result = ImGui::SliderInt(label, &valueAsInt, min, max);
    if (result) { *value = static_cast<T>(valueAsInt); }
    return result;
  }

  template <typename T>
  bool DragInt(char const * label, T * value, float accel = 1.0f) {
    int valueAsInt = static_cast<int>(*value);
    bool result = ImGui::DragInt(label, &valueAsInt, accel);
    if (result) { *value = static_cast<T>(valueAsInt); }
    return result;
  }

  template <typename T>
  bool DragInt2(char const * label, T * value, float accel = 1.0f) {
    std::array<int, 2> valueAsInt =
      { static_cast<int>(value[0]), static_cast<int>(value[1]) };

    bool result = ImGui::DragInt2(label, valueAsInt.data(), accel);
    if (result) {
      value[0] = static_cast<T>(valueAsInt[0]);
      value[1] = static_cast<T>(valueAsInt[1]);
    }
    return result;
  }

  template <typename T>
  bool InputInt(char const * label, T * value) {
    int valueAsInt = static_cast<int>(*value);
    bool result = ImGui::InputInt(label, &valueAsInt);
    if (result) { *value = static_cast<T>(valueAsInt); }
    return result;
  }

  template <typename T>
  bool InputInt2(char const * label, T * value) {
    std::array<int, 2> valueAsInt =
      { static_cast<int>(value[0]), static_cast<int>(value[1]) };

    bool result = ImGui::InputInt2(label, valueAsInt.data());

    if (result) {
      value[0] = static_cast<T>(valueAsInt[0]);
      value[1] = static_cast<T>(valueAsInt[1]);
    }

    return result;
  }

  bool CheckImageClicked(glm::vec2 & pixelClicked);

  bool InputText(
    char const * label, std::string * str, ImGuiInputTextFlags = 0
  );
}
