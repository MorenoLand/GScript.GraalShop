#include "app/font.hpp"
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#include <SDL3/SDL.h>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace app {
BitmapFont::~BitmapFont() {
  if (texture_) SDL_DestroyTexture(texture_);
}

bool BitmapFont::load(SDL_Renderer* renderer, const std::filesystem::path& path, float pixelHeight) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) return false;
  const std::vector<unsigned char> data((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  if (data.empty()) return false;
  stbtt_fontinfo info;
  if (!stbtt_InitFont(&info, data.data(), 0)) return false;
  const float scale = stbtt_ScaleForPixelHeight(&info, pixelHeight);
  int ascent = 0;
  int descent = 0;
  int lineGap = 0;
  stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
  lineHeight_ = (ascent - descent + lineGap) * scale;
  const float ascentPixels = ascent * scale;
  std::vector<unsigned char> atlas(static_cast<std::size_t>(atlasWidth_) * atlasHeight_ * 4, 0);
  int penX = 1;
  int penY = 1;
  int rowHeight = 0;
  for (int codepoint = 32; codepoint < 127; ++codepoint) {
    int advance = 0;
    int leftSide = 0;
    stbtt_GetCodepointHMetrics(&info, codepoint, &advance, &leftSide);
    Glyph& glyph = glyphs_[codepoint];
    glyph.advance = std::round(advance * scale);
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    stbtt_GetCodepointBitmapBox(&info, codepoint, scale, scale, &x0, &y0, &x1, &y1);
    const int width = x1 - x0;
    const int height = y1 - y0;
    if (width <= 0 || height <= 0) continue;
    if (penX + width + 1 >= atlasWidth_) {
      penX = 1;
      penY += rowHeight + 1;
      rowHeight = 0;
    }
    if (penY + height + 1 >= atlasHeight_) break;
    std::vector<unsigned char> bitmap(static_cast<std::size_t>(width) * height);
    stbtt_MakeCodepointBitmap(&info, bitmap.data(), width, height, width, scale, scale, codepoint);
    for (int row = 0; row < height; ++row)
      for (int column = 0; column < width; ++column) {
        const unsigned char coverage = bitmap[static_cast<std::size_t>(row) * width + column];
        const unsigned char alpha = coverage >= 96 ? 255 : 0;
        unsigned char* pixel = &atlas[(static_cast<std::size_t>(penY + row) * atlasWidth_ + penX + column) * 4];
        pixel[0] = 255;
        pixel[1] = 255;
        pixel[2] = 255;
        pixel[3] = alpha;
      }
    glyph.u0 = static_cast<float>(penX) / atlasWidth_;
    glyph.v0 = static_cast<float>(penY) / atlasHeight_;
    glyph.u1 = static_cast<float>(penX + width) / atlasWidth_;
    glyph.v1 = static_cast<float>(penY + height) / atlasHeight_;
    glyph.offsetX = static_cast<float>(x0);
    glyph.offsetY = ascentPixels + static_cast<float>(y0);
    glyph.visible = true;
    penX += width + 1;
    rowHeight = std::max(rowHeight, height);
  }
  SDL_Surface* surface = SDL_CreateSurfaceFrom(atlasWidth_, atlasHeight_, SDL_PIXELFORMAT_RGBA32, atlas.data(),
                                               atlasWidth_ * 4);
  if (!surface) return false;
  texture_ = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_DestroySurface(surface);
  if (texture_) {
    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_NEAREST);
  }
  return texture_ != nullptr;
}

