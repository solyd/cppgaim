#pragma once

#include <memory>

#include "SDL_video.h"
#include "SDL_ttf.h"

#include <fmt/format.h>

#include "sdl/Error.h"
#include "utils/custom_unique_ptr.h"

namespace sdl {

#define SDL_UNPACK_COLOR(c) (c).r, (c).g, (c).b, (c).a
#define SDL_UNPACK_POINT(p) (p).x, (p).y

#define SDL_DECL_CREATE_FULL(name, lower_name, unique_type, sdl_type, sdl_create, sdl_destroy) \
  using unique_type = utils::custom_unique_ptr<sdl_type, sdl_destroy>; \
  template<typename... Args> \
  unique_type make_##lower_name(Args&& ... args) { \
    sdl_type * res = sdl_create(args...); \
    if (res == nullptr) { \
      throw sdl::Error(fmt::format("Failed to create " #name ": {}", SDL_GetError())); \
    } \
    return unique_type(res); \
  }

#define SDL_DECL_CREATE(resource_name, unique_name) \
  SDL_DECL_CREATE_FULL( \
    resource_name, \
    unique_name, \
    unique_##unique_name, \
    SDL_##resource_name, \
    SDL_Create##resource_name, \
    SDL_Destroy##resource_name \
  )

SDL_DECL_CREATE(Window, window);
SDL_DECL_CREATE(Renderer, renderer);
SDL_DECL_CREATE(Texture, texture);

using unique_surface = utils::custom_unique_ptr<SDL_Surface, SDL_FreeSurface>;

unique_texture make_texture_from(SDL_Renderer& renderer, SDL_Surface& surface) {
  SDL_Texture* res = SDL_CreateTextureFromSurface(&renderer, &surface);

  if (res == nullptr) {
    throw sdl::Error(fmt::format("Failed to create texture from surface: {}", SDL_GetError()));
  }

  return unique_texture(res);
}

template<typename... Args>
decltype(auto) init(Args&& ... args) {
  SDL_Init(args...);
  return utils::make_guard([]() { SDL_Quit(); });
}

void set_color_key(SDL_Surface& surface, SDL_Color color) {
  if (SDL_SetColorKey(&surface, 1, SDL_MapRGBA(surface.format, SDL_UNPACK_COLOR(color))) != 0) {
    throw sdl::Error(fmt::format("Failed to set color key: {}", SDL_GetError()));
  }
}

SDL_Color get_pixel(SDL_Surface& surface, int x, int y) {
  if (x >= surface.w || y >= surface.h) {
    throw sdl::Error(fmt::format("Pixel is out of bounds: x={}, y={} vs. surface.w={}, surface.h={}",
                                 x, y, surface.w, surface.h));
  }

  auto pixel = static_cast<Uint32*>(surface.pixels)[(y * surface.w) + x];

  SDL_Color res;
  SDL_GetRGBA(pixel, surface.format, &res.r, &res.g, &res.b, &res.a);

  return res;
}

}

namespace ttf {

template<typename... Args>
decltype(auto) init(Args&& ...) {
  int res = TTF_Init();

  if (res != 0) {
    throw sdl::Error(fmt::format("Failed to init TTF: {}", TTF_GetError()));
  }

  return utils::make_guard([]() { TTF_Quit(); });
}

using unique_font = utils::custom_unique_ptr<TTF_Font, TTF_CloseFont>;

template<typename... Args>
unique_font open_font(Args&& ... args) {
  TTF_Font* res = TTF_OpenFont(args...);

  if (res == nullptr) {
    throw sdl::Error(fmt::format("Failed to open font: {}", TTF_GetError()));
  }

  return unique_font(res);
}

sdl::unique_surface render_text_solid(TTF_Font& font, const char* txt, SDL_Color color) {
  SDL_Surface* res = TTF_RenderText_Solid(&font, txt, color);

  if (res == nullptr) {
    throw sdl::Error(fmt::format("Failed to render text solid: {}", TTF_GetError()));
  }

  return sdl::unique_surface(res);
}

std::tuple<sdl::unique_texture, int, int>
make_texture(SDL_Renderer& renderer,
             TTF_Font& font,
             const std::string& txt,
             SDL_Color color)
{
  auto surface = render_text_solid(font, txt.c_str(), color);
  return std::make_tuple(
    sdl::make_texture_from(renderer, *surface),
    surface->w,
    surface->h
  );
}

}

namespace img {

template<typename... Args>
decltype(auto) init_png(Args&& ...) {
  int res = IMG_Init(IMG_INIT_PNG);

  if (res == 0) {
    throw sdl::Error(fmt::format("Failed to init IMG png: {}", IMG_GetError()));
  }

  return utils::make_guard([]() { IMG_Quit(); });
}

sdl::unique_surface load(const std::string& path) {
  SDL_Surface* res = IMG_Load(path.c_str());

  if (res == nullptr) {
    throw sdl::Error(fmt::format("Failed to open img: {}", IMG_GetError()));
  }

  return sdl::unique_surface(res);
}

}
