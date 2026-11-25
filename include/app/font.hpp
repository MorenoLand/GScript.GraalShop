#pragma once
#include <filesystem>
#include <string>

struct SDL_Renderer;
struct SDL_Texture;

namespace app {
class BitmapFont {
public:
  ~BitmapFont();
  bool load(SDL_Renderer* renderer, const std::filesystem::path& path, float pixelHeight);
  bool loadSystem(SDL_Renderer* renderer, const char* face, int height, bool bold);
  [[nodiscard]] bool ready() const { return texture_ != nullptr; }
  [[nodiscard]] float width(const std::string& text) const;
  [[nodiscard]] float height() const { return lineHeight_; }
  void draw(SDL_Renderer* renderer, float x, float y, const std::string& text) const;

private:
  struct Glyph {
    float u0{}, v0{}, u1{}, v1{};
    float offsetX{}, offsetY{}, advance{};
    bool visible{};
  };
  SDL_Texture* texture_{};
  Glyph glyphs_[128]{};
  float lineHeight_{11.0f};
  int atlasWidth_{512};
  int atlasHeight_{256};
};
}
