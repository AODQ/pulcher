#include <plugin-entity/cursor.hpp>

#include <pulcher-controls/controls.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/sokol.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/log.hpp>

#include <entt/entt.hpp>

namespace {

struct ComponentCursor {
  sg_pipeline sgPipeline;
  sg_shader sgProgram;
  sg_bindings sgBindings;
  std::unique_ptr<pul::gfx::SgBuffer> sgBufferOrigin = {};
  std::unique_ptr<pul::gfx::SgBuffer> sgBufferUvCoord = {};
};

void ConstructComponentCursor(ComponentCursor & self) {

  self.sgBindings = {};

  { // shader
    sg_shader_desc desc = {};
    desc.vs.uniform_blocks[0].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[0].uniforms[0].name = "originOffset";
    desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.uniform_blocks[1].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[1].uniforms[0].name = "framebufferResolution";
    desc.vs.uniform_blocks[1].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.uniform_blocks[2].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[2].uniforms[0].name = "cameraOrigin";
    desc.vs.uniform_blocks[2].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.uniform_blocks[3].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[3].uniforms[0].name = "cursorOrigin";
    desc.vs.uniform_blocks[3].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.source = PUL_SHADER(
      out vec2 uvCoord;
      out float playerCursorLength;
      flat out int primitiveId;

      uniform vec2 originOffset;
      uniform vec2 framebufferResolution;
      uniform vec2 cameraOrigin;
      uniform vec2 cursorOrigin;

      const vec2 vertexArray[6] = vec2[](
        vec2(0.0f,  0.0f)
      , vec2(1.0f,  1.0f)
      , vec2(1.0f,  0.0f)

      , vec2(0.0f,  0.0f)
      , vec2(0.0f,  1.0f)
      , vec2(1.0f,  1.0f)
      );

      // hg sdf mercury
      void pR(inout vec2 p, float a) {
        p = cos(a) * p + sin(a)*vec2(p.y, -p.x);
      }

      void main() {
        vec2 origin;

        origin = vertexArray[gl_VertexID%6];

        //                    ___________
        //                  /      |     \
        // compose: ------ (  -----+----- )
        //                  \      |     /
        //                    ----------

        // line to crosshair: 0..6
        // crosshair circle: 6..12
        // crosshair horizontal line 12..18
        // crosshair vertical line 18..24

        { // uv coord x (length)
          vec2 o = origin;

          // solid end-point shading for the rest
          if (gl_VertexID >= 6)
            o = vec2(1.0f);

          o.x *= length(cursorOrigin - originOffset) - 16.0f;
          playerCursorLength = clamp(o.x / 400.0f, 0.0f, 1.0f);

          if (gl_VertexID >= 6)
            playerCursorLength *= 2.5f;
        }

        if (gl_VertexID < 6) {
          vec2 direction = normalize(cursorOrigin - originOffset);
          origin.x *= length(cursorOrigin - originOffset) - 16.0f;
          pR(origin, atan(-direction.y, direction.x));
        } else if (gl_VertexID < 12) {
          uvCoord = vec2(origin.x, 1.0f-origin.y);
          origin -= vec2(0.5f);
          origin = origin*28.0f + (cursorOrigin - originOffset);
        } else if (gl_VertexID < 18) {
          vec2 direction = normalize(cursorOrigin - originOffset);
          origin.x = (origin.x - 0.5f);
          origin.x *= 16.0f;
          pR(origin, atan(-direction.y, direction.x));
          origin += (cursorOrigin - originOffset);
        } else if (gl_VertexID < 24) {
          vec2 direction = normalize(cursorOrigin - originOffset);
          origin.x = (origin.x - 0.5f);
          origin.x *= 16.0f;
          pR(origin, atan(-direction.y, direction.x) + 3.141f/2.0f);
          origin += (cursorOrigin - originOffset);
        }

        vec2 framebufferScale = vec2(2.0f) / framebufferResolution;
        vec2 vertexOrigin = (origin.xy)*vec2(1,-1) * framebufferScale;
        vertexOrigin +=
          (originOffset-cameraOrigin)*vec2(1, -1) * framebufferScale
        ;
        gl_Position = vec4(vertexOrigin, 0.3f, 1.0f);
        primitiveId = int(gl_VertexID);
      }
    );

    desc.fs.source = PUL_SHADER(
      in vec2 uvCoord;
      in float playerCursorLength;
      flat in int primitiveId;

      out vec4 outColor;

      void main() {
        outColor = vec4(1.0f, 1.0f, 1.0f, 1.0f-exp(-playerCursorLength*2.5f));
        if (primitiveId >= 6 && primitiveId < 12) {
          if (length(uvCoord - vec2(0.5f)) > 0.45f) { discard; }
          if (length(uvCoord - vec2(0.5f)) < 0.40f) { discard; }
        }
      }
    );

    self.sgProgram = sg_make_shader(&desc);
  }

  { // pipeline
    sg_pipeline_desc desc = {};

    desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    desc.index_type = SG_INDEXTYPE_NONE;

    desc.shader = self.sgProgram;
    desc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth_stencil.depth_write_enabled = false;

    desc.blend.enabled = true;
    desc.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    desc.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    desc.blend.op_rgb = SG_BLENDOP_ADD;
    desc.blend.src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA;
    desc.blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    desc.blend.op_alpha = SG_BLENDOP_ADD;

    desc.rasterizer.cull_mode = SG_CULLMODE_NONE;
    desc.rasterizer.alpha_to_coverage_enabled = false;
    desc.rasterizer.face_winding = SG_FACEWINDING_CCW;
    desc.rasterizer.sample_count = 1;

    desc.label = "cursor pipeline";

    self.sgPipeline = sg_make_pipeline(desc);
  }
}

}

void plugin::entity::ConstructCursor(
  pul::plugin::Info const &, pul::core::SceneBundle & scene
) {
  auto & registry = scene.EnttRegistry();

  auto cursorEntity = registry.create();
  ::ComponentCursor componentCursor;
  ConstructComponentCursor(componentCursor);
  registry.emplace<::ComponentCursor>(cursorEntity, std::move(componentCursor));
}

void plugin::entity::RenderCursor(
  pul::plugin::Info const &, pul::core::SceneBundle & scene
) {
  auto & registry = scene.EnttRegistry();
  auto view = registry.view<::ComponentCursor>();

  for (auto entity : view) {
    auto & cursor = registry.get<::ComponentCursor>(entity);

    sg_apply_pipeline(cursor.sgPipeline);

    sg_apply_bindings(cursor.sgBindings);

    auto playerOrigin = glm::vec2(scene.playerOrigin);

    sg_apply_uniforms(
      SG_SHADERSTAGE_VS
    , 0
    , &playerOrigin
    , sizeof(float) * 2ul
    );

    sg_apply_uniforms(
      SG_SHADERSTAGE_VS
    , 1
    , &scene.config.framebufferDimFloat.x
    , sizeof(float) * 2ul
    );

    auto cameraOrigin = glm::vec2(scene.cameraOrigin);

    sg_apply_uniforms(
      SG_SHADERSTAGE_VS
    , 2
    , &cameraOrigin.x
    , sizeof(float) * 2ul
    );

    auto mouseOrigin =
      playerOrigin + scene.PlayerController().current.lookOffset;

    sg_apply_uniforms(
      SG_SHADERSTAGE_VS
    , 3
    , &mouseOrigin.x
    , sizeof(float) * 2ul
    );

    sg_draw(0, 24, 1);
  }
}
