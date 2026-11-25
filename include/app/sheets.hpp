#pragma once
#include "gani/gani.hpp"
#include <filesystem>
#include <map>
#include <string>

struct SDL_Renderer;
struct SDL_Texture;

namespace app {
std::filesystem::path resolveSpriteSheet(const std::filesystem::path& gameRoot,
                                         const std::filesystem::path& documentPath, const gani::Document& document,
                                         const gani::Sprite& sprite);

bool imageSize(const std::filesystem::path& path, int& width, int& height);

class SheetCache {
public:
  explicit SheetCache(SDL_Renderer* renderer);
  ~SheetCache();
  SheetCache(const SheetCache&) = delete;
  SheetCache& operator=(const SheetCache&) = delete;
  SDL_Texture* get(const std::filesystem::path& path, bool bodyPalette = false);

private:
  SDL_Renderer* renderer_;
  std::map<std::string, SDL_Texture*> textures_;
};
}
