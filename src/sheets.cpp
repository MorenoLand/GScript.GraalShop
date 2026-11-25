#include "app/sheets.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <set>
#include <string_view>
#include <vector>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace app {
bool ensureRemoteFile(const std::filesystem::path& path) {
#ifdef __EMSCRIPTEN__
  std::error_code error;
  if (std::filesystem::is_regular_file(path, error)) return true;
  std::string extension = path.extension().string();
  for (char& character : extension) character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
  if (!extension.empty() && extension != ".png" && extension != ".jpg" && extension != ".jpeg" &&
      extension != ".bmp" && extension != ".gif" && extension != ".gani")
    return false;
  static std::set<std::string> missing;
  const std::string key = path.generic_string();
  if (missing.contains(key)) return false;
  std::string relative = key;
  while (!relative.empty() && relative.front() == '/') relative.erase(relative.begin());
  if (relative.empty()) return false;
  EM_ASM(
      {
        const url = UTF8ToString($0);
        const dest = UTF8ToString($1);
        try {
          const request = new XMLHttpRequest();
          request.open('GET', url, false);
          request.overrideMimeType('text/plain; charset=x-user-defined');
          request.send(null);
          if (request.status >= 200 && request.status < 300 && request.responseText.length) {
            const text = request.responseText;
            const bytes = new Uint8Array(text.length);
            for (let index = 0; index < text.length; ++index) bytes[index] = text.charCodeAt(index) & 0xff;
            const slash = dest.lastIndexOf('/');
            if (slash > 0) FS.mkdirTree(dest.substring(0, slash));
            FS.writeFile(dest, bytes);
          }
        } catch (error) {
        }
      },
      relative.c_str(), path.generic_string().c_str());
  const bool exists = std::filesystem::is_regular_file(path, error);
  if (!exists) missing.insert(key);
  return exists;
#else
  (void)path;
  return true;
#endif
}

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
  bool fetchable = true;
#ifdef __EMSCRIPTEN__
  const std::size_t dot = name.rfind('.');
  if (dot != std::string::npos) {
    std::string extension = name.substr(dot + 1);
    for (char& character : extension)
      character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    fetchable = extension == "png" || extension == "jpg" || extension == "jpeg" || extension == "bmp" ||
                extension == "gif";
  }
#endif
  const std::filesystem::path levels = gameRoot / "levels";
  const std::filesystem::path base = documentPath.empty() ? gameRoot : documentPath.parent_path();
  static std::map<std::string, std::filesystem::path> resolved;
  const std::string key = base.string() + "|" + name;
  if (const auto entry = resolved.find(key); entry != resolved.end()) return entry->second;
  const auto remember = [&](const std::filesystem::path& result) {
    resolved[key] = result;
    return result;
  };
  std::vector<std::filesystem::path> candidates;
  const std::filesystem::path specific = levels / folder / name;
#ifdef __EMSCRIPTEN__
  candidates.push_back(specific);
#endif
  candidates.push_back(base / name);
  if (std::find(candidates.begin(), candidates.end(), specific) == candidates.end()) candidates.push_back(specific);
  for (const char* directory : {"hats", "shields", "swords", "heads", "bodies", "weapons", "images/downloads",
                                "images"})
    if (folder != directory) candidates.push_back(levels / directory / name);
  candidates.push_back(levels / name);
  candidates.push_back(gameRoot / name);
  for (const std::filesystem::path& candidate : candidates)
    if ((!fetchable || ensureRemoteFile(candidate)) && std::filesystem::is_regular_file(candidate))
      return remember(candidate);
  std::error_code error;
  for (std::filesystem::recursive_directory_iterator entry(levels / "images", error), end; !error && entry != end;
       entry.increment(error))
    if (entry->is_regular_file() && entry->path().filename() == name) return remember(entry->path());
  return remember(candidates[0]);
}

bool imageSize(const std::filesystem::path& path, int& width, int& height) {
  int channels = 0;
  ensureRemoteFile(path);
  return stbi_info(path.string().c_str(), &width, &height, &channels) != 0;
}

SheetCache::SheetCache(SDL_Renderer* renderer) : renderer_(renderer) {}

SheetCache::~SheetCache() {
  for (const auto& [key, texture] : textures_) SDL_DestroyTexture(texture);
}

SDL_Texture* SheetCache::get(const std::filesystem::path& path, bool bodyPalette) {
  const std::string key = path.string() + (bodyPalette ? "|body" : "");
  if (const auto entry = textures_.find(key); entry != textures_.end()) return entry->second;
  std::string extension = path.extension().string();
  for (char& character : extension) character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
  if (extension == ".mng") return nullptr;
  SDL_Texture* texture = nullptr;
  ensureRemoteFile(path);
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
