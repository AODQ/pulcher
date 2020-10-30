#include <pulcher-gfx/imgui.hpp>

#include <glm/glm.hpp>

bool pul::imgui::CheckImageClicked(glm::vec2 & pixelClicked) {
  if (!ImGui::IsItemClicked()) { return false; }

  // if image is clicked, approximate the clicked pixel, taking into account
  // image resolution differences when displaying to imgui
  auto const
      imItemMin  = ImGui::GetItemRectMin()
    , imItemMax  = ImGui::GetItemRectMax()
    , imMousePos = ImGui::GetMousePos()
  ;

  auto const
      itemMin  = glm::vec2(imItemMin.x, imItemMin.y)
    , itemMax  = glm::vec2(imItemMax.x, imItemMax.y)
    , mousePos =
        glm::clamp(glm::vec2(imMousePos.x, imMousePos.y), itemMin, itemMax)
  ;

  auto pixel = (mousePos - itemMin);

  // store results
  pixelClicked = pixel;

  return true;
}

struct InputTextCallbackUserData {
  std::string * str;
  /* ImGuiInputTextCallback chainCallback; */
  /* void & chainCallbackUserData; */
};

namespace {
int InputTextCallback(ImGuiInputTextCallbackData * data) {
  auto userdata =
    *reinterpret_cast<InputTextCallbackUserData *>(data->UserData);

  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
    // resize string
    std::string & str = *userdata.str;
    IM_ASSERT(data->Buf == str.c_str());
    str.resize(data->BufTextLen);
    data->Buf = const_cast<char *>(reinterpret_cast<char const *>(str.c_str()));
  }

  return 0;
}
}

bool pul::imgui::InputText(
  char const * label, std::string * str, ImGuiInputTextFlags flags
) {
  IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
  flags |= ImGuiInputTextFlags_CallbackResize;

  InputTextCallbackUserData userdata;
  userdata.str = str;
  return
    ImGui::InputText(
      label
    , const_cast<char *>(reinterpret_cast<char const *>(str->c_str()))
    , str->capacity() + 1, flags, ::InputTextCallback, &userdata
    );
}
