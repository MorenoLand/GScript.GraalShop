#include "app/canvas.hpp"
#include "app/editor.hpp"
#include "app/sheets.hpp"
#include "gani/gani.hpp"
#include <SDL3/SDL.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
using app::CanvasView;
using app::drawCheckerboard;
using app::drawFrame;
using app::Editor;
using app::SheetCache;

#ifdef __EMSCRIPTEN__
app::Editor* g_editor = nullptr;
#endif

struct Options {
  std::vector<std::filesystem::path> documents;
  std::filesystem::path gameRoot;
  std::filesystem::path render;
  std::filesystem::path shot;
  std::filesystem::path addSprite;
  std::filesystem::path exeDir;
  bool check{};
  bool save{};
  int direction{2};
  int frame{};
};

bool parseInteger(const char* text, int& value) {
  char* end = nullptr;
  const long parsed = std::strtol(text, &end, 10);
  if (end == text || *end != '\0') return false;
  value = static_cast<int>(parsed);
  return true;
}

Options parseOptions(int argc, char* argv[]) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--check") {
      options.check = true;
      continue;
    }
    if (argument == "--game" && index + 1 < argc) {
      options.gameRoot = argv[++index];
      continue;
    }
    if (argument == "--render" && index + 1 < argc) {
      options.render = argv[++index];
      continue;
    }
    if (argument == "--shot" && index + 1 < argc) {
      options.shot = argv[++index];
      continue;
    }
    if (argument == "--add-sprite" && index + 1 < argc) {
      options.addSprite = argv[++index];
      continue;
    }
    if (argument == "--save") {
      options.save = true;
      continue;
    }
    if (argument == "--direction" && index + 1 < argc) {
      parseInteger(argv[++index], options.direction);
      continue;
    }
    if (argument == "--frame" && index + 1 < argc) {
      parseInteger(argv[++index], options.frame);
      continue;
    }
    options.documents.emplace_back(argument);
  }
  return options;
}

std::filesystem::path findGameRoot(std::filesystem::path start) {
  if (start.empty()) start = std::filesystem::current_path();
  std::error_code error;
  for (;;) {
    if (std::filesystem::is_directory(start / "levels" / "heads", error)) return start;
    const std::filesystem::path parent = start.parent_path();
    if (parent == start) return {};
    start = parent;
  }
}

std::filesystem::path defaultDocument(const std::filesystem::path& gameRoot) {
  const std::filesystem::path candidate = gameRoot / "levels" / "ganis" / "idle.gani";
  std::error_code error;
  return std::filesystem::is_regular_file(candidate, error) ? candidate : std::filesystem::path{};
}

int reportError(const std::filesystem::path& path, const gani::ParseResult& result) {
  if (result.error)
    std::fprintf(stderr, "%s:%zu: %s\n", path.string().c_str(), result.error->line, result.error->message.c_str());
  else
    std::fprintf(stderr, "%s: unable to load\n", path.string().c_str());
  return 1;
}

int runCheck(const Options& options) {
  int failed = 0;
  for (const std::filesystem::path& path : options.documents) {
    const gani::ParseResult result = gani::load(path);
    if (!result) {
      failed += reportError(path, result);
      continue;
    }
    const gani::Document& document = *result.document;
    std::printf("%s: %zu sprites, %zu frames, %s\n", path.string().c_str(), document.sprites.size(),
                document.frames.size(), document.version == gani::Version::gani0001 ? "GANI0001" : "GANI0002");
  }
  return failed == 0 ? 0 : 1;
}

