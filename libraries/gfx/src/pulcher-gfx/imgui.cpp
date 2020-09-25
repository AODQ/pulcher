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
