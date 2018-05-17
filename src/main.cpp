#include <exception>
#include <iostream>
#include <memory>
#include <algorithm>

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"

#include "spdlog/spdlog.h"

#include "utils/ScopeGuard.h"
#include "utils/FpsCounter.h"

#include "sdl/Error.h"
#include "sdl/helpers.h"

#include "yaml-cpp/yaml.h"

#include "fmt/format.h"

std::vector<std::tuple<sdl::unique_texture, int, int>>
make_fps_textures(SDL_Renderer& renderer, TTF_Font& font, int max_fps) {
  std::vector<std::tuple<sdl::unique_texture, int, int>> res;
  res.reserve(max_fps);

  for (int i = 0; i < max_fps; ++i) {
    res.emplace_back(
      ttf::make_texture(
        renderer,
        font,
        fmt::format("fps: {}", i),
        {0, 0, 0, 0}));
  }

  return res;
};

int main(int argc, char* argv[]) {
  using namespace cppgaim;

  auto log = spdlog::stdout_color_mt("cppgaim");
  log->set_level(spdlog::level::debug);

  auto config = YAML::LoadFile("config/default.yaml");

  try {
    auto sdl_quit = sdl::init(SDL_INIT_VIDEO);
    auto ttf_quit = ttf::init();
    auto img_quit = img::init_png();

    auto screen_w = config["window"]["width"].as<int>();
    auto screen_h = config["window"]["height"].as<int>();

    FpsCounter fps(10);

    auto window = sdl::make_window(
      "cppgaim",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      screen_w,
      screen_h,
      SDL_WINDOW_SHOWN
    );

    auto renderer = sdl::make_renderer(window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    auto font = ttf::open_font("resources/fonts/SourceCodePro-Regular.ttf", 13);

    //   _____            _ _
    //  / ____|          (_) |
    // | (___  _ __  _ __ _| |_ ___  ___
    //  \___ \| '_ \| '__| | __/ _ \/ __|
    //  ____) | |_) | |  | | ||  __/\__ \
    // |_____/| .__/|_|  |_|\__\___||___/
    //        | |
    //        |_|
    // ================================================================================
    auto sprite_sheet = img::load("resources/player.png");
    sdl::set_color_key(*sprite_sheet, sdl::get_pixel(*sprite_sheet, 0, 0));
    auto sprite_sheet_texture = sdl::make_texture_from(*renderer, *sprite_sheet);
    int sprite_size = 32;

    std::vector<SDL_Rect> up;
    up.push_back({sprite_size * 0, 0, sprite_size, sprite_size});
    up.push_back({sprite_size * 1, 0, sprite_size, sprite_size});

    std::vector<SDL_Rect> right;
    right.push_back({sprite_size * 2, 0, sprite_size, sprite_size});
    right.push_back({sprite_size * 3, 0, sprite_size, sprite_size});

    std::vector<SDL_Rect> down;
    down.push_back({sprite_size * 4, 0, sprite_size, sprite_size});
    down.push_back({sprite_size * 5, 0, sprite_size, sprite_size});

    std::vector<SDL_Rect> left;
    left.push_back({sprite_size * 6, 0, sprite_size, sprite_size});
    left.push_back({sprite_size * 7, 0, sprite_size, sprite_size});

    SDL_Rect& default_sprite = down[0];

    std::vector<SDL_Rect>* active_animation = &down;
    Uint32 curr_sprite_index = 0;

    SDL_Point curr_pos {50, 50};
    // ================================================================================

    const auto fps_cap = config["fps_cap"].as<double>();
    auto fps_textures = make_fps_textures(*renderer, *font, static_cast<int>(fps_cap));

    while (true) {
      // TODO(solyd): use SDL_GetPerformanceCounter andSDL_GetPerformanceFrequency
      auto frame_start_time = SDL_GetTicks();
      auto last_frame_duration = frame_start_time - fps.last_frame_start_time();

      auto sleep_delta = std::max(
        0,
        static_cast<int>(FpsCounter::ms_per_frame_for(fps_cap)) -
        static_cast<int>(last_frame_duration)
      );

      if (sleep_delta > 0) {
        log->debug(">>>>>>>>>>>>>>>>>>>>>>> SLEEEEEPP, for {}", sleep_delta);
        SDL_Delay(static_cast<Uint32>(sleep_delta));
      }

      // until now the logic belonged to "previous" frame
      frame_start_time = SDL_GetTicks();
      fps.mark_frame_start(frame_start_time);

      SDL_Event e;
      while (SDL_PollEvent(&e) != 0) {
        switch (e.type) {
          case SDL_KEYDOWN: {
            //if (e.key.repeat == 0) {
              switch (e.key.keysym.scancode) {
                case SDL_SCANCODE_DOWN:
                  active_animation = &down;
                  curr_pos.y += 5;
                  break;
                case SDL_SCANCODE_UP:
                  active_animation = &up;
                  curr_pos.y -= 5;
                  break;
                case SDL_SCANCODE_RIGHT:
                  active_animation = &right;
                  curr_pos.x += 5;
                  break;
                case SDL_SCANCODE_LEFT:
                  active_animation = &left;
                  curr_pos.x -= 5;
                  break;
                case SDL_SCANCODE_SPACE:
                  return EXIT_SUCCESS;
                  break;
                default:
                  break;
              }

              if (curr_pos.x >= screen_w) {
                curr_pos.x = screen_w - sprite_size;
              } else if (curr_pos.x < 0) {
                curr_pos.x = 0;
              }

              if (curr_pos.y >= screen_h) {
                curr_pos.y = screen_h - sprite_size;
              } else if (curr_pos.y < 0) {
                curr_pos.y = 0;
              }

          //  }
          }
            break;

          case SDL_QUIT:
            log->debug("SQL_QUIT");
            return EXIT_SUCCESS;
        }
      }

      // render shit on screen
      SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, SDL_ALPHA_OPAQUE);
      SDL_RenderClear(renderer.get());

      auto curr_fps = static_cast<Uint32>(fps.avg());
      // update fps txt on screen
      if (curr_fps < fps_cap) {
        auto&[fps_texture, w, h] = fps_textures[curr_fps];
        SDL_Rect fps_txt_dst_rect {0, 0, w, h};
        SDL_RenderCopy(renderer.get(), fps_texture.get(), nullptr, &fps_txt_dst_rect);
      }

      // draw the sprite
      SDL_Rect player_dst_rect {SDL_UNPACK_POINT(curr_pos), sprite_size*2, sprite_size*2};
      SDL_RenderCopy(renderer.get(), sprite_sheet_texture.get(), &((*active_animation)[curr_sprite_index]), &player_dst_rect);

      SDL_RenderPresent(renderer.get());
    }
  } catch (const sdl::Error& ex) {
    log->critical("Caught SDL Error: {}", ex.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
