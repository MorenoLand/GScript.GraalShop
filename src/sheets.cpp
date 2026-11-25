#include "app/sheets.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <SDL3/SDL.h>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string_view>
#include <vector>

namespace app {
namespace {
bool parseSuffix(std::string_view token, std::string_view prefix, int& index) {
  if (!token.starts_with(prefix)) return false;
  const std::string_view digits = token.substr(prefix.size());
  if (digits.empty()) return false;
  int value = 0;
  for (const char digit : digits) {
    if (digit < '0' || digit > '9') return false;
    value = value * 10 + (digit - '0');
  }
  index = value;
  return true;
}
}

std::filesystem::path resolveSpriteSheet(const std::filesystem::path& gameRoot,
                                         const std::filesystem::path& documentPath, const gani::Document& document,
                                         const gani::Sprite& sprite) {
  std::string type = sprite.type;
  for (char& character : type) character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
  std::string name;
  std::string folder;
  if (type.find('.') != std::string::npos) {
    name = sprite.type;
    folder = "images/downloads";
  } else if (type == "head") {
    name = document.defaults.head.value_or("head19.png");
    folder = "heads";
  } else if (type == "body") {
    name = document.defaults.body.value_or("body.png");
    folder = "bodies";
  } else if (type.starts_with("attr")) {
    int index = 0;
    if (parseSuffix(type, "attr", index)) {
      if (const auto entry = document.defaults.attrs.find(index); entry != document.defaults.attrs.end()) name = entry->second;
    }
    if (name.empty()) name = "hat0.png";
    folder = "hats";
  } else if (type.starts_with("param")) {
    int index = 0;
    if (parseSuffix(type, "param", index)) {
      if (const auto entry = document.defaults.params.find(index); entry != document.defaults.params.end()) name = entry->second;
    }
    if (name.empty()) name = sprite.name;
    folder = "images/downloads";
  } else if (type == "shield") {
    name = "shield1.png";
    folder = "shields";
  } else if (type == "sword") {
    name = "sword1.png";
    folder = "swords";
  } else if (type == "sprites") {
    name = "sprites.png";
    folder = "images/downloads";
  } else {
    name = sprite.name;
    folder = "images/downloads";
  }
  if (name.empty()) return {};
  const std::filesystem::path levels = gameRoot / "levels";
  const std::filesystem::path base = documentPath.empty() ? gameRoot : documentPath.parent_path();
  static std::map<std::string, std::filesystem::path> resolved;
  const std::string key = base.string() + "|" + name;
  if (const auto entry = resolved.find(key); entry != resolved.end()) return entry->second;
  const auto remember = [&](const std::filesystem::path& result) {
    resolved[key] = result;
    return result;
  };
  const std::filesystem::path candidates[] = {base / name,
                                              levels / folder / name,
                                              levels / "hats" / name,
                                              levels / "shields" / name,
                                              levels / "swords" / name,
                                              levels / "heads" / name,
                                              levels / "bodies" / name,
                                              levels / "weapons" / name,
                                              levels / "images/downloads" / name,
                                              levels / "images" / name,
                                              levels / name,
                                              gameRoot / name};
  for (const std::filesystem::path& candidate : candidates)
    if (std::filesystem::is_regular_file(candidate)) return remember(candidate);
  std::error_code error;
  for (std::filesystem::recursive_directory_iterator entry(levels / "images", error), end; !error && entry != end;
       entry.increment(error))
    if (entry->is_regular_file() && entry->path().filename() == name) return remember(entry->path());
  return remember(candidates[0]);
}

bool imageSize(const std::filesystem::path& path, int& width, int& height) {
  int channels = 0;
  return stbi_info(path.string().c_str(), &width, &height, &channels) != 0;
}

SheetCache::SheetCache(SDL_Renderer* renderer) : renderer_(renderer) {}

SheetCache::~SheetCache() {
  for (const auto& [key, texture] : textures_) SDL_DestroyTexture(texture);
}

SDL_Texture* SheetCache::get(const std::filesystem::path& path, bool bodyPalette) {
  const std::string key = path.string() + (bodyPalette ? "|body" : "");
  if (const auto entry = textures_.find(key); entry != textures_.end()) return entry->second;
  SDL_Texture* texture = nullptr;
  std::ifstream stream(path, std::ios::binary);
  if (stream) {
    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels =
        stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &width, &height, &channels, 4);
    if (pixels) {
      if (bodyPalette) {
        for (int index = 0; index < width * height; ++index) {
          unsigned char* pixel = pixels + index * 4;
          const unsigned char red = pixel[0];
          const unsigned char green = pixel[1];
          const unsigned char blue = pixel[2];
          if (red == 255 && green == 255 && blue == 255) {
            pixel[0] = 132;
            pixel[1] = 132;
            pixel[2] = 132;
          } else if (red == 0 && green == 0 && blue == 255) {
            pixel[0] = 41;
            pixel[1] = 41;
            pixel[2] = 41;
          } else if (red == 255 && green == 0 && blue == 0) {
            pixel[0] = 0;
            pixel[1] = 0;
            pixel[2] = 198;
          } else if (red == 206 && green == 24 && blue == 41) {
            pixel[0] = 132;
            pixel[1] = 0;
            pixel[2] = 0;
          }
        }
      }
      if (SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, pixels, width * 4)) {
        texture = SDL_CreateTextureFromSurface(renderer_, surface);
        SDL_DestroySurface(surface);
      }
      stbi_image_free(pixels);
    }
  }
  if (texture) {
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  } else {
    std::fprintf(stderr, "unable to load sprite sheet %s\n", key.c_str());
  }
  textures_[key] = texture;
  return texture;
}
}