int runRender(const Options& options) {
  const gani::ParseResult result = gani::load(options.documents.front());
  if (!result) return reportError(options.documents.front(), result);
  const gani::Document& document = *result.document;
  const std::filesystem::path gameRoot = options.gameRoot.empty()
                                             ? findGameRoot(options.documents.front().parent_path())
                                             : options.gameRoot;
  constexpr int size = 640;
  if (!SDL_Init(0)) {
    std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }
  SDL_Surface* surface = SDL_CreateSurface(size, size, SDL_PIXELFORMAT_ARGB8888);
  SDL_Renderer* renderer = surface ? SDL_CreateSoftwareRenderer(surface) : nullptr;
  if (!renderer) {
    std::fprintf(stderr, "renderer creation failed: %s\n", SDL_GetError());
    SDL_DestroySurface(surface);
    SDL_Quit();
    return 1;
  }
  SDL_SetRenderDrawColor(renderer, 24, 26, 30, 255);
  SDL_RenderClear(renderer);
  drawCheckerboard(renderer, 0.0f, 0.0f, static_cast<float>(size), static_cast<float>(size));
  SheetCache sheets(renderer);
  const float zoom = 4.0f;
  const CanvasView view{zoom, static_cast<float>(size) / 2.0f - 24.0f * zoom,
                        static_cast<float>(size) / 2.0f - 24.0f * zoom};
  const std::size_t frame = std::min(static_cast<std::size_t>(std::max(options.frame, 0)), document.frames.size() - 1);
  drawFrame(renderer, sheets, gameRoot, options.documents.front(), document, frame, options.direction, view);
  SDL_RenderPresent(renderer);
  const bool saved = SDL_SaveBMP(surface, options.render.string().c_str());
  if (!saved) std::fprintf(stderr, "unable to save %s: %s\n", options.render.string().c_str(), SDL_GetError());
  SDL_DestroyRenderer(renderer);
  SDL_DestroySurface(surface);
  SDL_Quit();
  return saved ? 0 : 1;
}

int runWindow(const Options& options) {
  std::filesystem::path documentPath = options.documents.empty() ? std::filesystem::path{} : options.documents.front();
  std::filesystem::path gameRoot = options.gameRoot;
  const std::filesystem::path settingsPath =
      options.exeDir.empty() ? std::filesystem::path{} : options.exeDir / "settings.ini";
  if (documentPath.empty() && gameRoot.empty() && !settingsPath.empty()) {
    std::ifstream stream(settingsPath);
    std::string line;
    while (std::getline(stream, line)) {
      if (line.rfind("folder=", 0) != 0) continue;
      std::string value = line.substr(7);
      while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) value.pop_back();
      const std::filesystem::path folder = value;
      if (folder.empty()) break;
      gameRoot = folder;
      documentPath = defaultDocument(gameRoot);
      break;
    }
  }
  if (!documentPath.empty()) {
    std::error_code error;
    if (std::filesystem::is_directory(documentPath, error)) {
      gameRoot = documentPath;
      documentPath = defaultDocument(gameRoot);
    }
  }
  if (gameRoot.empty()) gameRoot = findGameRoot(documentPath.empty() ? std::filesystem::current_path() : documentPath.parent_path());
#ifdef __EMSCRIPTEN__
  if (gameRoot.empty() && std::filesystem::is_directory("/game/levels/heads")) gameRoot = "/game";
#endif
  if (documentPath.empty() && !gameRoot.empty()) documentPath = defaultDocument(gameRoot);
  gani::Document document;
  if (!documentPath.empty()) {
    const gani::ParseResult result = gani::load(documentPath);
    if (!result) return reportError(documentPath, result);
    document = std::move(*result.document);
  }
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }
  std::string title = "Graal Shop - The Graal Animation Editor";
  if (!documentPath.empty()) title += " - " + documentPath.filename().string();
  SDL_Window* window = SDL_CreateWindow(title.c_str(), 538, 453,
                                        SDL_WINDOW_RESIZABLE | (options.shot.empty() ? 0 : SDL_WINDOW_HIDDEN));
#ifdef _WIN32
  if (window) {
    if (HWND handle = static_cast<HWND>(
            SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr))) {
      const BOOL dark = FALSE;
      DwmSetWindowAttribute(handle, 20, &dark, sizeof(dark));
      DwmSetWindowAttribute(handle, 19, &dark, sizeof(dark));
    }
  }
