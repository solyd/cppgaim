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
        {255, 255, 255, 0}));
  }

  return res;
};

class Cell {
public:
  Cell() = default;

  Cell(SDL_Rect rect_, SDL_Color color_, bool is_filled_)
    : rect_(rect_), color_(color_), is_filled_(is_filled_) {}

  void render(SDL_Renderer& r) {
    SDL_SetRenderDrawColor(&r, SDL_UNPACK_COLOR(color_));
    (is_filled_ ? SDL_RenderFillRect : SDL_RenderDrawRect)(&r, &rect_);
  }

  void set(SDL_Rect rect) { rect_ = rect; }
  void set(SDL_Color color) { color_ = color; }
  void set(bool is_filled) { is_filled_ = is_filled; }

private:
  SDL_Rect rect_;
  SDL_Color color_;
  bool is_filled_;
};

class Grid {
private:
  using CellsArray = std::vector<std::vector<Cell>>;

public:
  Grid(CellsArray::size_type num_cells_w, CellsArray::size_type num_cells_h)
    : cells_(num_cells_w, std::vector<Cell>(num_cells_h)) {}

  void set_screen_size(int w, int h) {
    int pixels_per_cell_w = static_cast<int>(w / num_cells_w());
    int pixels_per_cell_h = static_cast<int>(h / num_cells_h());

    int leftover_w = static_cast<int>(w - (pixels_per_cell_w * num_cells_w()));
//    int leftover_h =

    for (auto& column : cells_) {
      for (auto& cell : column) {
      }
    }
  }

private:
  CellsArray::size_type num_cells_w() { return cells_.size(); }
  CellsArray::size_type num_cells_h() { return cells_[0].size(); }

private:
  CellsArray cells_;
};

int main(int argc, char* argv[]) {
  auto log = spdlog::stdout_color_mt("cppgaim");
  log->set_level(spdlog::level::debug);

  auto config = YAML::LoadFile("config/default.yaml");

  try {
    auto sdl_quit = sdl::init(SDL_INIT_VIDEO);
    auto ttf_quit = sdl::init_ttf();

    auto window = sdl::make_window(
      "SDL2Test",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      config["window"]["width"].as<int>(),
      config["window"]["height"].as<int>(),
      SDL_WINDOW_SHOWN
    );

    auto renderer = sdl::make_renderer(window.get(), -1, SDL_RENDERER_ACCELERATED);
    auto font = ttf::open_font("resources/fonts/SourceCodePro-Regular.ttf", 13);

    int max_fps = 120;
    auto fps_textures = make_fps_textures(*renderer, *font, max_fps);

    Cell cell({40, 40, 40, 40}, {255, 104, 180}, false);

    Uint32 goal_fps = 60;
    Uint32 goal_ticks_per_loop = 1000 / goal_fps;

    Uint32 num_frames_since_measure = 0;
    Uint32 ticks_since_measure = 0;
    Uint32 measure_every_n_ticks = 1000;
    while (true) {
      Uint32 loop_ticks_start = SDL_GetTicks();

      SDL_Event e;
      while (SDL_PollEvent(&e) != 0) {
        switch (e.type) {
          case SDL_KEYDOWN: {
            log->debug("SDL_KEYDOWN: {}", e.key.keysym.scancode);

            if (e.key.repeat == false) {
              switch (e.key.keysym.scancode) {
                case SDL_SCANCODE_DOWN:
                  break;
                case SDL_SCANCODE_UP:
                  break;
                case SDL_SCANCODE_RIGHT:
                  break;
                case SDL_SCANCODE_LEFT:
                  break;
                case SDL_SCANCODE_SPACE:
                  break;
                default:
                  break;
              }
            }
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

      if (loop_ticks < goal_ticks_per_loop) {
        Uint32 sleep_ticks = goal_ticks_per_loop - loop_ticks;
        log->trace("sleeping for {} ms", sleep_ticks);
        SDL_Delay(sleep_ticks);
      }

      // update loop_ticks
      loop_ticks = SDL_GetTicks() - loop_ticks_start;

      // measure fps every n ticks
      num_frames_since_measure++;
      ticks_since_measure += loop_ticks;
      Uint32 curr_fps = num_frames_since_measure * 1000 / ticks_since_measure;
      if (ticks_since_measure >= measure_every_n_ticks) {
        log->debug("fps: {}", curr_fps);
        ticks_since_measure = 0;
        num_frames_since_measure = 0;
      }

      // render shit on screen
      SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, SDL_ALPHA_OPAQUE);
      SDL_RenderClear(renderer.get());

      cell.render(*renderer);

      // update fps txt on screen
      auto&[fps_texture, w, h] = fps_textures[curr_fps];
      SDL_Rect fps_txt_dst_rect {0, 0, w, h};
      SDL_RenderCopy(renderer.get(), fps_texture.get(), nullptr, &fps_txt_dst_rect);

      SDL_RenderPresent(renderer.get());
    }
  } catch (const sdl::Error& ex) {
    log->critical("Caught SDL Error: {}", ex.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

