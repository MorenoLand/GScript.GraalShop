#pragma once
#include "app/sheets.hpp"
#include "gani/gani.hpp"
#include <cstddef>
#include <filesystem>

struct SDL_Renderer;

namespace app {
struct CanvasView {
  float zoom{2.0f};
  float originX{0.0f};
  float originY{0.0f};
};

const gani::Sprite* findSprite(const gani::Document& document, int id);
bool isBodySprite(const gani::Sprite& sprite);

void drawCheckerboard(SDL_Renderer* renderer, float x, float y, float width, float height);

void drawFrame(SDL_Renderer* renderer, SheetCache& sheets, const std::filesystem::path& gameRoot,
               const std::filesystem::path& documentPath, const gani::Document& document, std::size_t frameIndex,
               int direction, const CanvasView& view);
}
