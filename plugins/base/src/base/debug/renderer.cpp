#include <plugin-base/debug/renderer.hpp>

#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/sokol.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-gfx/imgui.hpp>

#include <glad/glad.hpp>

namespace {
  // sokol information related to debug rendering
  struct DebugRenderInfo {
    sg_bindings bindings = {};
    sg_pipeline pipeline = {};
    sg_shader program = {};
  };

  // <origin vec4, color vec4> points , lines .. , circle ..
  pul::gfx::SgBuffer debugBuffer;

  DebugRenderInfo
    debugRenderPoint, debugRenderLine, debugRenderCircle
  ;

  constexpr size_t maxPrimitives = 500;
  std::vector<glm::vec4> debugUploadBuffer = {};
  // lines -> point -> circle
  constexpr size_t debugRenderLineBegin = 0;
  constexpr size_t debugBufferByteSize =
      ::maxPrimitives * sizeof(float) * 8 // points
    + ::maxPrimitives * sizeof(float) * 8 * 2 // lines
    + ::maxPrimitives * sizeof(float) * 8 * 4 // circle
  ;

  size_t debugRenderLineLength = 0ul;

  // this specifies the number of draw-calls, it's different from the length
  // as that length will be cleared out after a swap, however we will still be
  // required to render for interpolated frames
  size_t debugRenderLineDrawCalls = 0;
}

void plugin::debug::ShapesRenderInitialize() {

  { // buffer
    sg_buffer_desc desc = {};
    desc.size = ::debugBufferByteSize;
    desc.usage = SG_USAGE_STREAM;
    desc.content = nullptr;
    desc.label = "debug-primitive-render-buffer";
    ::debugBuffer.buffer = sg_make_buffer(desc);

    // resize upload buffer
    ::debugUploadBuffer.resize(desc.size / (sizeof(float) * 4));
  }

  // -- bindings
  ::debugRenderLine.bindings.vertex_buffers[0] = ::debugBuffer.buffer;
  ::debugRenderLine.bindings.vertex_buffer_offsets[0] = 0;

  { // -- line/point shader
    sg_shader_desc desc = {};
    desc.vs.uniform_blocks[0].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[0].uniforms[0].name = "originOffset";
    desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.uniform_blocks[1].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[1].uniforms[0].name = "framebufferResolution";
    desc.vs.uniform_blocks[1].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.source = PUL_SHADER(
      layout(location = 0) in vec2 inOrigin;
      layout(location = 1) in vec3 inColor;

      uniform vec2 originOffset;
      uniform vec2 framebufferResolution;

      flat out vec3 inoutColor;

      void main() {
        vec2 framebufferScale = vec2(2.0f) / framebufferResolution;
        vec2 vertexOrigin = (inOrigin)*vec2(1,-1) * framebufferScale;
        vertexOrigin += originOffset*vec2(-1, 1) * framebufferScale;
        gl_Position = vec4(vertexOrigin.xy, 0.5f, 1.0f);

        inoutColor = inColor;
      }
    );

    desc.fs.source = PUL_SHADER(
      flat in vec3 inoutColor;

      out vec4 outColor;

      void main() {
        outColor = vec4(inoutColor, 1.0f);
      }
    );

    auto program = sg_make_shader(&desc);

    ::debugRenderPoint.program = program;
    ::debugRenderLine.program = program;
  }

  { // -- line pipeline
    sg_pipeline_desc desc = {};

    desc.layout.buffers[0].stride = sizeof(float) * 8;
    desc.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
    desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT4;
    desc.layout.attrs[0].buffer_index = 0;
    desc.layout.attrs[0].offset = 0;

    desc.layout.buffers[1].stride = sizeof(float) * 8;
    desc.layout.buffers[1].step_func = SG_VERTEXSTEP_PER_VERTEX;
    desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
    desc.layout.attrs[1].buffer_index = 0;
    desc.layout.attrs[1].offset = sizeof(float) * 4;

    desc.primitive_type = SG_PRIMITIVETYPE_LINES;
    desc.index_type = SG_INDEXTYPE_NONE;

    desc.shader = ::debugRenderLine.program;
    desc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth_stencil.depth_write_enabled = true;

    desc.blend.enabled = false;

    desc.rasterizer.alpha_to_coverage_enabled = false;
    desc.rasterizer.face_winding = SG_FACEWINDING_CCW;
    desc.rasterizer.sample_count = 1;

    desc.label = "debug-render-ray-pipeline";

    ::debugRenderLine.pipeline = sg_make_pipeline(&desc);
  }

  (void)debugRenderCircle;
}