#endif
  SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, nullptr) : nullptr;
  if (!renderer) {
    std::fprintf(stderr, "window creation failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  SDL_SetRenderVSync(renderer, 1);
  SheetCache sheets(renderer);
  Editor editor(renderer, window, sheets, std::move(document), documentPath, gameRoot, settingsPath);
#ifdef __EMSCRIPTEN__
  g_editor = &editor;
#endif
  if (!options.addSprite.empty()) editor.addSpriteFromFile(options.addSprite);
  if (options.save) {
    const bool saved = editor.save();
    if (!saved) std::fprintf(stderr, "unable to save %s\n", documentPath.string().c_str());
    if (options.shot.empty()) {
      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(window);
      SDL_Quit();
      return saved ? 0 : 1;
    }
  }
  if (!options.shot.empty()) {
    int width = 0;
    int height = 0;
    SDL_GetRenderOutputSize(renderer, &width, &height);
    editor.draw(width, height);
    SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
    const bool saved = surface && SDL_SaveBMP(surface, options.shot.string().c_str());
    if (!saved) std::fprintf(stderr, "unable to save %s: %s\n", options.shot.string().c_str(), SDL_GetError());
    SDL_DestroySurface(surface);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return saved ? 0 : 1;
  }
  Uint64 previous = SDL_GetTicks();
  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) running = false;
      else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) running = false;
      else {
        int width = 0;
        int height = 0;
        SDL_GetRenderOutputSize(renderer, &width, &height);
        SDL_Event mapped = event;
        float lx = 0.0f;
        float ly = 0.0f;
        if (mapped.type == SDL_EVENT_MOUSE_MOTION &&
            SDL_RenderCoordinatesFromWindow(renderer, mapped.motion.x, mapped.motion.y, &lx, &ly)) {
          mapped.motion.x = lx;
          mapped.motion.y = ly;
        } else if ((mapped.type == SDL_EVENT_MOUSE_BUTTON_DOWN || mapped.type == SDL_EVENT_MOUSE_BUTTON_UP) &&
                   SDL_RenderCoordinatesFromWindow(renderer, mapped.button.x, mapped.button.y, &lx, &ly)) {
          mapped.button.x = lx;
          mapped.button.y = ly;
        } else if (mapped.type == SDL_EVENT_MOUSE_WHEEL &&
                   SDL_RenderCoordinatesFromWindow(renderer, mapped.wheel.mouse_x, mapped.wheel.mouse_y, &lx, &ly)) {
          mapped.wheel.mouse_x = lx;
          mapped.wheel.mouse_y = ly;
        }
        editor.handleEvent(mapped, width, height);
      }
    }
    const Uint64 now = SDL_GetTicks();
    const float delta = static_cast<float>(now - previous) / 1000.0f;
    previous = now;
    editor.update(delta);
    int width = 0;
    int height = 0;
    SDL_GetRenderOutputSize(renderer, &width, &height);
    editor.draw(width, height);
    SDL_RenderPresent(renderer);
    SDL_Delay(1);
  }
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
}

#ifdef __EMSCRIPTEN__
extern "C" EMSCRIPTEN_KEEPALIVE void graalshop_open_file(const char* path) {
  if (g_editor && path) g_editor->loadDocument(path);
}

extern "C" EMSCRIPTEN_KEEPALIVE void graalshop_add_sprite(const char* image, const char* description, int index, int x,
                                                           int y, int width, int height, int gridWidth,
                                                           int gridHeight) {
  if (g_editor) g_editor->addSpriteDefinition(image ? image : "", description ? description : "", index, x, y, width,
                                              height, gridWidth, gridHeight);
}
#endif

int main(int argc, char* argv[]) {
  Options options = parseOptions(argc, argv);
  options.exeDir = argc > 0 ? std::filesystem::absolute(argv[0]).parent_path() : std::filesystem::path{};
  const Options& resolved = options;
  if (resolved.check) return runCheck(resolved);
  if (!resolved.render.empty()) {
    if (resolved.documents.empty()) {
      std::fprintf(stderr, "--render requires a document\n");
      return 2;
    }
    return runRender(resolved);
  }
  return runWindow(resolved);
}