bool BitmapFont::loadSystem(SDL_Renderer* renderer, const char* face, int height, bool bold) {
#ifdef _WIN32
  HDC dc = CreateCompatibleDC(nullptr);
  if (!dc) return false;
  BITMAPINFO info{};
  info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  info.bmiHeader.biWidth = 320;
  info.bmiHeader.biHeight = -48;
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 32;
  void* bits = nullptr;
  HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
  HGDIOBJ oldBitmap = nullptr;
  HFONT font = nullptr;
  HGDIOBJ oldFont = nullptr;
  if (bitmap) {
    oldBitmap = SelectObject(dc, bitmap);
    font = CreateFontA(height, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face);
    if (font) oldFont = SelectObject(dc, font);
  }
  if (!bitmap || !font || !bits) {
    if (oldFont) SelectObject(dc, oldFont);
    if (font) DeleteObject(font);
    if (oldBitmap) SelectObject(dc, oldBitmap);
    if (bitmap) DeleteObject(bitmap);
    DeleteDC(dc);
    return false;
  }
  SetBkMode(dc, TRANSPARENT);
  SetTextColor(dc, RGB(0, 0, 0));
  TEXTMETRICA metrics{};
  GetTextMetricsA(dc, &metrics);
  lineHeight_ = static_cast<float>(metrics.tmHeight);
  std::vector<unsigned char> atlas(static_cast<std::size_t>(atlasWidth_) * atlasHeight_ * 4, 0);
  int penX = 1;
  int penY = 1;
  int rowHeight = 0;
  for (int codepoint = 32; codepoint < 127; ++codepoint) {
    const char character = static_cast<char>(codepoint);
    SIZE extent{};
    GetTextExtentPoint32A(dc, &character, 1, &extent);
    PatBlt(dc, 0, 0, 320, 48, WHITENESS);
    TextOutA(dc, 0, 0, &character, 1);
    GdiFlush();
    const unsigned char* pixels = static_cast<const unsigned char*>(bits);
    int minX = 320;
    int minY = 48;
    int maxX = -1;
    int maxY = -1;
    for (int row = 0; row < 48; ++row)
      for (int column = 0; column < 320; ++column) {
        if (pixels[(static_cast<std::size_t>(row) * 320 + column) * 4] > 127) continue;
        minX = std::min(minX, column);
        minY = std::min(minY, row);
        maxX = std::max(maxX, column);
        maxY = std::max(maxY, row);
      }
    Glyph& glyph = glyphs_[codepoint];
    glyph.advance = static_cast<float>(extent.cx);
    if (maxX < minX) continue;
    const int width = maxX - minX + 1;
    const int glyphHeight = maxY - minY + 1;
    if (penX + width + 1 >= atlasWidth_) {
      penX = 1;
      penY += rowHeight + 1;
      rowHeight = 0;
    }
    if (penY + glyphHeight + 1 >= atlasHeight_) break;
    for (int row = 0; row < glyphHeight; ++row)
      for (int column = 0; column < width; ++column) {
        const bool dark = pixels[(static_cast<std::size_t>(minY + row) * 320 + minX + column) * 4] < 128;
        unsigned char* pixel = &atlas[(static_cast<std::size_t>(penY + row) * atlasWidth_ + penX + column) * 4];
        pixel[0] = 255;
        pixel[1] = 255;
        pixel[2] = 255;
        pixel[3] = dark ? 255 : 0;
      }
    glyph.u0 = static_cast<float>(penX) / atlasWidth_;
    glyph.v0 = static_cast<float>(penY) / atlasHeight_;
    glyph.u1 = static_cast<float>(penX + width) / atlasWidth_;
    glyph.v1 = static_cast<float>(penY + glyphHeight) / atlasHeight_;
    glyph.offsetX = static_cast<float>(minX);
    glyph.offsetY = static_cast<float>(minY);
    glyph.visible = true;
    penX += width + 1;
    rowHeight = std::max(rowHeight, glyphHeight);
  }
  SelectObject(dc, oldFont);
  DeleteObject(font);
  SelectObject(dc, oldBitmap);
  DeleteObject(bitmap);
  DeleteDC(dc);
  SDL_Surface* surface = SDL_CreateSurfaceFrom(atlasWidth_, atlasHeight_, SDL_PIXELFORMAT_RGBA32, atlas.data(),
                                               atlasWidth_ * 4);
  if (!surface) return false;
  texture_ = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_DestroySurface(surface);
  if (texture_) {
    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_NEAREST);
  }
  return texture_ != nullptr;
#else
  (void)renderer;
  (void)face;
  (void)height;
  (void)bold;
  return false;
#endif
}

float BitmapFont::width(const std::string& text) const {
  float total = 0.0f;
  for (const char character : text) {
    const int codepoint = static_cast<unsigned char>(character);
    if (codepoint < 32 || codepoint > 126) continue;
    total += glyphs_[codepoint].advance;
  }
  return total;
}

void BitmapFont::draw(SDL_Renderer* renderer, float x, float y, const std::string& text) const {
  if (!texture_ || !renderer) return;
  Uint8 red = 0;
  Uint8 green = 0;
  Uint8 blue = 0;
  Uint8 alpha = 255;
  SDL_GetRenderDrawColor(renderer, &red, &green, &blue, &alpha);
  SDL_SetTextureColorMod(texture_, red, green, blue);
  float penX = x;
  for (const char character : text) {
    const int codepoint = static_cast<unsigned char>(character);
    if (codepoint < 32 || codepoint > 126) continue;
    const Glyph& glyph = glyphs_[codepoint];
    if (glyph.visible) {
      const SDL_FRect source{glyph.u0 * atlasWidth_, glyph.v0 * atlasHeight_, (glyph.u1 - glyph.u0) * atlasWidth_,
                             (glyph.v1 - glyph.v0) * atlasHeight_};
      const SDL_FRect target{penX + glyph.offsetX, y + glyph.offsetY, source.w, source.h};
      SDL_RenderTexture(renderer, texture_, &source, &target);
    }
    penX += glyph.advance;
  }
}
}