void plugin::debug::ShapesRenderShutdown() {
  sg_destroy_pipeline(::debugRenderPoint.pipeline);
  sg_destroy_shader(::debugRenderPoint.program);
  sg_destroy_pipeline(::debugRenderLine.pipeline);
  sg_destroy_shader(::debugRenderLine.program);
  sg_destroy_pipeline(::debugRenderCircle.pipeline);
  sg_destroy_shader(::debugRenderCircle.program);

  ::debugBuffer.Destroy();

  debugRenderLineLength = -1ul;
}

void plugin::debug::RenderLine(
  glm::vec2 start, glm::vec2 end, glm::vec3 color
) {

  if (::debugRenderLineLength >= ::maxPrimitives) { return; }

  auto offset = ::debugRenderLineBegin + ::debugRenderLineLength*4;

  ::debugUploadBuffer[offset + 0] = glm::vec4(start, 0.0f, 1.0f);
  ::debugUploadBuffer[offset + 1] = glm::vec4(color, 1.0f);
  ::debugUploadBuffer[offset + 2] = glm::vec4(end, 0.0f, 1.0f);
  ::debugUploadBuffer[offset + 3] = glm::vec4(color, 1.0f);

  ++ ::debugRenderLineLength;
}

void plugin::debug::RenderAabbByCenter(
  glm::vec2 origin, glm::vec2 dim, glm::vec3 color
) {
  // render box with lines

  plugin::debug::RenderLine(
    origin + glm::vec2(-dim.x, -dim.y)
  , origin + glm::vec2(+dim.x, -dim.y)
  , color
  );

  plugin::debug::RenderLine(
    origin + glm::vec2(-dim.x, +dim.y)
  , origin + glm::vec2(+dim.x, +dim.y)
  , color
  );

  plugin::debug::RenderLine(
    origin + glm::vec2(+dim.x, -dim.y)
  , origin + glm::vec2(+dim.x, +dim.y)
  , color
  );

  plugin::debug::RenderLine(
    origin + glm::vec2(-dim.x, -dim.y)
  , origin + glm::vec2(-dim.x, +dim.y)
  , color
  );
}

// sorry future self for how this circle is rendered, but your previous self
// left the function decl in and didn't even implement this
void plugin::debug::RenderCircle(
  glm::vec2 const origin, float const radius, glm::vec3 const color
) {
  // render circle with lines
  plugin::debug::RenderLine(
    origin + glm::vec2(-radius, 0.0)
  , origin + glm::vec2(0.0, +radius)
  , color
  );

  plugin::debug::RenderLine(
    origin + glm::vec2(0.0, +radius)
  , origin + glm::vec2(+radius, 0.0)
  , color
  );

  plugin::debug::RenderLine(
    origin + glm::vec2(+radius, 0.0)
  , origin + glm::vec2(0.0, -radius)
  , color
  );

  plugin::debug::RenderLine(
    origin + glm::vec2(0.0, -radius)
  , origin + glm::vec2(-radius, 0.0)
  , color
  );
}

void plugin::debug::ShapesRender(
  pul::core::SceneBundle const & scene
, pul::core::RenderBundleInstance const &
) {

  // check if buffer needs to be updated
  if (::debugRenderLineLength != 0ul) {
    plugin::debug::ShapesRenderSwap();
  }

  if (::debugRenderLineDrawCalls == 0ul) { return; }

  glm::vec2 const cameraOrigin = scene.cameraOrigin;

  // -- lines
  sg_apply_pipeline(::debugRenderLine.pipeline);
  sg_apply_bindings(::debugRenderLine.bindings);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &cameraOrigin.x, sizeof(float) * 2ul);
  sg_apply_uniforms(
    SG_SHADERSTAGE_VS, 1,
    &scene.config.framebufferDimFloat.x, sizeof(float) * 2ul
  );
  glLineWidth(1.0f);
  sg_draw(0, ::debugRenderLineDrawCalls, 1);
}

void plugin::debug::ShapesRenderSwap() {

  // upload data
  sg_update_buffer(
    ::debugBuffer.buffer
  , ::debugUploadBuffer.data()
  , ::debugBufferByteSize
  );

  // update draw calls
  ::debugRenderLineDrawCalls = ::debugRenderLineLength * 2;

  // reset buffers
  ::debugRenderLineLength = 0ul;
}
