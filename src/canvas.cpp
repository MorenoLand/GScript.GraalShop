#include "app/canvas.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <string_view>

namespace app {
const gani::Sprite* findSprite(const gani::Document& document, int id) {
  for (const gani::Sprite& sprite : document.sprites)
    if (sprite.id == id) return &sprite;
  return nullptr;
}

bool isBodySprite(const gani::Sprite& sprite) {
  const std::string_view type = sprite.type;
  return type.size() == 4 && (type[0] == 'b' || type[0] == 'B') && (type[1] == 'o' || type[1] == 'O') &&
         (type[2] == 'd' || type[2] == 'D') && (type[3] == 'y' || type[3] == 'Y');
}

void drawCheckerboard(SDL_Renderer* renderer, float x, float y, float width, float height) {
  constexpr float cell = 16.0f;
  const SDL_FRect area{x, y, width, height};
  SDL_SetRenderDrawColor(renderer, 40, 42, 48, 255);
  SDL_RenderFillRect(renderer, &area);
  SDL_SetRenderDrawColor(renderer, 46, 48, 55, 255);
  const int columns = static_cast<int>((width + cell - 1) / cell);
  const int rows = static_cast<int>((height + cell - 1) / cell);
  for (int row = 0; row < rows; ++row) {
    for (int column = 0; column < columns; ++column) {
      if ((row + column) % 2 != 0) continue;
      const float left = x + static_cast<float>(column) * cell;
      const float top = y + static_cast<float>(row) * cell;
      const SDL_FRect square{left, top, std::min(cell, x + width - left), std::min(cell, y + height - top)};
      SDL_RenderFillRect(renderer, &square);
    }
  }
}

void drawFrame(SDL_Renderer* renderer, SheetCache& sheets, const std::filesystem::path& gameRoot,
               const std::filesystem::path& documentPath, const gani::Document& document, std::size_t frameIndex,
               int direction, const CanvasView& view) {
  if (frameIndex >= document.frames.size()) return;
  const gani::Frame& frame = document.frames[frameIndex];
  if (direction < 0 || static_cast<std::size_t>(direction) >= frame.rows.size()) return;
  for (const gani::FrameSprite& part : frame.rows[static_cast<std::size_t>(direction)]) {
    const gani::Sprite* sprite = part.sprite >= 0 ? findSprite(document, part.sprite) : nullptr;
    const gani::Sprite implicit{0, part.type, 0, 0, 32, 32, part.type};
    if (!sprite && part.sprite < 0) sprite = &implicit;
    if (!sprite) continue;
    const std::filesystem::path sheet = resolveSpriteSheet(gameRoot, documentPath, document, *sprite);
    if (sheet.empty()) continue;
    SDL_Texture* texture = sheets.get(sheet, isBodySprite(*sprite));
    if (!texture) continue;
    const auto drawSprite = [&](const gani::Sprite& source, int dx, int dy) {
      const SDL_FRect sourceRect{static_cast<float>(source.x), static_cast<float>(source.y),
                                 static_cast<float>(source.width), static_cast<float>(source.height)};
      const SDL_FRect target{view.originX + static_cast<float>(dx) * view.zoom,
                             view.originY + static_cast<float>(dy) * view.zoom,
                             static_cast<float>(source.width) * view.zoom,
                             static_cast<float>(source.height) * view.zoom};
      SDL_RenderTexture(renderer, texture, &sourceRect, &target);
    };
    drawSprite(*sprite, part.dx, part.dy);
    for (const gani::Attach& attach : document.attachments) {
      if (attach.sprite != sprite->id) continue;
      const gani::Sprite* attached = findSprite(document, attach.attached);
      if (!attached) continue;
      const std::filesystem::path attachedSheet = resolveSpriteSheet(gameRoot, documentPath, document, *attached);
      SDL_Texture* attachedTexture = attachedSheet.empty() ? nullptr : sheets.get(attachedSheet, isBodySprite(*attached));
      if (!attachedTexture) continue;
      const SDL_FRect sourceRect{static_cast<float>(attached->x), static_cast<float>(attached->y),
                                 static_cast<float>(attached->width), static_cast<float>(attached->height)};
      const SDL_FRect target{view.originX + static_cast<float>(part.dx + attach.dx) * view.zoom,
                             view.originY + static_cast<float>(part.dy + attach.dy) * view.zoom,
                             static_cast<float>(attached->width) * view.zoom,
                             static_cast<float>(attached->height) * view.zoom};
      SDL_RenderTexture(renderer, attachedTexture, &sourceRect, &target);
    }
  }
}
}
