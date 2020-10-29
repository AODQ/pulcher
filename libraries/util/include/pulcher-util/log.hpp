#pragma once

#pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wshadow"
  #include <spdlog/spdlog.h>
#pragma GCC diagnostic pop

namespace {
  auto StandardFormat(std::string & p, fmt::format_parse_context & ctx) {
    auto it = std::find(ctx.begin(), ctx.end(), '}');
    p = "{:" + std::string{ctx.begin(), it} + "}";
    return it;
  }
}

#define PUL_ASSERT(X, ...) \
  if (!(X)) { \
    spdlog::critical( \
      "Assertion fail; {}@{}: '{}'" \
    , __FILE__, __LINE__ \
    , #X \
    ); \
    __VA_ARGS__ \
  }

#define PUL_ASSERT_CMP(X, CMP, Y, ...) \
  if (!((X) CMP (Y))) { \
    spdlog::critical( \
      "Assertion fail; {}@{}: '{}' ({}) {} '{}' ({})" \
    , __FILE__, __LINE__ \
    , #X , (X) , #CMP , #Y , (Y) \
    ); \
    __VA_ARGS__ \
  }

////////////////////////////////////////////////////////////////////////////////
template<> struct fmt::formatter<glm::vec2> {
  std::string p = "{}";
  auto parse(format_parse_context & ctx) { return ::StandardFormat(p, ctx); }

  template <typename FmtCtx> auto format(glm::vec2 const & vec, FmtCtx & ctx) {
    return
      format_to(
        ctx.out()
      , "(" + p + ", " + p + ")"
      , vec.x, vec.y
      );
  }
};

////////////////////////////////////////////////////////////////////////////////
template<> struct fmt::formatter<glm::u16vec2> {
  std::string p = "{}";
  auto parse(format_parse_context & ctx) { return ::StandardFormat(p, ctx); }

  template <typename FmtCtx> auto format(glm::u16vec2 const & vec, FmtCtx & ctx)
  {
    return
      format_to(
        ctx.out()
      , "(" + p + ", " + p + ")"
      , vec.x, vec.y
      );
  }
};

////////////////////////////////////////////////////////////////////////////////
template<> struct fmt::formatter<glm::ivec2> {
  std::string p = "{}";
  auto parse(format_parse_context & ctx) { return ::StandardFormat(p, ctx); }

  template <typename FmtCtx> auto format(glm::ivec2 const & vec, FmtCtx & ctx)
  {
    return
      format_to(
        ctx.out()
      , "(" + p + ", " + p + ")"
      , vec.x, vec.y
      );
  }
};

////////////////////////////////////////////////////////////////////////////////
template<> struct fmt::formatter<glm::uvec2> {
  std::string p = "{}";
  auto parse(format_parse_context & ctx) { return ::StandardFormat(p, ctx); }

  template <typename FmtCtx> auto format(glm::uvec2 const & vec, FmtCtx & ctx)
  {
    return
      format_to(
        ctx.out()
      , "(" + p + ", " + p + ")"
      , vec.x, vec.y
      );
  }
};

////////////////////////////////////////////////////////////////////////////////
template<> struct fmt::formatter<glm::vec3> {
  std::string p = "{}";
  auto parse(format_parse_context & ctx) { return ::StandardFormat(p, ctx); }

  template <typename FmtCtx> auto format(glm::vec3 const & vec, FmtCtx & ctx) {
    return
      format_to(
        ctx.out()
      , "(" + p + ", " + p + ", " + p + ")"
      , vec.x, vec.y, vec.z
      );
  }
};


////////////////////////////////////////////////////////////////////////////////
template<> struct fmt::formatter<glm::vec4> {
  std::string p = "{}";
  auto parse(format_parse_context & ctx) { return ::StandardFormat(p, ctx); }

  template <typename FmtCtx> auto format(glm::vec4 const & vec, FmtCtx & ctx) {
    return
      format_to(
        ctx.out()
      , "(" + p + ", " + p + ", " + p + ", " + p + ")"
      , vec.x, vec.y, vec.z, vec.w
      );
  }
};

////////////////////////////////////////////////////////////////////////////////
template<> struct fmt::formatter<glm::mat3> {
  std::string p = "{}";
  auto parse(format_parse_context & ctx) { return ::StandardFormat(p, ctx); }

  template <typename FmtCtx> auto format(glm::mat3 const & m, FmtCtx & ctx)
  {
    return
      format_to(
        ctx.out()
      , "[{}, {}, {}, {}, {}, {}, {}, {}, {}]"
      , m[0][0], m[0][1], m[0][2]
      , m[1][0], m[1][1], m[1][2]
      , m[2][0], m[2][1], m[2][2]
      );
  }
};
