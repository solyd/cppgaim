#include <exception>
#include <iostream>
#include <memory>

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"

#include "spdlog/spdlog.h"

#include "utils/ScopeGuard.h"

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
  auto log = spdlog::stdout_color_mt("cppgaim");
  log->set_level(spdlog::level::debug);

  auto config = YAML::LoadFile("config/default.yaml");

  try {
    auto sdl_quit = sdl::init(SDL_INIT_VIDEO);
    auto ttf_quit = ttf::init();
    auto img_quit = img::init_png();

    auto screen_w = config["window"]["width"].as<int>();
    auto screen_h = config["window"]["height"].as<int>();

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

    int max_fps = 120;
    auto fps_textures = make_fps_textures(*renderer, *font, max_fps);

    Uint32 goal_fps = 60;
    Uint32 goal_ticks_per_loop = 1000 / goal_fps;

    Uint32 num_frames_since_measure = 0;
    Uint32 ticks_since_measure = 0;
    Uint32 measure_every_n_ticks = 1000;
    Uint32 num_frames = 0;
    Uint32 ticks_last_animation = 0;

    Uint32 frame_times[100];
    int frame_times_index = 0;
    while (true) {
      Uint32 loop_ticks_start = SDL_GetTicks();

      frame_times[frame_times_index] = loop_ticks_start;
      frame_times_index++;

      if (frame_times_index >= 100) {
        frame_times_index = 0;

        Uint32 frame_time_diffs[99];
        double avg_fps = 0.0;
        for (int i = 0; i < 99; ++i) {
          frame_time_diffs[i] = frame_times[i+1] - frame_times[i];
          avg_fps += frame_time_diffs[i];
        }

        avg_fps /= 99.0f;
        log->debug("avg time per frame over last 100 frames: {}", avg_fps);
      }

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

              if (loop_ticks_start - ticks_last_animation > 150) {
                curr_sprite_index = static_cast<Uint32>((curr_sprite_index + 1) % active_animation->size());
                ticks_last_animation = loop_ticks_start;
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

      Uint32 loop_ticks_end = SDL_GetTicks();
      Uint32 loop_ticks = loop_ticks_end - loop_ticks_start;
      log->trace("loop iteration took {} ms", loop_ticks);



//      if (loop_ticks < goal_ticks_per_loop) {
//        Uint32 sleep_ticks = goal_ticks_per_loop - loop_ticks;
//        log->trace("sleeping for {} ms", sleep_ticks);
//        SDL_Delay(sleep_ticks);
//      }

      // update loop_ticks
      loop_ticks = SDL_GetTicks() - loop_ticks_start;

      // measure fps every n ticks
      num_frames++;
      num_frames_since_measure++;
      ticks_since_measure += loop_ticks;
      Uint32 curr_fps = num_frames_since_measure * 1000 / (ticks_since_measure + 1);
      if (ticks_since_measure >= measure_every_n_ticks) {
        log->debug("fps: {}", curr_fps);
        ticks_since_measure = 0;
        num_frames_since_measure = 0;
      }

      // render shit on screen
      SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, SDL_ALPHA_OPAQUE);
      SDL_RenderClear(renderer.get());

      // update fps txt on screen
      if (curr_fps < max_fps) {
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
