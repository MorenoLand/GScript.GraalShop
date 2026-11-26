#include "app/editor.hpp"
#include "app/font.hpp"
#include "glyphs.hpp"
#include "tickfont.hpp"
#include "timefont.hpp"
#include <SDL3/SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace app {
namespace {
struct FontSlot {
  app::BitmapFont font;
  bool loaded = false;
};

void ensureFont(FontSlot& slot, SDL_Renderer* renderer, const std::filesystem::path& assets, bool bold, float size) {
  if (slot.loaded) return;
  slot.loaded = true;
#ifdef _WIN32
  if (slot.font.loadSystem(renderer, "MS Sans Serif", bold ? -16 : -11, bold)) return;
#endif
  std::vector<std::filesystem::path> candidates;
  candidates.emplace_back(bold ? "C:/Windows/Fonts/tahomabd.ttf" : "C:/Windows/Fonts/tahoma.ttf");
  if (!assets.empty()) candidates.push_back(assets / (bold ? "DejaVuSans-Bold.ttf" : "DejaVuSans.ttf"));
  candidates.emplace_back(bold ? "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
                               : "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
  candidates.emplace_back("C:/Windows/Fonts/arial.ttf");
  for (const std::filesystem::path& candidate : candidates)
    if (slot.font.load(renderer, candidate, size)) break;
}

FontSlot g_regular;

void drawTextEx(SDL_Renderer* renderer, FontSlot& slot, float x, float baseline, const std::string& text,
                const std::filesystem::path& assets) {
  ensureFont(slot, renderer, assets, false, 13.0f);
  if (slot.font.ready()) {
    slot.font.draw(renderer, x, baseline, text);
    return;
  }
  SDL_SetRenderScale(renderer, 1.0f, 1.0f);
  SDL_RenderDebugText(renderer, x, baseline, text.c_str());
}

void drawText(SDL_Renderer* renderer, float x, float baseline, const std::string& text,
              const std::filesystem::path& assets) {
  drawTextEx(renderer, g_regular, x, baseline, text, assets);
}

constexpr float kToolbarHeight = 33.0f;
constexpr float kStripLeft = 19.0f;
constexpr float kStripTop = 35.0f;
constexpr float kStripWidth = 50.0f;
constexpr float kStripHeight = 329.0f;
constexpr float kCanvasLeft = 69.0f;
constexpr float kCanvasTop = 35.0f;
constexpr float kCanvasWidth = 297.0f;
constexpr float kCanvasHeight = 329.0f;
constexpr float kInfoLeft = 376.0f;
constexpr float kBarTop = 412.0f;
constexpr int kStripMargin = 5;
constexpr int kStripGap = 10;
constexpr float kRowTop = 197.0f;
constexpr float kRowHeight = 16.0f;
constexpr float kThumbTop = 52.0f;
constexpr float kThumbTrack = 295.0f;
constexpr float kThumbHeight = 19.0f;
const float kToolbarCells[] = {10.0f, 35.0f, 60.0f, 85.0f, 110.0f, 143.0f, 168.0f, 193.0f, 224.0f, 249.0f, 274.0f, 330.0f};
const SDL_Color kText{0, 0, 0, 255};
const SDL_Color kForm{240, 240, 240, 255};
const SDL_Color kCanvas{0, 128, 0, 255};
const SDL_Color kGrid{0, 160, 0, 255};

void setColor(SDL_Renderer* renderer, const SDL_Color& color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void fill(SDL_Renderer* renderer, const SDL_FRect& rect, const SDL_Color& color) {
  setColor(renderer, color);
  SDL_RenderFillRect(renderer, &rect);
}

void outline(SDL_Renderer* renderer, const SDL_FRect& rect, const SDL_Color& color) {
  setColor(renderer, color);
  SDL_RenderRect(renderer, &rect);
}

void hLine(SDL_Renderer* renderer, float x1, float x2, float y, SDL_Color color) {
  setColor(renderer, color);
  SDL_RenderLine(renderer, x1, y, x2, y);
}

void vLine(SDL_Renderer* renderer, float x, float y1, float y2, SDL_Color color) {
  setColor(renderer, color);
  SDL_RenderLine(renderer, x, y1, x, y2);
}

void raised(SDL_Renderer* renderer, const SDL_FRect& rect) {
  fill(renderer, rect, kForm);
  hLine(renderer, rect.x, rect.x + rect.w - 1.0f, rect.y, SDL_Color{255, 255, 255, 255});
  vLine(renderer, rect.x, rect.y, rect.y + rect.h - 1.0f, SDL_Color{255, 255, 255, 255});
  hLine(renderer, rect.x + 1.0f, rect.x + rect.w - 2.0f, rect.y + 1.0f, SDL_Color{227, 227, 227, 255});
  vLine(renderer, rect.x + 1.0f, rect.y + 1.0f, rect.y + rect.h - 2.0f, SDL_Color{227, 227, 227, 255});
  hLine(renderer, rect.x + 1.0f, rect.x + rect.w - 2.0f, rect.y + rect.h - 2.0f, SDL_Color{160, 160, 160, 255});
  vLine(renderer, rect.x + rect.w - 2.0f, rect.y + 1.0f, rect.y + rect.h - 2.0f, SDL_Color{160, 160, 160, 255});
  hLine(renderer, rect.x, rect.x + rect.w - 1.0f, rect.y + rect.h - 1.0f, SDL_Color{105, 105, 105, 255});
  vLine(renderer, rect.x + rect.w - 1.0f, rect.y, rect.y + rect.h - 1.0f, SDL_Color{105, 105, 105, 255});
}

void sunken(SDL_Renderer* renderer, const SDL_FRect& rect) {
  fill(renderer, rect, SDL_Color{255, 255, 255, 255});
  hLine(renderer, rect.x, rect.x + rect.w - 1.0f, rect.y, SDL_Color{160, 160, 160, 255});
  vLine(renderer, rect.x, rect.y, rect.y + rect.h - 1.0f, SDL_Color{160, 160, 160, 255});
  hLine(renderer, rect.x + 1.0f, rect.x + rect.w - 1.0f, rect.y + 1.0f, SDL_Color{105, 105, 105, 255});
  vLine(renderer, rect.x + 1.0f, rect.y + 1.0f, rect.y + rect.h - 1.0f, SDL_Color{105, 105, 105, 255});
  hLine(renderer, rect.x + 1.0f, rect.x + rect.w - 2.0f, rect.y + rect.h - 2.0f, SDL_Color{227, 227, 227, 255});
  vLine(renderer, rect.x + rect.w - 2.0f, rect.y + 1.0f, rect.y + rect.h - 2.0f, SDL_Color{227, 227, 227, 255});
  hLine(renderer, rect.x, rect.x + rect.w - 1.0f, rect.y + rect.h - 1.0f, SDL_Color{255, 255, 255, 255});
  vLine(renderer, rect.x + rect.w - 1.0f, rect.y, rect.y + rect.h - 1.0f, SDL_Color{255, 255, 255, 255});
}

void comboBox(SDL_Renderer* renderer, const SDL_FRect& rect) {
  sunken(renderer, rect);
  const SDL_FRect button{rect.x + rect.w - 19.0f, rect.y + 2.0f, 17.0f, rect.h - 4.0f};
  fill(renderer, button, kForm);
  hLine(renderer, button.x, button.x + button.w - 1.0f, button.y, SDL_Color{227, 227, 227, 255});
  vLine(renderer, button.x, button.y, button.y + button.h - 2.0f, SDL_Color{227, 227, 227, 255});
  hLine(renderer, button.x + 1.0f, button.x + button.w - 2.0f, button.y + 1.0f, SDL_Color{255, 255, 255, 255});
  vLine(renderer, button.x + 1.0f, button.y + 1.0f, button.y + button.h - 3.0f, SDL_Color{255, 255, 255, 255});
  hLine(renderer, button.x + 1.0f, button.x + button.w - 2.0f, button.y + button.h - 2.0f, SDL_Color{160, 160, 160, 255});
  vLine(renderer, button.x + button.w - 2.0f, button.y + 1.0f, button.y + button.h - 3.0f, SDL_Color{160, 160, 160, 255});
  hLine(renderer, button.x, button.x + button.w - 1.0f, button.y + button.h - 1.0f, SDL_Color{105, 105, 105, 255});
  vLine(renderer, button.x + button.w - 1.0f, button.y, button.y + button.h - 1.0f, SDL_Color{105, 105, 105, 255});
  const float cx = button.x + 8.0f;
  for (int row = 0; row < 4; ++row)
    hLine(renderer, cx - 3.0f + row, cx + 3.0f - row, button.y + 7.0f + row, SDL_Color{0, 0, 0, 255});
}

void checkBox(SDL_Renderer* renderer, float x, float y, bool checked) {
  const SDL_FRect rect{x, y, 12.0f, 13.0f};
  fill(renderer, rect, SDL_Color{255, 255, 255, 255});
  hLine(renderer, x, x + 11.0f, y, SDL_Color{105, 105, 105, 255});
  vLine(renderer, x, y, y + 12.0f, SDL_Color{105, 105, 105, 255});
  hLine(renderer, x + 1.0f, x + 11.0f, y + 1.0f, SDL_Color{160, 160, 160, 255});
  vLine(renderer, x + 1.0f, y + 1.0f, y + 12.0f, SDL_Color{160, 160, 160, 255});
  hLine(renderer, x, x + 11.0f, y + 12.0f, SDL_Color{227, 227, 227, 255});
  vLine(renderer, x + 11.0f, y, y + 12.0f, SDL_Color{227, 227, 227, 255});
  if (!checked) return;
  setColor(renderer, SDL_Color{0, 0, 0, 255});
  for (int step = 0; step < 3; ++step) SDL_RenderLine(renderer, x + 3.0f + step, y + 6.0f + step, x + 4.0f + step, y + 7.0f + step);
  for (int step = 0; step < 6; ++step) SDL_RenderLine(renderer, x + 6.0f + step, y + 8.0f - step, x + 7.0f + step, y + 8.0f - step);
}

const GlyphInfo* findGlyph(const char* name) {
  for (const GlyphInfo& glyph : kGlyphs)
    if (std::strcmp(glyph.name, name) == 0) return &glyph;
  return nullptr;
}

void rightTriangle(SDL_Renderer* renderer, float x, float y, float width, float height, SDL_Color fillColor,
                   SDL_Color outlineColor) {
  for (int row = 0; row < static_cast<int>(height); ++row) {
    const int half = static_cast<int>(height) / 2;
    const int extent = std::min(row, static_cast<int>(height) - 1 - row);
    const float length = width * static_cast<float>(extent) / static_cast<float>(std::max(1, half));
    hLine(renderer, x, x + length, y + row, outlineColor);
  }
  for (int row = 1; row < static_cast<int>(height) - 1; ++row) {
    const int half = static_cast<int>(height) / 2;
    const int extent = std::min(row, static_cast<int>(height) - 1 - row) - 1;
    const float length = width * static_cast<float>(std::max(0, extent)) / static_cast<float>(std::max(1, half));
    hLine(renderer, x + 1.0f, x + 1.0f + length, y + row, fillColor);
  }
}

bool inside(const SDL_FRect& rect, float x, float y) {
  return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
}

void mousePosition(SDL_Renderer* renderer, float& x, float& y) {
  SDL_GetMouseState(&x, &y);
  SDL_RenderCoordinatesFromWindow(renderer, x, y, &x, &y);
}

int keyframeInt(const gani::MovieKeyframe& keyframe, const char* key, int fallback) {
  const auto entry = keyframe.values.find(key);
  return entry == keyframe.values.end() ? fallback : std::atoi(entry->second.c_str());
}

const gani::MovieKeyframe* nearestKeyframe(const gani::MovieActor& actor, int frame) {
  const gani::MovieKeyframe* best = nullptr;
  for (const gani::MovieKeyframe& keyframe : actor.keyframes)
    if (keyframe.frame <= frame && (!best || keyframe.frame > best->frame)) best = &keyframe;
  if (best) return best;
  return actor.keyframes.empty() ? nullptr : &actor.keyframes.front();
}
}

Editor::Editor(SDL_Renderer* renderer, SDL_Window* window, SheetCache& sheets, gani::Document document,
               std::filesystem::path path, std::filesystem::path gameRoot, std::filesystem::path settingsPath)
    : renderer_(renderer), window_(window), sheets_(sheets), document_(std::move(document)), path_(std::move(path)),
      gameRoot_(std::move(gameRoot)), settingsPath_(std::move(settingsPath)) {
  selected_ = document_.sprites.empty() ? -1 : 0;
  view_ = layout(538, 453);
  if (!settingsPath_.empty()) {
    std::ifstream stream(settingsPath_);
    std::string line;
    while (std::getline(stream, line)) {
      if (line.rfind("automaticgani=", 0) == 0) automaticGani_ = line.substr(15) == "1";
      else if (line.rfind("graal2002=", 0) == 0) graal2002_ = line.substr(10) == "1";
    }
  }
}

void Editor::writeSettings() {
  if (settingsPath_.empty() || gameRoot_.empty()) return;
  std::ofstream stream(settingsPath_, std::ios::binary | std::ios::trunc);
  if (stream) {
    stream << "folder=" << gameRoot_.string() << '\n';
    stream << "automaticgani=" << (automaticGani_ ? 1 : 0) << '\n';
    stream << "graal2002=" << (graal2002_ ? 1 : 0) << '\n';
  }
}

std::filesystem::path Editor::assetDir() const {
  return settingsPath_.empty() ? std::filesystem::path{} : settingsPath_.parent_path() / "assets";
}

namespace {
std::string hexColor(unsigned int value) {
  char buffer[8];
  std::snprintf(buffer, sizeof buffer, "%06X", value & 0xFFFFFFu);
  return buffer;
}
}  // namespace

int Editor::movieLengthSeconds() const {
  const int frames = std::max(1, document_.frameCount.value_or(100));
  return std::max(1, (frames + 19) / 20);
}

gani::FrameSprite* Editor::selectedFrameSprite() {
  gani::Frame* frame = currentFrame();
  if (!frame || direction_ < 0 || static_cast<std::size_t>(direction_) >= frame->rows.size()) return nullptr;
  if (selected_ < 0 || selected_ >= static_cast<int>(document_.sprites.size())) return nullptr;
  const int id = document_.sprites[static_cast<std::size_t>(selected_)].id;
  for (gani::FrameSprite& part : frame->rows[static_cast<std::size_t>(direction_)])
    if (part.sprite == id) return &part;
  return nullptr;
}

std::string Editor::spritePanelValue(int row) const {
  const gani::Sprite* selected = selected_ >= 0 && selected_ < static_cast<int>(document_.sprites.size())
                                     ? &document_.sprites[static_cast<std::size_t>(selected_)]
                                     : nullptr;
  if (row == 0) {
    if (framePanel_ && !document_.frames.empty()) {
      const std::size_t index = std::min(frame_, document_.frames.size() - 1);
      char buffer[16];
      std::snprintf(buffer, sizeof buffer, "%.2f",
                    0.05 * static_cast<double>(std::max(1, document_.frames[index].waitRepeats + 1)));
      return buffer;
    }
    return selected ? std::to_string(selected->id) : std::string{};
  }
  switch (row) {
    case 2: return document_.loop ? "true" : "false";
    case 3: return document_.continuous ? "true" : "false";
    case 4: return hexColor(document_.backColor);
    case 5: return document_.singleDirection ? "true" : "false";
    case 6: return document_.setbackTo.value_or("");
    case 8: return document_.defaults.head.value_or("");
    case 9: return document_.defaults.body.value_or("");
    default: break;
  }
  if (row >= 10 && row <= 12) {
    const auto entry = document_.defaults.attrs.find(row - 9);
    return entry == document_.defaults.attrs.end() ? std::string{} : entry->second;
  }
  if (row >= 13 && row <= 15) {
    const auto entry = document_.defaults.params.find(row - 12);
    return entry == document_.defaults.params.end() ? std::string{} : entry->second;
  }
  return {};
}

std::string Editor::moviePanelValue(int row) const {
  switch (row) {
    case 0: return std::to_string(movieLengthSeconds());
    case 1: return document_.loop ? "true" : "false";
    case 2: return document_.continuous ? "true" : "false";
    case 3: return hexColor(document_.backColor);
    case 4: return document_.setbackTo.value_or("");
    case 5: return document_.defaults.head.value_or("");
    case 6: return document_.defaults.body.value_or("");
    default: break;
  }
  if (row >= 7 && row <= 9) {
    const auto entry = document_.defaults.attrs.find(row - 6);
    return entry == document_.defaults.attrs.end() ? std::string{} : entry->second;
  }
  if (row >= 10 && row <= 12) {
    const auto entry = document_.defaults.params.find(row - 9);
    return entry == document_.defaults.params.end() ? std::string{} : entry->second;
  }
  return {};
}

void Editor::beginPanelEdit(int target, int row) {
  if (editingText_) {
    editingText_ = false;
    SDL_StopTextInput(window_);
  }
  if (target == 1 && row == 0 && !document_.movie && framePanel_) playing_ = false;
  panelEditTarget_ = target;
  panelEditRow_ = row;
  panelEditFresh_ = true;
  if (target == 2 || target == 3) {
    gani::FrameSprite* part = selectedFrameSprite();
    if (!part) {
      panelEditTarget_ = 0;
      return;
    }
    panelEditText_ = std::to_string(target == 2 ? part->dx : part->dy);
  } else {
    panelEditText_ = document_.movie ? moviePanelValue(row) : spritePanelValue(row);
  }
  panelEditOriginal_ = panelEditText_;
  SDL_StartTextInput(window_);
}

void Editor::cancelPanelEdit() {
  panelEditTarget_ = 0;
  panelEditRow_ = -1;
  panelEditFresh_ = false;
  SDL_StopTextInput(window_);
}

void Editor::togglePanelBool(int row) {
  bool* value = nullptr;
  if (document_.movie) {
    if (row == 1) value = &document_.loop;
    else if (row == 2) value = &document_.continuous;
  } else {
    if (row == 2) value = &document_.loop;
    else if (row == 3) value = &document_.continuous;
    else if (row == 5) value = &document_.singleDirection;
  }
  if (!value) return;
  pushUndo();
  *value = !*value;
  dirty_ = true;
  status_ = "updated";
}

void Editor::setPanelValue(int row, const std::string& value) {
  auto setOptional = [&](std::optional<std::string>& target) {
    const bool empty = value.empty();
    if ((!target && empty) || (target && *target == value)) return;
    pushUndo();
    if (empty) target.reset();
    else target = value;
    dirty_ = true;
  };
  if (document_.movie) {
    if (row == 0) {
      char* end = nullptr;
      const long seconds = std::strtol(value.c_str(), &end, 10);
      if (end == value.c_str() || *end != '\0' || seconds <= 0) return;
      const int frames = static_cast<int>(std::clamp(seconds, 1L, 100000L)) * 20;
      if (document_.frameCount.value_or(-1) == frames) return;
      pushUndo();
      document_.frameCount = frames;
      dirty_ = true;
      status_ = "updated";
      return;
    }
    if (row == 3) {
      if (value.size() != 6) return;
      char* end = nullptr;
      const unsigned long color = std::strtoul(value.c_str(), &end, 16);
      if (end == value.c_str() || *end != '\0') return;
      if (document_.backColor == color) return;
      pushUndo();
      document_.backColor = static_cast<unsigned int>(color);
      dirty_ = true;
      status_ = "updated";
      return;
    }
    if (row == 4) setOptional(document_.setbackTo);
    else if (row == 5) setOptional(document_.defaults.head);
    else if (row == 6) setOptional(document_.defaults.body);
    else if (row >= 7 && row <= 9) {
      const int index = row - 6;
      auto entry = document_.defaults.attrs.find(index);
      if (value.empty()) {
        if (entry == document_.defaults.attrs.end()) return;
        pushUndo();
        document_.defaults.attrs.erase(index);
      } else {
        if (entry != document_.defaults.attrs.end() && entry->second == value) return;
        pushUndo();
        document_.defaults.attrs[index] = value;
      }
      dirty_ = true;
      status_ = "updated";
    } else if (row >= 10 && row <= 12) {
      const int index = row - 9;
      auto entry = document_.defaults.params.find(index);
      if (value.empty()) {
        if (entry == document_.defaults.params.end()) return;
        pushUndo();
        document_.defaults.params.erase(index);
      } else {
        if (entry != document_.defaults.params.end() && entry->second == value) return;
        pushUndo();
        document_.defaults.params[index] = value;
      }
      dirty_ = true;
      status_ = "updated";
    }
    return;
  }
  if (row == 0) {
    if (!framePanel_ || document_.frames.empty()) return;
    char* end = nullptr;
    const double seconds = std::strtod(value.c_str(), &end);
    if (end == value.c_str() || *end != '\0' || seconds <= 0.0) return;
    const std::size_t index = std::min(frame_, document_.frames.size() - 1);
    const int wait = std::max(0, static_cast<int>(std::lround(seconds / 0.05)) - 1);
    if (document_.frames[index].waitRepeats == wait) return;
    pushUndo();
    document_.frames[index].waitRepeats = wait;
    dirty_ = true;
    status_ = "updated";
    return;
  }
  if (row == 4) {
    if (value.size() != 6) return;
    char* end = nullptr;
    const unsigned long color = std::strtoul(value.c_str(), &end, 16);
    if (end == value.c_str() || *end != '\0') return;
    if (document_.backColor == color) return;
    pushUndo();
    document_.backColor = static_cast<unsigned int>(color);
    dirty_ = true;
    status_ = "updated";
    return;
  }
  if (row == 6) setOptional(document_.setbackTo);
  else if (row == 8) setOptional(document_.defaults.head);
  else if (row == 9) setOptional(document_.defaults.body);
  else if (row >= 10 && row <= 12) {
    const int index = row - 9;
    auto entry = document_.defaults.attrs.find(index);
    if (value.empty()) {
      if (entry == document_.defaults.attrs.end()) return;
      pushUndo();
      document_.defaults.attrs.erase(index);
    } else {
      if (entry != document_.defaults.attrs.end() && entry->second == value) return;
      pushUndo();
      document_.defaults.attrs[index] = value;
    }
    dirty_ = true;
    status_ = "updated";
  } else if (row >= 13 && row <= 15) {
    const int index = row - 12;
    auto entry = document_.defaults.params.find(index);
    if (value.empty()) {
      if (entry == document_.defaults.params.end()) return;
      pushUndo();
      document_.defaults.params.erase(index);
    } else {
      if (entry != document_.defaults.params.end() && entry->second == value) return;
      pushUndo();
      document_.defaults.params[index] = value;
    }
    dirty_ = true;
    status_ = "updated";
  }
}

void Editor::commitPanelEdit() {
  if (panelEditTarget_ == 0) return;
  if (panelEditTarget_ == 2 || panelEditTarget_ == 3) {
    gani::FrameSprite* part = selectedFrameSprite();
    if (part) {
      char* end = nullptr;
      const long parsed = std::strtol(panelEditText_.c_str(), &end, 10);
      if (end != panelEditText_.c_str() && *end == '\0') {
        int& target = panelEditTarget_ == 2 ? part->dx : part->dy;
        const int value = static_cast<int>(std::clamp(parsed, -32768L, 32767L));
        if (target != value) {
          pushUndo();
          target = value;
          dirty_ = true;
          status_ = "updated";
        }
      }
    }
  } else {
    setPanelValue(panelEditRow_, panelEditText_);
  }
  cancelPanelEdit();
}

void Editor::drawPanelEditor(const std::filesystem::path& assets) {
  if (panelEditTarget_ == 0) return;
  SDL_FRect box{};
  if (panelEditTarget_ == 2) box = SDL_FRect{429.0f + panelShift_, 147.0f, 34.0f, 22.0f};
  else if (panelEditTarget_ == 3) box = SDL_FRect{489.0f + panelShift_, 147.0f, 34.0f, 22.0f};
  else if (document_.movie) box = SDL_FRect{455.0f + panelShift_, 130.0f + 16.0f * panelEditRow_, 71.0f, 15.0f};
  else box = SDL_FRect{455.0f + panelShift_, kRowTop + 16.0f * panelEditRow_ + 1.0f, 72.0f, 15.0f};
  fill(renderer_, box, SDL_Color{255, 255, 255, 255});
  outline(renderer_, box, SDL_Color{105, 105, 105, 255});
  ensureFont(g_regular, renderer_, assets, false, 13.0f);
  const float textWidth = g_regular.font.width(panelEditText_);
  const float maxWidth = std::max(1.0f, box.w - 5.0f);
  const float offset = std::max(0.0f, textWidth - maxWidth);
  const SDL_Rect clip{static_cast<int>(box.x + 2.0f), static_cast<int>(box.y + 1.0f),
                      static_cast<int>(box.w - 4.0f), static_cast<int>(box.h - 3.0f)};
  SDL_SetRenderClipRect(renderer_, &clip);
  setColor(renderer_, kText);
  drawText(renderer_, box.x + 2.0f - offset, box.y + 1.0f, panelEditText_, assets);
  if ((SDL_GetTicks() / 500) % 2 == 0) {
    const float caretX = box.x + 2.0f - offset + textWidth;
    SDL_RenderLine(renderer_, caretX, box.y + 2.0f, caretX, box.y + box.h - 3.0f);
  }
  SDL_SetRenderClipRect(renderer_, nullptr);
}

Editor::Layout Editor::layout(int width, int height) const {
  Layout result;
  const float panelLeft = std::max(376.0f, static_cast<float>(width) - 162.0f);
  const float barTop = static_cast<float>(height) - 41.0f;
  const float timelineTop = barTop - 48.0f;
  const float contentHeight = std::max(0.0f, timelineTop - 35.0f);
  const float canvasRight = std::max(69.0f, panelLeft - 10.0f);
  result.panelLeft = panelLeft;
  result.strip = SDL_FRect{19.0f, 35.0f, 50.0f, contentHeight};
  result.canvas = SDL_FRect{69.0f, 35.0f, canvasRight - 69.0f, contentHeight};
  result.timeline = SDL_FRect{69.0f, timelineTop, canvasRight - 69.0f, 46.0f};
  result.bar = SDL_FRect{0.0f, barTop, panelLeft, 41.0f};
  return result;
}

int Editor::stripContentHeight() const {
  int total = kStripMargin;
  for (const gani::Sprite& sprite : document_.sprites) total += sprite.height + kStripGap;
  return total;
}

int Editor::stripMaxScroll() const {
  const int visible = view_.strip.h > 0.0f ? static_cast<int>(view_.strip.h) : static_cast<int>(kStripHeight);
  return std::max(0, stripContentHeight() - visible);
}

float Editor::originX(const SDL_FRect& canvas) const {
  return canvas.x + std::floor(canvas.w / 2.0f) + panX_ - 24.0f * zoom_;
}

float Editor::originY(const SDL_FRect& canvas) const {
  return canvas.y + std::floor(canvas.h / 2.0f) + panY_ - 16.0f * zoom_;
}

gani::Frame* Editor::currentFrame() {
  if (frame_ >= document_.frames.size()) return nullptr;
  return &document_.frames[frame_];
}

gani::FrameSprite* Editor::hitTest(float x, float y, const Layout& layout, int& spriteIndex, int& partIndex) {
  gani::Frame* frame = currentFrame();
  if (!frame || direction_ < 0 || static_cast<std::size_t>(direction_) >= frame->rows.size()) return nullptr;
  const float baseX = originX(layout.canvas);
  const float baseY = originY(layout.canvas);
  auto& row = frame->rows[static_cast<std::size_t>(direction_)];
  for (std::size_t index = row.size(); index-- > 0;) {
    gani::FrameSprite& part = row[index];
    const gani::Sprite* sprite = part.sprite >= 0 ? findSprite(document_, part.sprite) : nullptr;
    if (!sprite) continue;
    const SDL_FRect rect{baseX + static_cast<float>(part.dx) * zoom_, baseY + static_cast<float>(part.dy) * zoom_,
                         static_cast<float>(sprite->width) * zoom_, static_cast<float>(sprite->height) * zoom_};
    if (!inside(rect, x, y)) continue;
    for (std::size_t candidate = 0; candidate < document_.sprites.size(); ++candidate)
      if (document_.sprites[candidate].id == sprite->id) spriteIndex = static_cast<int>(candidate);
    partIndex = static_cast<int>(index);
    return &part;
  }
  return nullptr;
}

void Editor::pushUndo() {
  undo_.push_back(document_);
  if (undo_.size() > 64) undo_.erase(undo_.begin());
  redo_.clear();
  dirty_ = true;
}

void Editor::undo() {
  if (undo_.empty()) {
    status_ = "nothing to undo";
    return;
  }
  redo_.push_back(document_);
  document_ = std::move(undo_.back());
  undo_.pop_back();
  if (selected_ >= static_cast<int>(document_.sprites.size())) selected_ = static_cast<int>(document_.sprites.size()) - 1;
  if (frame_ >= document_.frames.size()) frame_ = document_.frames.empty() ? 0 : document_.frames.size() - 1;
  status_ = "undo";
  dirty_ = true;
}

void Editor::redo() {
  if (redo_.empty()) {
    status_ = "nothing to redo";
    return;
  }
  undo_.push_back(document_);
  document_ = std::move(redo_.back());
  redo_.pop_back();
  if (selected_ >= static_cast<int>(document_.sprites.size())) selected_ = static_cast<int>(document_.sprites.size()) - 1;
  if (frame_ >= document_.frames.size()) frame_ = document_.frames.empty() ? 0 : document_.frames.size() - 1;
  status_ = "redo";
  dirty_ = true;
}

void Editor::saveAs() {
#ifdef __EMSCRIPTEN__
  saveTo(path_.empty() ? std::filesystem::path("/game/levels/ganis/untitled.gani") : path_);
#else
  static const SDL_DialogFileFilter filters[] = {{"Graal animations", "gani"}};
  SDL_ShowSaveFileDialog(
      [](void* userdata, const char* const* files, int) {
        if (files && files[0]) static_cast<Editor*>(userdata)->saveTo(files[0]);
      },
      this, window_, filters, 1, nullptr);
#endif
}

std::vector<int> Editor::rowSpriteIndices() const {
  std::vector<int> result;
  if (frame_ >= document_.frames.size()) return result;
  const gani::Frame& frame = document_.frames[frame_];
  if (direction_ < 0 || static_cast<std::size_t>(direction_) >= frame.rows.size()) return result;
  for (const gani::FrameSprite& part : frame.rows[static_cast<std::size_t>(direction_)]) {
    if (part.sprite < 0) continue;
    bool seen = false;
    for (const int index : result)
      if (document_.sprites[static_cast<std::size_t>(index)].id == part.sprite) {
        seen = true;
        break;
      }
    if (seen) continue;
    for (std::size_t index = 0; index < document_.sprites.size(); ++index)
      if (document_.sprites[index].id == part.sprite) {
        result.push_back(static_cast<int>(index));
        break;
      }
  }
  return result;
}

void Editor::addSelectedToFrame() {
  if (selected_ < 0 || selected_ >= static_cast<int>(document_.sprites.size())) return;
  gani::Frame* frame = currentFrame();
  if (!frame || direction_ < 0 || static_cast<std::size_t>(direction_) >= frame->rows.size()) return;
  auto& row = frame->rows[static_cast<std::size_t>(direction_)];
  for (const gani::FrameSprite& part : row)
    if (part.sprite == document_.sprites[static_cast<std::size_t>(selected_)].id) return;
  pushUndo();
  gani::FrameSprite part;
  part.sprite = document_.sprites[static_cast<std::size_t>(selected_)].id;
  part.dx = 12;
  part.dy = 34;
  row.push_back(part);
  status_ = "sprite added to frame";
}

void Editor::removeSelectedFromFrame() {
  gani::Frame* frame = currentFrame();
  if (!frame || selected_ < 0 || selected_ >= static_cast<int>(document_.sprites.size())) return;
  if (direction_ < 0 || static_cast<std::size_t>(direction_) >= frame->rows.size()) return;
  const int id = document_.sprites[static_cast<std::size_t>(selected_)].id;
  auto& row = frame->rows[static_cast<std::size_t>(direction_)];
  for (std::size_t index = row.size(); index-- > 0;) {
    if (row[index].sprite != id) continue;
    pushUndo();
    row.erase(row.begin() + static_cast<std::ptrdiff_t>(index));
    status_ = "sprite removed from frame";
    return;
  }
}

void Editor::nudgeSelected(int dx, int dy) {
  gani::Frame* frame = currentFrame();
  if (!frame || selected_ < 0 || selected_ >= static_cast<int>(document_.sprites.size())) return;
  if (direction_ < 0 || static_cast<std::size_t>(direction_) >= frame->rows.size()) return;
  const int id = document_.sprites[static_cast<std::size_t>(selected_)].id;
  auto& row = frame->rows[static_cast<std::size_t>(direction_)];
  for (gani::FrameSprite& part : row)
    if (part.sprite == id) {
      pushUndo();
      part.dx += dx;
      part.dy += dy;
      status_ = "sprite moved";
      return;
    }
}

int Editor::toolbarCellAt(float x, float y) const {
  if (y < 4.0f || y >= 29.0f) return -1;
  for (int index = 0; index < 12; ++index) {
    if (index == 4 || index == 7) continue;
    if (x >= kToolbarCells[index] && x < kToolbarCells[index] + 25.0f) return index;
  }
  return -1;
}

void Editor::addSpriteToFrame(int spriteIndex) { addSpriteToFrameAt(spriteIndex, 8, 8); }

void Editor::addSpriteToFrameAt(int spriteIndex, int dx, int dy) {
  if (spriteIndex < 0 || spriteIndex >= static_cast<int>(document_.sprites.size())) return;
  gani::Frame* frame = currentFrame();
  if (!frame || direction_ < 0 || static_cast<std::size_t>(direction_) >= frame->rows.size()) return;
  pushUndo();
  gani::FrameSprite part;
  part.sprite = document_.sprites[static_cast<std::size_t>(spriteIndex)].id;
  part.dx = dx;
  part.dy = dy;
  frame->rows[static_cast<std::size_t>(direction_)].push_back(part);
  selected_ = spriteIndex;
  status_ = "sprite added";
}

void Editor::removeSelected() {
  gani::Frame* frame = currentFrame();
  if (!frame || selected_ < 0 || selected_ >= static_cast<int>(document_.sprites.size())) return;
  if (direction_ < 0 || static_cast<std::size_t>(direction_) >= frame->rows.size()) return;
  const int id = document_.sprites[static_cast<std::size_t>(selected_)].id;
  auto& row = frame->rows[static_cast<std::size_t>(direction_)];
  for (std::size_t index = row.size(); index-- > 0;) {
    if (row[index].sprite != id) continue;
    pushUndo();
    row.erase(row.begin() + static_cast<std::ptrdiff_t>(index));
    status_ = "sprite removed";
    return;
  }
}

void Editor::insertFrame() {
  pushUndo();
  const std::size_t rows = document_.singleDirection ? 1 : 4;
  gani::Frame frame;
  frame.rows.resize(rows);
  const std::size_t position = document_.frames.empty() ? 0 : std::min(frame_ + 1, document_.frames.size());
  document_.frames.insert(document_.frames.begin() + static_cast<std::ptrdiff_t>(position), std::move(frame));
  frame_ = position;
  status_ = "frame added";
}

void Editor::duplicateFrame() {
  if (document_.frames.empty()) return;
  pushUndo();
  const std::size_t position = std::min(frame_ + 1, document_.frames.size());
  document_.frames.insert(document_.frames.begin() + static_cast<std::ptrdiff_t>(position), document_.frames[frame_]);
  frame_ = position;
  status_ = "frame duplicated";
}

void Editor::deleteFrame() {
  if (document_.frames.size() <= 1) return;
  pushUndo();
  document_.frames.erase(document_.frames.begin() + static_cast<std::ptrdiff_t>(frame_));
  if (frame_ >= document_.frames.size()) frame_ = document_.frames.size() - 1;
  status_ = "frame deleted";
}

void Editor::update(float deltaSeconds) {
  if (document_.movie) {
    if (!playing_) return;
    const int count = std::max(1, document_.frameCount.value_or(40));
    elapsed_ += deltaSeconds;
    while (elapsed_ >= 0.05f) {
      elapsed_ -= 0.05f;
      if (static_cast<int>(frame_) + 1 < count) {
        ++frame_;
      } else if (document_.loop || document_.continuous) {
        frame_ = 0;
      } else {
        playing_ = false;
        elapsed_ = 0.0f;
        break;
      }
    }
    return;
  }
  if (!playing_ || document_.frames.empty()) return;
  elapsed_ += deltaSeconds;
  for (;;) {
    const std::size_t index = std::min(frame_, document_.frames.size() - 1);
    const float duration = 0.05f * static_cast<float>(std::max(1, document_.frames[index].waitRepeats + 1));
    if (elapsed_ < duration) break;
    elapsed_ -= duration;
    if (frame_ + 1 < document_.frames.size()) {
      ++frame_;
    } else if (document_.loop || document_.continuous) {
      frame_ = 0;
    } else {
      playing_ = false;
      elapsed_ = 0.0f;
      break;
    }
  }
}

void Editor::drawStrip(const Layout& layout, const std::filesystem::path& assets) {
  (void)assets;
  fill(renderer_, layout.strip, SDL_Color{0, 192, 0, 255});
  stripScroll_ = std::clamp(stripScroll_, 0, stripMaxScroll());
  const SDL_Rect clip{static_cast<int>(layout.strip.x), static_cast<int>(layout.strip.y),
                      static_cast<int>(layout.strip.w), static_cast<int>(layout.strip.h)};
  SDL_SetRenderClipRect(renderer_, &clip);
  const int stripTop = static_cast<int>(layout.strip.y);
  const int stripBottom = static_cast<int>(layout.strip.y + layout.strip.h);
  int y = stripTop + kStripMargin - stripScroll_;
  for (std::size_t index = 0; index < document_.sprites.size(); ++index) {
    const gani::Sprite& sprite = document_.sprites[index];
    const int height = sprite.height;
    if (y + height > stripTop && y < stripBottom) {
      const std::filesystem::path sheet = resolveSpriteSheet(gameRoot_, path_, document_, sprite);
      SDL_Texture* texture = sheet.empty() ? nullptr : sheets_.get(sheet, isBodySprite(sprite));
      if (texture) {
        const SDL_FRect source{static_cast<float>(sprite.x), static_cast<float>(sprite.y),
                               static_cast<float>(sprite.width), static_cast<float>(sprite.height)};
        const SDL_FRect target{static_cast<float>(static_cast<int>(kStripLeft) +
                                                  (static_cast<int>(kStripWidth) - sprite.width) / 2),
                               static_cast<float>(y), static_cast<float>(sprite.width),
                               static_cast<float>(sprite.height)};
        SDL_RenderTexture(renderer_, texture, &source, &target);
      }
    }
    y += height + kStripGap;
  }
  SDL_SetRenderClipRect(renderer_, nullptr);
  const float trackHeight = std::max(kThumbHeight + 1.0f, layout.strip.h - 34.0f);
  if (!checkerTexture_) {
    Uint32 pixels[17 * 2];
    for (int row = 0; row < 2; ++row)
      for (int x = 0; x < 17; ++x)
        pixels[row * 17 + x] = ((x + row) % 2 == 0) ? 0xFFFFFFFFu : 0xFFF0F0F0u;
    checkerTexture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 17, 2);
    if (checkerTexture_) {
      SDL_UpdateTexture(checkerTexture_, nullptr, pixels, 17 * 4);
      SDL_SetTextureBlendMode(checkerTexture_, SDL_BLENDMODE_NONE);
    }
  }
  if (checkerTexture_) {
    for (float ty = 0.0f; ty < trackHeight; ty += 2.0f) {
      const SDL_FRect source{0.0f, 0.0f, 17.0f, std::min(2.0f, trackHeight - ty)};
      const SDL_FRect target{2.0f, 52.0f + ty, 17.0f, source.h};
      SDL_RenderTexture(renderer_, checkerTexture_, &source, &target);
    }
  }
  const int maximum = stripMaxScroll();
  const float fraction = maximum > 0 ? static_cast<float>(stripScroll_) / static_cast<float>(maximum) : 0.0f;
  const SDL_FRect thumb{2.0f, std::floor(52.0f + fraction * (trackHeight - kThumbHeight)), 17.0f, kThumbHeight};
  fill(renderer_, thumb, kForm);
  hLine(renderer_, thumb.x, thumb.x + thumb.w - 1.0f, thumb.y, SDL_Color{227, 227, 227, 255});
  vLine(renderer_, thumb.x, thumb.y, thumb.y + thumb.h - 1.0f, SDL_Color{227, 227, 227, 255});
  hLine(renderer_, thumb.x + 1.0f, thumb.x + thumb.w - 2.0f, thumb.y + 1.0f, SDL_Color{255, 255, 255, 255});
  vLine(renderer_, thumb.x + 1.0f, thumb.y + 1.0f, thumb.y + thumb.h - 2.0f, SDL_Color{255, 255, 255, 255});
  hLine(renderer_, thumb.x + 1.0f, thumb.x + thumb.w - 2.0f, thumb.y + thumb.h - 2.0f, SDL_Color{160, 160, 160, 255});
  vLine(renderer_, thumb.x + thumb.w - 2.0f, thumb.y + 1.0f, thumb.y + thumb.h - 2.0f, SDL_Color{160, 160, 160, 255});
  hLine(renderer_, thumb.x, thumb.x + thumb.w - 1.0f, thumb.y + thumb.h - 1.0f, SDL_Color{105, 105, 105, 255});
  vLine(renderer_, thumb.x + thumb.w - 1.0f, thumb.y, thumb.y + thumb.h - 1.0f, SDL_Color{105, 105, 105, 255});
}

void Editor::drawCanvas(const Layout& layout, const std::filesystem::path& assets) {
  (void)assets;
  const SDL_Color canvasColor{static_cast<Uint8>((document_.backColor >> 16) & 0xFF),
                              static_cast<Uint8>((document_.backColor >> 8) & 0xFF),
                              static_cast<Uint8>(document_.backColor & 0xFF), 255};
  fill(renderer_, layout.canvas, canvasColor);
  const SDL_Rect clip{static_cast<int>(layout.canvas.x), static_cast<int>(layout.canvas.y),
                      static_cast<int>(layout.canvas.w), static_cast<int>(layout.canvas.h)};
  SDL_SetRenderClipRect(renderer_, &clip);
  const float baseX = originX(layout.canvas);
  const float baseY = originY(layout.canvas);
  setColor(renderer_, kGrid);
  for (int y = static_cast<int>(layout.canvas.y); y < static_cast<int>(layout.canvas.y + layout.canvas.h); ++y)
    if ((y - static_cast<int>(layout.canvas.y)) % 6 < 3) SDL_RenderPoint(renderer_, baseX, static_cast<float>(y));
  for (int x = static_cast<int>(layout.canvas.x); x < static_cast<int>(layout.canvas.x + layout.canvas.w); ++x)
    if ((x - static_cast<int>(layout.canvas.x)) % 6 < 3) SDL_RenderPoint(renderer_, static_cast<float>(x), baseY);
  const float cell = 48.0f * zoom_;
  for (float y = baseY; y <= baseY + cell; y += 1.0f) {
    if (y < layout.canvas.y || y >= layout.canvas.y + layout.canvas.h) continue;
    const int offset = static_cast<int>((y - baseY) / zoom_);
    if (offset % 6 < 3) SDL_RenderPoint(renderer_, baseX + cell, y);
  }
  for (float x = baseX + 1.0f; x <= baseX + cell; x += 1.0f) {
    if (x < layout.canvas.x || x >= layout.canvas.x + layout.canvas.w) continue;
    const int offset = static_cast<int>((x - baseX) / zoom_);
    if ((offset + 2) % 6 < 3) SDL_RenderPoint(renderer_, x, baseY + cell);
  }
  if (document_.movie) {
    for (const gani::MovieActor& actor : document_.movie->actors) {
      const gani::MovieKeyframe* keyframe = nearestKeyframe(actor, static_cast<int>(frame_));
      if (!keyframe) continue;
      const int dx = keyframeInt(*keyframe, "dx", 0);
      const int dy = keyframeInt(*keyframe, "dy", 0);
      if (actor.type == "SPRITE") {
        const int id = keyframeInt(*keyframe, "sprite #", -1);
        const gani::Sprite* sprite = id >= 0 ? findSprite(document_, id) : nullptr;
        if (!sprite) continue;
        const std::filesystem::path sheet = resolveSpriteSheet(gameRoot_, path_, document_, *sprite);
        SDL_Texture* texture = sheet.empty() ? nullptr : sheets_.get(sheet, isBodySprite(*sprite));
        if (!texture) continue;
        const SDL_FRect source{static_cast<float>(sprite->x), static_cast<float>(sprite->y),
                               static_cast<float>(sprite->width), static_cast<float>(sprite->height)};
        const SDL_FRect target{baseX + static_cast<float>(dx) * zoom_, baseY + static_cast<float>(dy) * zoom_,
                               static_cast<float>(sprite->width) * zoom_,
                               static_cast<float>(sprite->height) * zoom_};
        SDL_RenderTexture(renderer_, texture, &source, &target);
      } else if (actor.type == "TEXT") {
        std::string value;
        if (editingText_ && selectedActor_ >= 0 &&
            selectedActor_ < static_cast<int>(document_.movie->actors.size()) &&
            &actor == &document_.movie->actors[static_cast<std::size_t>(selectedActor_)])
          value = textBuffer_;
        else {
          const auto text = keyframe->values.find("text");
          if (text == keyframe->values.end()) continue;
          value = text->second;
        }
        setColor(renderer_, kText);
        drawText(renderer_, baseX + static_cast<float>(dx) * zoom_, baseY + static_cast<float>(dy) * zoom_, value,
                 assets);
      }
    }
    SDL_SetRenderClipRect(renderer_, nullptr);
    return;
  }
  gani::Frame* frame = currentFrame();
  if (frame && direction_ >= 0 && static_cast<std::size_t>(direction_) < frame->rows.size()) {
    for (const gani::FrameSprite& part : frame->rows[static_cast<std::size_t>(direction_)]) {
      const gani::Sprite* sprite = part.sprite >= 0 ? findSprite(document_, part.sprite) : nullptr;
      if (!sprite) continue;
      const std::filesystem::path sheet = resolveSpriteSheet(gameRoot_, path_, document_, *sprite);
      SDL_Texture* texture = sheet.empty() ? nullptr : sheets_.get(sheet, isBodySprite(*sprite));
      if (!texture) continue;
      const SDL_FRect source{static_cast<float>(sprite->x), static_cast<float>(sprite->y),
                             static_cast<float>(sprite->width), static_cast<float>(sprite->height)};
      const SDL_FRect target{baseX + static_cast<float>(part.dx) * zoom_,
                             baseY + static_cast<float>(part.dy) * zoom_,
                             static_cast<float>(sprite->width) * zoom_,
                             static_cast<float>(sprite->height) * zoom_};
      SDL_RenderTexture(renderer_, texture, &source, &target);
    }
  }
  SDL_SetRenderClipRect(renderer_, nullptr);
}

void Editor::drawPanel(const Layout& layout, const std::filesystem::path& assets) {
  (void)layout;
  const gani::Sprite* selected = selected_ >= 0 && selected_ < static_cast<int>(document_.sprites.size())
                                     ? &document_.sprites[static_cast<std::size_t>(selected_)]
                                     : nullptr;
  const std::string direction =
      direction_ == 0 ? "up" : direction_ == 1 ? "left" : direction_ == 2 ? "down" : "right";
  int dx = 0;
  int dy = 0;
  bool present = false;
  if (selected) {
    if (gani::Frame* frame = currentFrame(); frame && direction_ >= 0 &&
        static_cast<std::size_t>(direction_) < frame->rows.size())
      for (const gani::FrameSprite& part : frame->rows[static_cast<std::size_t>(direction_)])
        if (part.sprite == selected->id) {
          dx = part.dx;
          dy = part.dy;
          present = true;
          break;
        }
  }
  setColor(renderer_, kText);
  drawText(renderer_, 379.0f, 59.0f, direction, assets);
  if (selected) {
    SDL_SetRenderClipRect(renderer_, nullptr);
    drawText(renderer_, 380.0f, 104.0f, selected->name.empty() ? selected->type : selected->name, assets);
  }
  if (present) {
    drawText(renderer_, 431.0f, 154.0f, std::to_string(dx), assets);
    drawText(renderer_, 491.0f, 154.0f, std::to_string(dy), assets);
  }
  if (framePanel_) {
    fill(renderer_, SDL_FRect{382.0f, 199.0f, 70.0f, 14.0f}, kForm);
    setColor(renderer_, kText);
    drawText(renderer_, 384.0f, 201.0f, "Length", assets);
  }
  const SDL_Rect clip{454, 197, 72, 256};
  SDL_SetRenderClipRect(renderer_, &clip);
  setColor(renderer_, kText);
  for (int row = 0; row < 16; ++row)
    drawText(renderer_, 455.0f, kRowTop + static_cast<float>(row) * kRowHeight + 2.0f, spritePanelValue(row),
             assets);
  SDL_SetRenderClipRect(renderer_, nullptr);
}

void Editor::ensureTimeFont() {
  if (timeFont_) return;
  constexpr int count = 16;
  SDL_Surface* surface = SDL_CreateSurface(kTimeGlyphWidth * count, kTimeGlyphHeight, SDL_PIXELFORMAT_ARGB8888);
  if (!surface) return;
  auto* pixels = static_cast<Uint32*>(surface->pixels);
  const int pitch = surface->pitch / 4;
  for (int glyph = 0; glyph < count; ++glyph)
    for (int row = 0; row < kTimeGlyphHeight; ++row)
      for (int column = 0; column < kTimeGlyphWidth; ++column)
        pixels[row * pitch + glyph * kTimeGlyphWidth + column] =
            (kTimeGlyphs[glyph][row] & (1u << column)) ? 0xFFFFFFFFu : 0x00000000u;
  timeFont_ = SDL_CreateTextureFromSurface(renderer_, surface);
  SDL_DestroySurface(surface);
  if (timeFont_) {
    SDL_SetTextureBlendMode(timeFont_, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(timeFont_, SDL_SCALEMODE_NEAREST);
  }
}

void Editor::ensureTickFont() {
  if (tickFont_) return;
  constexpr int count = 11;
  SDL_Surface* surface = SDL_CreateSurface(kTickGlyphWidth * count, kTickGlyphHeight, SDL_PIXELFORMAT_ARGB8888);
  if (!surface) return;
  auto* pixels = static_cast<Uint32*>(surface->pixels);
  const int pitch = surface->pitch / 4;
  for (int glyph = 0; glyph < count; ++glyph)
    for (int row = 0; row < kTickGlyphHeight; ++row)
      for (int column = 0; column < kTickGlyphWidth; ++column)
        pixels[row * pitch + glyph * kTickGlyphWidth + column] =
            (kTickGlyphs[glyph][row] & (1u << column)) ? 0xFFFFFFFFu : 0x00000000u;
  tickFont_ = SDL_CreateTextureFromSurface(renderer_, surface);
  SDL_DestroySurface(surface);
  if (tickFont_) {
    SDL_SetTextureBlendMode(tickFont_, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(tickFont_, SDL_SCALEMODE_NEAREST);
  }
}

void Editor::drawTickString(float x, float y, const std::string& text) {
  ensureTickFont();
  if (!tickFont_) return;
  static const std::string charset = "0123456789.";
  SDL_SetTextureColorMod(tickFont_, 0, 0, 0);
  for (const char character : text) {
    const std::size_t index = charset.find(character);
    if (index != std::string::npos) {
      const SDL_FRect source{static_cast<float>(index * kTickGlyphWidth), 0.0f, static_cast<float>(kTickGlyphWidth),
                             static_cast<float>(kTickGlyphHeight)};
      const SDL_FRect target{x, y, static_cast<float>(kTickGlyphWidth), static_cast<float>(kTickGlyphHeight)};
      SDL_RenderTexture(renderer_, tickFont_, &source, &target);
    }
    x += static_cast<float>(kTickGlyphWidth);
  }
  SDL_SetTextureColorMod(tickFont_, 255, 255, 255);
}

void Editor::drawTimeString(float x, float y, const std::string& text) {
  ensureTimeFont();
  if (!timeFont_) return;
  static const std::string charset = "0123456789.Ttime";
  SDL_SetTextureColorMod(timeFont_, 0, 0, 0);
  for (const char character : text) {
    const std::size_t index = charset.find(character);
    if (index != std::string::npos) {
      const SDL_FRect source{static_cast<float>(index * kTimeGlyphWidth), 0.0f, static_cast<float>(kTimeGlyphWidth),
                             static_cast<float>(kTimeGlyphHeight)};
      const SDL_FRect target{x, y, static_cast<float>(kTimeGlyphWidth), static_cast<float>(kTimeGlyphHeight)};
      SDL_RenderTexture(renderer_, timeFont_, &source, &target);
    }
    x += static_cast<float>(kTimeGlyphWidth);
  }
  SDL_SetTextureColorMod(timeFont_, 255, 255, 255);
}

void Editor::drawTimeline(const Layout& layout, const std::filesystem::path& assets) {
  (void)assets;
  const float left = layout.timeline.x;
  const float width = layout.timeline.w;
  const float top = layout.timeline.y;
  if (document_.movie) {
    fill(renderer_, layout.timeline, SDL_Color{0, 192, 0, 255});
    const int length = movieLengthSeconds();
    const float span = static_cast<float>(length) * 1.2f;
    const float rulerLeft = left + 12.0f;
    const float rulerRight = left + width - 15.0f;
    const float rulerWidth = std::max(1.0f, rulerRight - rulerLeft);
    const float pxPerSecond = rulerWidth / span;
    float minorStep = 0.625f;
    if (pxPerSecond >= 15.0f) minorStep = 0.25f;
    else if (pxPerSecond >= 8.0f) minorStep = 0.5f;
    const float majorStep = pxPerSecond >= 8.0f ? 1.0f : 2.5f;
    const SDL_Color black{0, 0, 0, 255};
    fill(renderer_, SDL_FRect{rulerLeft, top + 29.0f, rulerWidth + 1.0f, 2.0f}, black);
    for (float tick = 0.0f; tick <= span + 0.001f; tick += minorStep) {
      const float x = rulerLeft + tick * pxPerSecond;
      const bool major = std::fabs(std::fmod(tick + 0.0001f, majorStep)) < 0.001f;
      if (major) fill(renderer_, SDL_FRect{x, top + 20.0f, 4.0f, 10.0f}, black);
      else fill(renderer_, SDL_FRect{x, top + 23.0f, 2.0f, 7.0f}, black);
    }
    const int step = std::max(1, static_cast<int>(std::ceil(2.5f / pxPerSecond)));
    for (int second = 0; second <= static_cast<int>(span + 0.001f); second += step) {
      const std::string label = std::to_string(second);
      const int textWidth = kTickGlyphWidth * static_cast<int>(label.size());
      float x = static_cast<float>(std::lround(rulerLeft + static_cast<float>(second) * pxPerSecond - textWidth / 2.0f));
      if (label.size() == 1) x += 1.0f;
      drawTickString(x, top + 31.0f, label);
    }
    const float lengthX = rulerLeft + (static_cast<float>(length) / span) * rulerWidth;
    fill(renderer_, SDL_FRect{lengthX - 4.0f, top + 17.0f, 9.0f, 9.0f}, SDL_Color{255, 0, 0, 255});
    const float frameX = rulerLeft + (static_cast<float>(frame_) / 20.0f / span) * rulerWidth;
    rightTriangle(renderer_, frameX - 3.0f, top + 17.0f, 8.0f, 9.0f, SDL_Color{255, 0, 0, 255},
                  SDL_Color{0, 0, 0, 255});
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%.2f", static_cast<double>(frame_) * 0.05);
    setColor(renderer_, kText);
    drawTimeString(22.0f, top + 25.0f, buffer);
    return;
  }
  const int frameCount = std::max(1, static_cast<int>(document_.frames.size()));
  std::vector<float> starts(static_cast<std::size_t>(frameCount), 0.0f);
  float total = 0.0f;
  for (int index = 0; index < frameCount; ++index) {
    starts[static_cast<std::size_t>(index)] = total;
    const int wait = index < static_cast<int>(document_.frames.size())
                         ? std::max(1, document_.frames[static_cast<std::size_t>(index)].waitRepeats + 1)
                         : 1;
    total += 0.05f * static_cast<float>(wait);
  }
  const float span = total + 0.05f;
  const float rulerLeft = left + 12.0f;
  const float rulerRight = left + width - 15.0f;
  const float rulerWidth = std::max(1.0f, rulerRight - rulerLeft);
  const float pxPerSecond = rulerWidth / std::max(0.0001f, span);
  const float tickPixels = 0.05f * pxPerSecond;
  const SDL_Color black{0, 0, 0, 255};
  fill(renderer_, layout.timeline, SDL_Color{0, 192, 0, 255});
  fill(renderer_, SDL_FRect{rulerLeft, top + 29.0f, rulerWidth, 2.0f}, black);
  for (float tick = 0.0f; tick <= span + 0.001f; tick += 0.05f) {
    const float x = rulerLeft + tick * pxPerSecond;
    fill(renderer_, SDL_FRect{tick == 0.0f ? x : x - 1.0f, top + 27.0f, 2.0f, 4.0f}, black);
  }
  for (int index = 0; index < frameCount; ++index) {
    const float x0 = rulerLeft + starts[static_cast<std::size_t>(index)] * pxPerSecond;
    const float x1 = index + 1 < frameCount ? rulerLeft + starts[static_cast<std::size_t>(index + 1)] * pxPerSecond
                                             : rulerLeft + total * pxPerSecond;
    if (x1 - x0 < 3.0f) continue;
    auto roundedBar = [&](float x, float y, float w, float h, SDL_Color color, float corner, bool shoulder) {
      for (int row = 0; row < static_cast<int>(h); ++row) {
        float inset = 0.0f;
        if (row == 0 || row == static_cast<int>(h) - 1) inset = corner;
        else if (row == 1 || row == static_cast<int>(h) - 2) inset = 1.0f;
        else if (shoulder && (row == 2 || row == static_cast<int>(h) - 3)) inset = 1.0f;
        if (w - 2.0f * inset > 0.0f) fill(renderer_, SDL_FRect{x + inset, y + row, w - 2.0f * inset, 1.0f}, color);
      }
    };
    roundedBar(x0 + 1.0f, top + 5.0f, x1 - x0 - 2.0f, 10.0f, black, 3.0f, true);
    roundedBar(x0 + 2.0f, top + 6.0f, x1 - x0 - 4.0f, 8.0f, SDL_Color{0, 0, 255, 255}, 2.0f, false);
  }
  const int tickCount = static_cast<int>(std::lround(span / 0.05f));
  int labelStep = frameCount >= 5 ? 5 : 1;
  while (labelStep * tickPixels < 24.0f) labelStep += 5;
  auto drawLabel = [&](float x, const std::string& label) {
    const int textWidth = kTickGlyphWidth * static_cast<int>(label.size());
    x = std::min(x, left + width - 3.0f - textWidth);
    drawTickString(x, top + 31.0f, label);
  };
  for (int tick = 0; tick <= tickCount; tick += labelStep) {
    const float seconds = static_cast<float>(tick) * 0.05f;
    std::string label;
    if (tick == 0) label = "0";
    else {
      char buffer[16];
      std::snprintf(buffer, sizeof buffer, "%.2f", static_cast<double>(seconds));
      label = buffer;
      if (!label.empty() && label[0] == '0') label.erase(0, 1);
    }
    const int textWidth = kTickGlyphWidth * static_cast<int>(label.size());
    float x = static_cast<float>(std::lround(rulerLeft + seconds * pxPerSecond - textWidth / 2.0f));
    if (label.size() == 1) x += 1.0f;
    drawLabel(x, label);
  }
  if (tickCount % labelStep != 0) {
    char buffer[16];
    std::snprintf(buffer, sizeof buffer, "%.2f", static_cast<double>(span));
    std::string label = buffer;
    if (!label.empty() && label[0] == '0') label.erase(0, 1);
    const int textWidth = kTickGlyphWidth * static_cast<int>(label.size());
    drawLabel(static_cast<float>(std::lround(rulerLeft + span * pxPerSecond - textWidth / 2.0f)), label);
  }
  const float endX = rulerLeft + total * pxPerSecond;
  fill(renderer_, SDL_FRect{endX - 5.0f, top + 16.0f, 11.0f, 11.0f}, SDL_Color{0, 0, 0, 255});
  fill(renderer_, SDL_FRect{endX - 4.0f, top + 17.0f, 9.0f, 9.0f}, SDL_Color{255, 0, 0, 255});
  const std::size_t current = std::min<std::size_t>(frame_, static_cast<std::size_t>(frameCount - 1));
  const float currentX = rulerLeft + starts[current] * pxPerSecond;
  static const char* const playhead[] = {
      "BB.........", "BRBB.......", "BRRRBB.....", "BRRRRRBB...", "BRRRRRRRBB.",
      "BRRRRRRRRBB", "BRRRRRRBB..", "BRRRRBB....", "BRRBBB.....", "BBB.BB.....",
      "B...BB.....", "....BB.....", "....BB....."};
  for (int row = 0; row < 13; ++row)
    for (int column = 0; playhead[row][column]; ++column) {
      const char cell = playhead[row][column];
      if (cell == '.') continue;
      fill(renderer_, SDL_FRect{currentX - 4.0f + column, top + 16.0f + row, 1.0f, 1.0f},
           cell == 'R' ? SDL_Color{255, 0, 0, 255} : SDL_Color{0, 0, 0, 255});
    }
  char buffer[32];
  std::snprintf(buffer, sizeof buffer, "%.2f", static_cast<double>(starts[current]));
  setColor(renderer_, kText);
  drawTimeString(22.0f, top + 25.0f, buffer);
}

void Editor::scrubTimeline(float x, const Layout& layout) {
  const float left = layout.timeline.x;
  const float width = layout.timeline.w;
  const float rulerLeft = left + 12.0f;
  const float rulerRight = left + width - 15.0f;
  const float rulerWidth = std::max(1.0f, rulerRight - rulerLeft);
  if (document_.movie) {
    const float span = static_cast<float>(movieLengthSeconds()) * 1.2f;
    const float seconds = std::clamp((x - rulerLeft) / rulerWidth * span, 0.0f,
                                     static_cast<float>(movieLengthSeconds()));
    frame_ = static_cast<std::size_t>(std::lround(seconds * 20.0f));
    const int maximumFrame = std::max(0, document_.frameCount.value_or(1) - 1);
    if (frame_ > static_cast<std::size_t>(maximumFrame)) frame_ = static_cast<std::size_t>(maximumFrame);
    playing_ = false;
    elapsed_ = 0.0f;
    return;
  }
  if (document_.frames.empty()) return;
  float total = 0.0f;
  std::vector<float> starts(document_.frames.size(), 0.0f);
  for (std::size_t index = 0; index < document_.frames.size(); ++index) {
    starts[index] = total;
    total += 0.05f * static_cast<float>(std::max(1, document_.frames[index].waitRepeats + 1));
  }
  const float span = total + 0.05f;
  const float seconds = std::clamp((x - rulerLeft) / rulerWidth * span, 0.0f, total);
  std::size_t target = 0;
  for (std::size_t index = 0; index < document_.frames.size(); ++index) {
    const float next = index + 1 < document_.frames.size() ? starts[index + 1] : total;
    target = index;
    if (seconds < next) break;
  }
  frame_ = target;
  playing_ = false;
  elapsed_ = 0.0f;
}

void Editor::drawUndoRedo(const std::filesystem::path& assets) {
  (void)assets;
  const struct {
    int cell;
    bool enabled;
    const char* glyph;
  } icons[] = {{5, !undo_.empty(), "SpeedButton24"}, {6, !redo_.empty(), "SpeedButton25"}};
  for (const auto& icon : icons) {
    const GlyphInfo* glyph = findGlyph(icon.glyph);
    if (!glyph) continue;
    const float x = kToolbarCells[icon.cell] + static_cast<float>(std::lround((25.0 - glyph->w) / 2.0));
    const float y = 4.0f + static_cast<float>(std::lround((25.0 - glyph->h) / 2.0));
    if (icon.enabled) drawGlyph(icon.glyph, x, y);
    else drawGlyphDisabled(icon.glyph, x, y);
  }
}

void Editor::drawHover(const std::filesystem::path& assets) {
  float mx = 0.0f;
  float my = 0.0f;
  mousePosition(renderer_, mx, my);
  const int cell = toolbarCellAt(mx, my);
  const bool disabled = (cell == 5 && undo_.empty()) || (cell == 6 && redo_.empty());
  if (cell >= 0 && cell != 4 && cell != 7 && !disabled) {
    raised(renderer_, SDL_FRect{kToolbarCells[cell], 4.0f, 25.0f, 25.0f});
    const char* glyph = nullptr;
    switch (cell) {
      case 0: glyph = "SpeedButton34"; break;
      case 1: glyph = "SpeedButton9"; break;
      case 2: glyph = "SpeedButton10"; break;
      case 3: glyph = "SpeedButton11"; break;
      case 8: glyph = "SpeedButton18"; break;
      case 9: glyph = "SpeedButton22"; break;
      case 10: glyph = "SpeedButton26"; break;
      case 11: glyph = "SpeedButton23"; break;
      default: break;
    }
    if (glyph) {
      if (const GlyphInfo* info = findGlyph(glyph))
        drawGlyph(glyph, kToolbarCells[cell] + static_cast<float>(std::lround((25.0 - info->w) / 2.0)),
                  4.0f + static_cast<float>(std::lround((25.0 - info->h) / 2.0)));
    }
  }
  if (pressed_) {
    setColor(renderer_, SDL_Color{105, 105, 105, 255});
    SDL_RenderLine(renderer_, pressedRect_.x, pressedRect_.y, pressedRect_.x + pressedRect_.w - 1.0f, pressedRect_.y);
    SDL_RenderLine(renderer_, pressedRect_.x, pressedRect_.y, pressedRect_.x, pressedRect_.y + pressedRect_.h - 1.0f);
    setColor(renderer_, SDL_Color{255, 255, 255, 255});
    SDL_RenderLine(renderer_, pressedRect_.x, pressedRect_.y + pressedRect_.h - 1.0f,
                   pressedRect_.x + pressedRect_.w - 1.0f, pressedRect_.y + pressedRect_.h - 1.0f);
    SDL_RenderLine(renderer_, pressedRect_.x + pressedRect_.w - 1.0f, pressedRect_.y,
                   pressedRect_.x + pressedRect_.w - 1.0f, pressedRect_.y + pressedRect_.h - 1.0f);
  }
  drawUndoRedo(assets);
}

void Editor::drawPopup(const std::filesystem::path& assets) {
  if (popup_ == Popup::none) return;
  const std::vector<int> sprites = popup_ == Popup::sprites ? rowSpriteIndices() : std::vector<int>{};
  const int count = popup_ == Popup::direction ? 4 : static_cast<int>(sprites.size());
  const int visible = std::min(count, 8);
  if (visible <= 0) {
    popup_ = Popup::none;
    return;
  }
  const float top = popup_ == Popup::direction ? 76.0f : 121.0f;
  int selected = -1;
  if (popup_ == Popup::direction) selected = direction_;
  else
    for (std::size_t index = 0; index < sprites.size(); ++index)
      if (sprites[index] == selected_) {
        selected = static_cast<int>(index);
        break;
      }
  if (selected >= popupScroll_ + visible) popupScroll_ = selected - visible + 1;
  if (selected >= 0 && selected < popupScroll_) popupScroll_ = selected;
  popupScroll_ = std::clamp(popupScroll_, 0, std::max(0, count - visible));
  setColor(renderer_, SDL_Color{255, 255, 255, 255});
  fill(renderer_, popup_ == Popup::direction ? SDL_FRect{377.0f + panelShift_, 56.0f, 133.0f, 19.0f}
                                              : SDL_FRect{378.0f + panelShift_, 101.0f, 133.0f, 19.0f},
       SDL_Color{0, 120, 215, 255});
  setColor(renderer_, SDL_Color{255, 255, 255, 255});
  if (popup_ == Popup::direction)
    drawText(renderer_, 379.0f + panelShift_, 59.0f,
             direction_ == 0 ? "up" : direction_ == 1 ? "left" : direction_ == 2 ? "down" : "right", assets);
  else if (selected_ >= 0 && selected_ < static_cast<int>(document_.sprites.size()))
    drawText(renderer_, 380.0f + panelShift_, 104.0f,
             document_.sprites[static_cast<std::size_t>(selected_)].name.empty()
                 ? document_.sprites[static_cast<std::size_t>(selected_)].type
                 : document_.sprites[static_cast<std::size_t>(selected_)].name,
             assets);
  const SDL_FRect rect{376.0f + panelShift_, top, 155.0f, static_cast<float>(visible * 13 + 2)};
  fill(renderer_, rect, SDL_Color{255, 255, 255, 255});
  outline(renderer_, rect, SDL_Color{100, 100, 100, 255});
  for (int row = 0; row < visible; ++row) {
    const int index = popupScroll_ + row;
    if (index >= count) break;
    const SDL_FRect item{rect.x + 1.0f, rect.y + 1.0f + static_cast<float>(row) * 13.0f, rect.w - 2.0f, 13.0f};
    if (index == selected) fill(renderer_, item, SDL_Color{0, 120, 215, 255});
    std::string label;
    if (popup_ == Popup::direction)
      label = index == 0 ? "up" : index == 1 ? "left" : index == 2 ? "down" : "right";
    else {
      const gani::Sprite& sprite = document_.sprites[static_cast<std::size_t>(sprites[static_cast<std::size_t>(index)])];
      label = sprite.name.empty() ? sprite.type : sprite.name;
    }
    setColor(renderer_, index == selected ? SDL_Color{255, 255, 255, 255} : SDL_Color{0, 0, 0, 255});
    drawText(renderer_, rect.x + 4.0f, rect.y + 1.0f + static_cast<float>(row) * 13.0f + 1.0f, label, assets);
  }
  if (count > visible) {
    const SDL_FRect track{rect.x + rect.w - 11.0f, rect.y + 1.0f, 10.0f, rect.h - 2.0f};
    fill(renderer_, track, SDL_Color{227, 227, 227, 255});
    const float thumbHeight = std::max(10.0f, track.h * static_cast<float>(visible) / static_cast<float>(count));
    const float thumbY = track.y + (track.h - thumbHeight) * static_cast<float>(popupScroll_) /
                                       static_cast<float>(std::max(1, count - visible));
    fill(renderer_, SDL_FRect{track.x, thumbY, track.w, thumbHeight}, SDL_Color{160, 160, 160, 255});
  }
}

void Editor::drawTooltip(const std::filesystem::path& assets) {
  if (popup_ != Popup::none || stripDrag_) {
    tooltipTarget_ = -1;
    return;
  }
  float mx = 0.0f;
  float my = 0.0f;
  mousePosition(renderer_, mx, my);
  int target = -1;
  std::string hint;
  const int cell = toolbarCellAt(mx, my);
  if (cell >= 0) {
    if ((cell == 5 && undo_.empty()) || (cell == 6 && redo_.empty())) {
      tooltipTarget_ = -1;
      return;
    }
    static const char* hints[] = {"New Animation", "Open Animation", "Save Animation", "Save as...", "",
                                  "Undo", "Redo", "", "Add Sprite Definition",
                                  "Remove Unused Sprite Definitions", "Import Sprite Definitions from...", ""};
    hint = hints[cell];
    target = 100 + cell;
  } else if (mx >= view_.strip.x && mx < view_.strip.x + view_.strip.w && my >= view_.strip.y &&
             my < view_.strip.y + view_.strip.h) {
    int cursor = static_cast<int>(view_.strip.y) + kStripMargin - stripScroll_;
    for (std::size_t index = 0; index < document_.sprites.size(); ++index) {
      const gani::Sprite& sprite = document_.sprites[index];
      if (my >= static_cast<float>(cursor) && my < static_cast<float>(cursor + sprite.height)) {
        hint = (sprite.name.empty() ? sprite.type : sprite.name) + "\n# " + std::to_string(sprite.id);
        target = 200 + static_cast<int>(index);
        break;
      }
      cursor += sprite.height + kStripGap;
    }
  }
  if (target < 0 || hint.empty()) {
    tooltipTarget_ = -1;
    return;
  }
  const unsigned long long now = SDL_GetTicks();
  if (target != tooltipTarget_) {
    tooltipTarget_ = target;
    tooltipSince_ = now;
    tooltipText_ = hint;
    return;
  }
  if (now - tooltipSince_ < 700) return;
  ensureFont(g_regular, renderer_, assets, false, 13.0f);
  std::vector<std::string> lines;
  std::string current;
  for (const char character : tooltipText_) {
    if (character == '\n') {
      lines.push_back(current);
      current.clear();
    } else {
      current += character;
    }
  }
  lines.push_back(current);
  float width = 0.0f;
  for (const std::string& line : lines) width = std::max(width, g_regular.font.width(line));
  const float boxWidth = width + 6.0f;
  const float boxHeight = static_cast<float>(lines.size()) * 13.0f + 4.0f;
  const float x = std::min(mx + 12.0f, 538.0f - boxWidth - 2.0f);
  const float y = std::min(my + 18.0f, 453.0f - boxHeight - 2.0f);
  fill(renderer_, SDL_FRect{x, y, boxWidth, boxHeight}, SDL_Color{255, 255, 225, 255});
  outline(renderer_, SDL_FRect{x, y, boxWidth, boxHeight}, SDL_Color{160, 160, 160, 255});
  setColor(renderer_, SDL_Color{0, 0, 0, 255});
  for (std::size_t index = 0; index < lines.size(); ++index)
    drawText(renderer_, x + 3.0f, y + 2.0f + static_cast<float>(index) * 13.0f, lines[index], assets);
}

void Editor::drawMoviePanel(const std::filesystem::path& assets) {
  const SDL_Rect clip{454, 128, 72, 208};
  SDL_SetRenderClipRect(renderer_, &clip);
  setColor(renderer_, kText);
  for (int row = 0; row < 13; ++row)
    drawText(renderer_, 455.0f, 131.0f + static_cast<float>(row) * 16.0f, moviePanelValue(row), assets);
  SDL_SetRenderClipRect(renderer_, nullptr);
}

void Editor::drawModeButton(const std::filesystem::path& assets, float appWidth) {
  const SDL_FRect button{std::max(4.0f, appWidth - 125.0f), 4.0f, 99.0f, 25.0f};
  if (appWidth != 538.0f) fill(renderer_, SDL_FRect{360.0f, 4.0f, appWidth - 360.0f, 25.0f}, kForm);
  fill(renderer_, button, kForm);
  setColor(renderer_, SDL_Color{255, 255, 255, 255});
  SDL_RenderLine(renderer_, button.x, button.y, button.x + button.w - 1.0f, button.y);
  SDL_RenderLine(renderer_, button.x, button.y, button.x, button.y + button.h - 1.0f);
  setColor(renderer_, SDL_Color{227, 227, 227, 255});
  SDL_RenderLine(renderer_, button.x + 1.0f, button.y + 1.0f, button.x + button.w - 2.0f, button.y + 1.0f);
  SDL_RenderLine(renderer_, button.x + 1.0f, button.y + 1.0f, button.x + 1.0f, button.y + button.h - 2.0f);
  setColor(renderer_, SDL_Color{160, 160, 160, 255});
  SDL_RenderLine(renderer_, button.x + 1.0f, button.y + button.h - 2.0f, button.x + button.w - 1.0f,
                 button.y + button.h - 2.0f);
  SDL_RenderLine(renderer_, button.x + button.w - 2.0f, button.y, button.x + button.w - 2.0f,
                 button.y + button.h - 1.0f);
  setColor(renderer_, SDL_Color{105, 105, 105, 255});
  SDL_RenderLine(renderer_, button.x, button.y + button.h - 1.0f, button.x + button.w - 1.0f,
                 button.y + button.h - 1.0f);
  SDL_RenderLine(renderer_, button.x + button.w - 1.0f, button.y, button.x + button.w - 1.0f,
                 button.y + button.h - 1.0f);
  ensureFont(g_regular, renderer_, assets, false, 13.0f);
  const std::string label = document_.movie ? "<- Sprite Mode" : "-> Movie Mode";
  const float width = g_regular.font.ready() ? g_regular.font.width(label) : static_cast<float>(label.size()) * 7.0f;
  setColor(renderer_, kText);
  drawText(renderer_, button.x + (button.w - width) / 2.0f, button.y + 5.0f, label, assets);
}

void Editor::ensureGlyphs() {
  if (glyphTexture_) return;
  glyphTexture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, kGlyphAtlasWidth,
                                    kGlyphAtlasHeight);
  if (!glyphTexture_) return;
  SDL_UpdateTexture(glyphTexture_, nullptr, kGlyphAtlas, kGlyphAtlasWidth * 4);
  SDL_SetTextureBlendMode(glyphTexture_, SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(glyphTexture_, SDL_SCALEMODE_NEAREST);
}

void Editor::drawGlyph(const char* name, float x, float y, Uint8 r, Uint8 g, Uint8 b) {
  const GlyphInfo* glyph = findGlyph(name);
  if (!glyph) return;
  ensureGlyphs();
  if (!glyphTexture_) return;
  const SDL_FRect source{static_cast<float>(glyph->x), static_cast<float>(glyph->y), static_cast<float>(glyph->w),
                         static_cast<float>(glyph->h)};
  const SDL_FRect target{x, y, static_cast<float>(glyph->w), static_cast<float>(glyph->h)};
  SDL_SetTextureColorMod(glyphTexture_, r, g, b);
  SDL_RenderTexture(renderer_, glyphTexture_, &source, &target);
  SDL_SetTextureColorMod(glyphTexture_, 255, 255, 255);
}

void Editor::drawGlyphSilhouette(const char* name, float x, float y, SDL_Color color) {
  const GlyphInfo* glyph = findGlyph(name);
  if (!glyph) return;
  for (int row = 0; row < glyph->h; ++row)
    for (int column = 0; column < glyph->w; ++column) {
      const int index = ((glyph->y + row) * kGlyphAtlasWidth + glyph->x + column) * 4;
      if (kGlyphAtlas[index + 3] == 0) continue;
      fill(renderer_, SDL_FRect{x + column, y + row, 1.0f, 1.0f}, color);
    }
}

void Editor::drawGlyphDisabled(const char* name, float x, float y) {
  const GlyphInfo* glyph = findGlyph(name);
  if (!glyph) return;
  const auto isBlack = [&](int column, int row) {
    if (column < 0 || row < 0 || column >= glyph->w || row >= glyph->h) return false;
    const int index = ((glyph->y + row) * kGlyphAtlasWidth + glyph->x + column) * 4;
    return kGlyphAtlas[index + 3] != 0 && kGlyphAtlas[index] == 0 && kGlyphAtlas[index + 1] == 0 &&
           kGlyphAtlas[index + 2] == 0;
  };
  for (int row = 0; row < glyph->h; ++row)
    for (int column = 0; column < glyph->w; ++column) {
      if (!isBlack(column, row)) continue;
      if (column + 1 < glyph->w && row + 1 < glyph->h && !isBlack(column + 1, row + 1))
        fill(renderer_, SDL_FRect{x + column + 1, y + row + 1, 1.0f, 1.0f}, SDL_Color{255, 255, 255, 255});
    }
  for (int row = 0; row < glyph->h; ++row)
    for (int column = 0; column < glyph->w; ++column)
      if (isBlack(column, row))
        fill(renderer_, SDL_FRect{x + column, y + row, 1.0f, 1.0f}, SDL_Color{160, 160, 160, 255});
}

void Editor::drawToolbarGlyphs() {
  const struct {
    int cell;
    const char* glyph;
  } icons[] = {{0, "SpeedButton34"}, {1, "SpeedButton9"}, {2, "SpeedButton10"}, {3, "SpeedButton11"},
               {8, "SpeedButton18"}, {9, "SpeedButton22"}, {10, "SpeedButton26"}, {11, "SpeedButton23"}};
  for (const auto& icon : icons) {
    const GlyphInfo* glyph = findGlyph(icon.glyph);
    if (!glyph) continue;
    const float cellX = kToolbarCells[icon.cell];
    drawGlyph(icon.glyph, cellX + static_cast<float>(std::lround((25.0 - glyph->w) / 2.0)),
              4.0f + static_cast<float>(std::lround((25.0 - glyph->h) / 2.0)));
  }
}

void Editor::drawPanelStatic() {
  const std::filesystem::path assets = assetDir();
  ensureFont(g_regular, renderer_, assets, false, 13.0f);
  const SDL_Color mid{160, 160, 160, 255};
  const SDL_Color white{255, 255, 255, 255};
  setColor(renderer_, kText);
  drawText(renderer_, 380.0f, 39.0f, "Direction:", assets);
  comboBox(renderer_, SDL_FRect{375.0f, 55.0f, 154.0f, 21.0f});
  outline(renderer_, SDL_FRect{368.0f, 85.0f, 168.0f, 99.0f}, mid);
  hLine(renderer_, 369.0f, 534.0f, 86.0f, white);
  vLine(renderer_, 369.0f, 86.0f, 182.0f, white);
  vLine(renderer_, 536.0f, 86.0f, 184.0f, white);
  hLine(renderer_, 369.0f, 536.0f, 184.0f, white);
  fill(renderer_, SDL_FRect{376.0f, 80.0f, 89.0f, 9.0f}, kForm);
  setColor(renderer_, kText);
  drawText(renderer_, 379.0f, 80.0f, "Selected Sprite(s)", assets);
  comboBox(renderer_, SDL_FRect{376.0f, 100.0f, 154.0f, 21.0f});
  const float buttons[] = {379.0f, 413.0f, 438.0f, 474.0f, 498.0f};
  const char* buttonGlyphs[] = {"SpeedButton19", "SpeedButton2", "SpeedButton4", "SpeedButton1", "SpeedButton3"};
  const float glyphX[] = {385.0f, 419.0f, 444.0f, 480.0f, 504.0f};
  const float glyphY[] = {127.0f, 126.0f, 126.0f, 126.0f, 126.0f};
  for (int index = 0; index < 5; ++index) {
    raised(renderer_, SDL_FRect{buttons[index] + 2.0f, 123.0f, 23.0f, 22.0f});
    drawGlyph(buttonGlyphs[index], glyphX[index], glyphY[index]);
  }
  raised(renderer_, SDL_FRect{381.0f, 146.0f, 23.0f, 22.0f});
  drawGlyph("SpeedButton20", 385.0f, 150.0f);
  sunken(renderer_, SDL_FRect{428.0f, 151.0f, 36.0f, 21.0f});
  sunken(renderer_, SDL_FRect{488.0f, 151.0f, 36.0f, 21.0f});
  setColor(renderer_, kText);
  drawText(renderer_, 415.0f, 154.0f, "x:", assets);
  drawText(renderer_, 475.0f, 154.0f, "y:", assets);
  const SDL_Color dark{105, 105, 105, 255};
  const SDL_Color pale{227, 227, 227, 255};
  hLine(renderer_, 376.0f, 528.0f, 195.0f, mid);
  hLine(renderer_, 377.0f, 527.0f, 196.0f, dark);
  vLine(renderer_, 376.0f, 195.0f, 452.0f, mid);
  vLine(renderer_, 377.0f, 196.0f, 452.0f, dark);
  vLine(renderer_, 452.0f, 197.0f, 452.0f, mid);
  vLine(renderer_, 453.0f, 197.0f, 452.0f, white);
  vLine(renderer_, 527.0f, 197.0f, 452.0f, mid);
  vLine(renderer_, 528.0f, 196.0f, 452.0f, pale);
  vLine(renderer_, 529.0f, 195.0f, 452.0f, white);
  for (int row = 0; row < 17; ++row) {
    const float y = 212.0f + static_cast<float>(row) * 16.0f;
    if (y < 453.0f) {
      hLine(renderer_, 378.0f, 452.0f, y, mid);
      hLine(renderer_, 454.0f, 526.0f, y, mid);
    }
  }
  const char* labels[] = {"Sprite #", "", "Loop", "Continuous", "Back. color", "Single dir", "Setbackto", "",
                          "Def. head", "Def. body", "Def. attr1", "Def. attr2", "Def. attr3", "Def. param1",
                          "Def. param2", "Def. param3"};
  setColor(renderer_, kText);
  for (int row = 0; row < 16; ++row)
    drawText(renderer_, 383.0f, 199.0f + static_cast<float>(row) * 16.0f, labels[row], assets);
}

void Editor::drawMoviePanelStatic() {
  const std::filesystem::path assets = assetDir();
  ensureFont(g_regular, renderer_, assets, false, 13.0f);
  const SDL_Color mid{160, 160, 160, 255};
  checkBox(renderer_, 377.0f, 42.0f, automaticGani_);
  checkBox(renderer_, 377.0f, 58.0f, graal2002_);
  setColor(renderer_, kText);
  drawText(renderer_, 393.0f, 41.0f, "Automatic Gani Setting", assets);
  drawText(renderer_, 393.0f, 57.0f, "Graal2002 ganis", assets);
  drawText(renderer_, 380.0f, 86.0f, "Edit attributes:", assets);
  comboBox(renderer_, SDL_FRect{376.0f, 100.0f, 153.0f, 20.0f});
  drawText(renderer_, 379.0f, 104.0f, "Gani", assets);
  hLine(renderer_, 376.0f, 527.0f, 127.0f, mid);
  hLine(renderer_, 376.0f, 527.0f, 128.0f, SDL_Color{255, 255, 255, 255});
  vLine(renderer_, 376.0f, 127.0f, 336.0f, mid);
  vLine(renderer_, 377.0f, 127.0f, 336.0f, SDL_Color{105, 105, 105, 255});
  vLine(renderer_, 452.0f, 127.0f, 336.0f, mid);
  vLine(renderer_, 453.0f, 127.0f, 336.0f, SDL_Color{255, 255, 255, 255});
  vLine(renderer_, 527.0f, 127.0f, 336.0f, mid);
  vLine(renderer_, 528.0f, 127.0f, 336.0f, SDL_Color{227, 227, 227, 255});
  vLine(renderer_, 529.0f, 127.0f, 336.0f, SDL_Color{255, 255, 255, 255});
  for (int row = 1; row < 14; ++row) hLine(renderer_, 376.0f, 527.0f, 127.0f + static_cast<float>(row) * 16.0f, mid);
  const char* labels[] = {"Length", "Loop", "Continuous", "Back. color", "Setbackto", "Def. head", "Def. body",
                          "Def. attr1", "Def. attr2", "Def. attr3", "Def. param1", "Def. param2", "Def. param3"};
  setColor(renderer_, kText);
  for (int row = 0; row < 13; ++row)
    drawText(renderer_, 383.0f, 131.0f + static_cast<float>(row) * 16.0f, labels[row], assets);
}

void Editor::drawBottomBar() {
  fill(renderer_, SDL_FRect{0.0f, 410.0f, 376.0f, 43.0f}, kForm);
  hLine(renderer_, 0.0f, 366.0f, 410.0f, SDL_Color{160, 160, 160, 255});
  hLine(renderer_, 1.0f, 366.0f, 410.0f, SDL_Color{227, 227, 227, 255});
  hLine(renderer_, 0.0f, 367.0f, 411.0f, SDL_Color{255, 255, 255, 255});
  vLine(renderer_, 367.0f, 409.0f, 411.0f, SDL_Color{255, 255, 255, 255});
  const float cells[] = {72.0f, 97.0f, 132.0f, 157.0f, 182.0f, 207.0f, 232.0f, 267.0f};
  const char* spriteGlyphs[] = {"SpeedButton12", "SpeedButton13", "SpeedButton14", "SpeedButton15",
                                "SpeedButton16", "SpeedButton17", "SpeedButton21", "SpeedButton35"};
  const char* movieGlyphs[] = {"SpeedButton27", "SpeedButton28", "SpeedButton29", "SpeedButton30",
                               "SpeedButton31", "SpeedButton32", "SpeedButton33", "SpeedButton36"};
  const char* const* glyphs = document_.movie ? movieGlyphs : spriteGlyphs;
  for (int index = 0; index < 8; ++index) {
    raised(renderer_, SDL_FRect{cells[index], 420.0f, 25.0f, 25.0f});
    const GlyphInfo* glyph = findGlyph(glyphs[index]);
    if (!glyph) continue;
    const float x = cells[index] + static_cast<float>(std::lround((25.0 - glyph->w) / 2.0));
    const float y = 420.0f + static_cast<float>(std::lround((25.0 - glyph->h) / 2.0));
    bool enabled = true;
    if (!document_.movie) {
      if (index == 3) enabled = document_.frames.size() > 1;
      else if (index == 4) enabled = !document_.frames.empty();
      else if (index == 5 || index == 6) enabled = false;
    }
    if (enabled) drawGlyph(glyphs[index], x, y);
    else drawGlyphDisabled(glyphs[index], x, y);
  }
}

void Editor::drawChrome() {
  const SDL_Color mid{160, 160, 160, 255};
  const SDL_Color dark{105, 105, 105, 255};
  const SDL_Color white{255, 255, 255, 255};
  fill(renderer_, SDL_FRect{0.0f, 0.0f, 538.0f, 453.0f}, kForm);
  fill(renderer_, SDL_FRect{0.0f, 0.0f, 538.0f, 1.0f}, white);
  fill(renderer_, SDL_FRect{0.0f, 1.0f, 1.0f, 31.0f}, white);
  fill(renderer_, SDL_FRect{0.0f, 32.0f, 538.0f, 1.0f}, mid);
  fill(renderer_, SDL_FRect{0.0f, 33.0f, 366.0f, 1.0f}, mid);
  fill(renderer_, SDL_FRect{0.0f, 34.0f, 1.0f, 1.0f}, mid);
  fill(renderer_, SDL_FRect{1.0f, 34.0f, 365.0f, 1.0f}, dark);
  fill(renderer_, SDL_FRect{0.0f, 35.0f, 1.0f, 375.0f}, mid);
  fill(renderer_, SDL_FRect{1.0f, 35.0f, 1.0f, 375.0f}, dark);
  fill(renderer_, SDL_FRect{366.0f, 33.0f, 1.0f, 1.0f}, mid);
  fill(renderer_, SDL_FRect{366.0f, 34.0f, 1.0f, 376.0f}, SDL_Color{227, 227, 227, 255});
  fill(renderer_, SDL_FRect{367.0f, 33.0f, 1.0f, 377.0f}, white);
  drawToolbarGlyphs();
  const auto scrollButton = [&](float x, float y, float w, float h) {
    fill(renderer_, SDL_FRect{x, y, w, h}, kForm);
    hLine(renderer_, x, x + w - 2.0f, y, SDL_Color{227, 227, 227, 255});
    vLine(renderer_, x, y, y + h - 2.0f, SDL_Color{227, 227, 227, 255});
    hLine(renderer_, x + 1.0f, x + w - 3.0f, y + 1.0f, white);
    vLine(renderer_, x + 1.0f, y + 1.0f, y + h - 3.0f, white);
    vLine(renderer_, x + w - 2.0f, y + 1.0f, y + h - 2.0f, mid);
    hLine(renderer_, x + 1.0f, x + w - 2.0f, y + h - 2.0f, mid);
    hLine(renderer_, x, x + w - 1.0f, y + h - 1.0f, dark);
    vLine(renderer_, x + w - 1.0f, y, y + h - 1.0f, dark);
  };
  scrollButton(2.0f, 35.0f, 17.0f, 17.0f);
  scrollButton(2.0f, 347.0f, 17.0f, 17.0f);
  setColor(renderer_, SDL_Color{0, 0, 0, 255});
  for (int row = 0; row < 4; ++row) {
    SDL_RenderLine(renderer_, 10.0f - row, 41.0f + row, 10.0f + row, 41.0f + row);
    SDL_RenderLine(renderer_, 10.0f - row, 357.0f - row, 10.0f + row, 357.0f - row);
  }
  fill(renderer_, SDL_FRect{2.0f, 364.0f, 67.0f, 46.0f}, SDL_Color{0, 192, 0, 255});
  drawTimeString(22.0f, 369.0f, "Time");
  hLine(renderer_, 7.0f, 66.0f, 387.0f, SDL_Color{0, 0, 0, 255});
  hLine(renderer_, 7.0f, 66.0f, 405.0f, SDL_Color{0, 0, 0, 255});
  vLine(renderer_, 7.0f, 387.0f, 405.0f, SDL_Color{0, 0, 0, 255});
  vLine(renderer_, 66.0f, 387.0f, 405.0f, SDL_Color{0, 0, 0, 255});
  setColor(renderer_, kText);
  const std::filesystem::path assets = assetDir();
  ensureFont(g_regular, renderer_, assets, false, 13.0f);
  if (document_.movie) drawMoviePanelStatic();
  else drawPanelStatic();
  drawBottomBar();
}

void Editor::drawDragGhost(const std::filesystem::path& assets) {
  (void)assets;
  if (stripDragSprite_ < 0 || stripDragSprite_ >= static_cast<int>(document_.sprites.size())) return;
  float mx = 0.0f;
  float my = 0.0f;
  mousePosition(renderer_, mx, my);
  const gani::Sprite& sprite = document_.sprites[static_cast<std::size_t>(stripDragSprite_)];
  const std::filesystem::path sheet = resolveSpriteSheet(gameRoot_, path_, document_, sprite);
  SDL_Texture* texture = sheet.empty() ? nullptr : sheets_.get(sheet, isBodySprite(sprite));
  if (!texture) return;
  const SDL_FRect source{static_cast<float>(sprite.x), static_cast<float>(sprite.y),
                         static_cast<float>(sprite.width), static_cast<float>(sprite.height)};
  const SDL_FRect target{mx - static_cast<float>(sprite.width) / 2.0f,
                         my - static_cast<float>(sprite.height) / 2.0f,
                         static_cast<float>(sprite.width), static_cast<float>(sprite.height)};
  SDL_SetTextureAlphaMod(texture, 200);
  SDL_RenderTexture(renderer_, texture, &source, &target);
  SDL_SetTextureAlphaMod(texture, 255);
}

void Editor::ensureUiTexture() {
  if (uiTexture_) return;
  uiTexture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 538, 453);
  if (uiTexture_) SDL_SetTextureBlendMode(uiTexture_, SDL_BLENDMODE_NONE);
}

void Editor::drawBase(const Layout& layout, const std::filesystem::path& assets, bool interactive) {
  drawChrome();
  drawStrip(layout, assets);
  drawCanvas(layout, assets);
  if (document_.movie) drawMoviePanel(assets);
  else drawPanel(layout, assets);
  drawTimeline(layout, assets);
  drawModeButton(assets, 538.0f);
  drawUndoRedo(assets);
  if (interactive) {
    drawHover(assets);
    if (!document_.movie) drawPopup(assets);
    drawTooltip(assets);
    drawDragGhost(assets);
  }
}

void Editor::draw(int width, int height) {
  view_ = layout(width, height);
  panelShift_ = view_.panelLeft - 376.0f;
  reflow_ = !(width == 538 && height == 453);
  const std::filesystem::path assets = assetDir();
  fill(renderer_, SDL_FRect{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)}, kForm);
  if (!reflow_) {
    drawBase(view_, assets, true);
    drawPanelEditor(assets);
    return;
  }
  ensureUiTexture();
  if (!uiTexture_) {
    drawBase(view_, assets, true);
    drawPanelEditor(assets);
    return;
  }
  SDL_SetRenderTarget(renderer_, uiTexture_);
  SDL_SetRenderDrawColor(renderer_, 240, 240, 240, 255);
  SDL_RenderClear(renderer_);
  drawBase(layout(538, 453), assets, false);
  SDL_SetRenderTarget(renderer_, nullptr);
  fill(renderer_, SDL_FRect{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)}, kForm);
  const auto blit = [&](float sx, float sy, float sw, float sh, float dx, float dy) {
    const SDL_FRect source{sx, sy, sw, sh};
    const SDL_FRect target{dx, dy, sw, sh};
    SDL_RenderTexture(renderer_, uiTexture_, &source, &target);
  };
  blit(0.0f, 0.0f, 538.0f, 33.0f, 0.0f, 0.0f);
  fill(renderer_, SDL_FRect{538.0f, 0.0f, static_cast<float>(width) - 538.0f, 32.0f}, kForm);
  setColor(renderer_, SDL_Color{160, 160, 160, 255});
  SDL_RenderLine(renderer_, 0.0f, 32.0f, static_cast<float>(width), 32.0f);
  blit(0.0f, 412.0f, 376.0f, 41.0f, 0.0f, view_.bar.y);
  fill(renderer_, SDL_FRect{376.0f, view_.bar.y, view_.panelLeft - 376.0f, 41.0f}, kForm);
  blit(0.0f, 364.0f, 69.0f, 45.0f, 0.0f, view_.timeline.y);
  blit(0.0f, 33.0f, 19.0f, 19.0f, 0.0f, 33.0f);
  const float stripBottom = view_.strip.y + view_.strip.h;
  blit(0.0f, 347.0f, 19.0f, 18.0f, 0.0f, stripBottom - 17.0f);
  drawStrip(view_, assets);
  drawCanvas(view_, assets);
  drawTimeline(view_, assets);
  setColor(renderer_, SDL_Color{160, 160, 160, 255});
  SDL_RenderLine(renderer_, 0.0f, 33.0f, view_.panelLeft - 10.0f, 33.0f);
  setColor(renderer_, SDL_Color{105, 105, 105, 255});
  SDL_RenderLine(renderer_, 0.0f, 34.0f, view_.panelLeft - 11.0f, 34.0f);
  blit(376.0f, 33.0f, 162.0f, 420.0f, view_.panelLeft, 33.0f);
  blit(366.0f, 33.0f, 10.0f, 420.0f, view_.panelLeft - 10.0f, 33.0f);
  const float extensionTop = 453.0f;
  if (view_.bar.y > extensionTop) {
    fill(renderer_, SDL_FRect{view_.panelLeft - 10.0f, extensionTop, 10.0f, view_.bar.y - extensionTop},
         kForm);
  }
  if (static_cast<float>(height) > extensionTop) {
    fill(renderer_, SDL_FRect{view_.panelLeft, extensionTop, static_cast<float>(width) - view_.panelLeft,
                              static_cast<float>(height) - extensionTop},
         kForm);
  }
  drawPanelEditor(assets);
  setColor(renderer_, SDL_Color{160, 160, 160, 255});
  SDL_RenderLine(renderer_, 0.0f, 33.0f, 0.0f, view_.bar.y);
  setColor(renderer_, SDL_Color{105, 105, 105, 255});
  SDL_RenderLine(renderer_, 1.0f, 34.0f, 1.0f, view_.bar.y - 1.0f);
  drawModeButton(assets, static_cast<float>(width));
  drawHover(assets);
  if (!document_.movie) drawPopup(assets);
  drawTooltip(assets);
  drawDragGhost(assets);
}

bool Editor::save() {
  if (path_.empty()) {
    static const SDL_DialogFileFilter filters[] = {{"Graal animations", "gani"}};
    SDL_ShowSaveFileDialog(
        [](void* userdata, const char* const* files, int) {
          if (files && files[0]) static_cast<Editor*>(userdata)->saveTo(files[0]);
        },
        this, window_, filters, 1, nullptr);
    return true;
  }
  return saveTo(path_);
}

bool Editor::saveTo(const std::filesystem::path& path) {
  std::ofstream stream(path, std::ios::binary);
  if (!stream) {
    status_ = "save failed";
    return false;
  }
  const std::string serialized = gani::serialize(document_);
  stream << serialized;
  path_ = path;
  dirty_ = false;
  status_ = "saved";
  if (window_) SDL_SetWindowTitle(window_, ("Graal Shop - The Graal Animation Editor - " + path.filename().string()).c_str());
  writeSettings();
#ifdef __EMSCRIPTEN__
  EM_ASM({
    try {
      var blob = new Blob([UTF8ToString($0)], {type: 'text/plain'});
      var link = document.createElement('a');
      link.href = URL.createObjectURL(blob);
      link.download = UTF8ToString($1);
      document.body.appendChild(link);
      link.click();
      link.remove();
      setTimeout(function() { URL.revokeObjectURL(link.href); }, 1000);
    } catch (err) {
      console.error('Graal Shop save failed', err);
    }
  }, serialized.c_str(), path.filename().string().c_str());
#endif
  return true;
}

void Editor::newDocument() {
  pushUndo();
  gani::Document document;
  gani::Sprite shadow;
  shadow.id = 0;
  shadow.type = "SPRITES";
  shadow.width = 24;
  shadow.height = 12;
  shadow.name = "shadow";
  document.sprites.push_back(shadow);
  document.defaults.head = "head19.png";
  document.defaults.body = "body.png";
  gani::Frame frame;
  frame.rows.resize(4);
  for (std::vector<gani::FrameSprite>& row : frame.rows) row.push_back(gani::FrameSprite{0, "", 12, 34});
  document.frames.push_back(frame);
  document_ = std::move(document);
  path_.clear();
  frame_ = 0;
  framePanel_ = false;
  selected_ = 0;
  undo_.clear();
  playing_ = true;
  dirty_ = true;
  status_ = "new document";
  if (window_) SDL_SetWindowTitle(window_, "Graal Shop - The Graal Animation Editor - untitled");
}

void Editor::addSpriteFromFile(const std::filesystem::path& path) {
  int width = 0;
  int height = 0;
  if (!imageSize(path, width, height)) {
    status_ = "unreadable image";
    return;
  }
  pushUndo();
  gani::Sprite sprite;
  for (const gani::Sprite& existing : document_.sprites) sprite.id = std::max(sprite.id, existing.id + 1);
  std::error_code error;
  const std::filesystem::path relative = std::filesystem::relative(path, gameRoot_, error);
  const bool inside = !error && !relative.empty() && relative.string().rfind("..", 0) != 0;
  sprite.type = inside ? path.filename().string() : path.string();
  sprite.x = 0;
  sprite.y = 0;
  sprite.width = width;
  sprite.height = height;
  sprite.name = path.stem().string();
  document_.sprites.push_back(std::move(sprite));
  selected_ = static_cast<int>(document_.sprites.size()) - 1;
  status_ = "sprite added";
}

void Editor::addSpriteDefinition(const std::string& image, const std::string& description, int index, int x, int y,
                                 int width, int height, int gridWidth, int gridHeight) {
  if (width <= 0 || height <= 0 || gridWidth <= 0 || gridHeight <= 0) return;
  pushUndo();
  for (int row = 0; row < gridHeight; ++row)
    for (int column = 0; column < gridWidth; ++column) {
      gani::Sprite sprite;
      sprite.id = index + column + row * gridHeight;
      sprite.type = image.empty() ? "SPRITES" : image;
      sprite.name = description;
      sprite.x = x + column * width;
      sprite.y = y + row * height;
      sprite.width = width;
      sprite.height = height;
      document_.sprites.push_back(std::move(sprite));
    }
  selected_ = static_cast<int>(document_.sprites.size()) - 1;
  dirty_ = true;
  status_ = "sprite added";
}

void Editor::openAddSpriteDialog() {
#ifdef __EMSCRIPTEN__
  EM_ASM({
    var overlay = document.createElement('div');
    overlay.id = 'gs-addsprite';
    overlay.style.cssText = 'position:fixed;inset:0;background:rgba(0,0,0,.18);display:flex;align-items:center;justify-content:center;z-index:10000;';
    overlay.innerHTML = '<div style="width:577px;background:#f0f0f0;border:1px solid #000;box-shadow:3px 3px 0 rgba(0,0,0,.22);font:12px Tahoma,sans-serif;color:#000">'
      + '<div style="height:24px;background:linear-gradient(#fff,#e9e9e9);border-bottom:1px solid #a0a0a0;display:flex;align-items:center;padding:0 8px;font-weight:bold">Add Sprites<span id="gs-close" style="margin-left:auto;cursor:pointer;font-weight:normal">X</span></div>'
      + '<div style="display:flex;gap:8px;padding:8px">'
      + '<fieldset style="width:273px;border:1px solid #a0a0a0;margin:0;padding:8px"><legend style="padding:0 2px">Sprite attributes</legend>'
      + '<p style="margin:0 0 8px">Specify the image for the sprite. This can be a normal bitmap (*.png) or a name of a default game image which will be replaced by the actual image of the player in the game.</p>'
      + '<div style="display:flex;align-items:center;gap:6px"><label>Image:</label><input id="gs-image" list="gs-images" value="SPRITES" style="flex:1;height:22px;border:2px inset #d0d0d0;background:#fff"></div>'
      + '<datalist id="gs-images"><option>SPRITES</option><option>HEAD</option><option>BODY</option><option>SHIELD</option><option>HORSE</option><option>SWORD</option><option>HAT</option></datalist>'
      + '<p style="margin:10px 0 8px">The description is the text displayed when you move the mouse over the sprite list on the left side of the screen.</p>'
      + '<div style="display:flex;align-items:center;gap:6px"><label>Description:</label><input id="gs-desc" value="sprite" style="flex:1;height:22px;border:2px inset #d0d0d0;background:#fff"></div>'
      + '<p style="margin:10px 0 8px">The sprite index specifies the order of the sprite in the sprite list and must be unique.</p>'
      + '<div style="display:flex;align-items:center;gap:6px"><label>Sprite index:</label><input id="gs-index" value="20" style="flex:1;height:22px;border:2px inset #d0d0d0;background:#fff"></div>'
      + '</fieldset>'
      + '<fieldset style="width:281px;border:1px solid #a0a0a0;margin:0;padding:8px"><legend style="padding:0 2px">Sprite size</legend>'
      + '<p style="margin:0 0 8px">Specify the rectangle of the image which is drawn:</p>'
      + '<div style="display:flex;gap:12px"><div><label>x:</label><input id="gs-x" value="0" style="width:36px;height:22px;border:2px inset #d0d0d0;background:#fff"></div><div><label>y:</label><input id="gs-y" value="0" style="width:36px;height:22px;border:2px inset #d0d0d0;background:#fff"></div></div>'
      + '<div style="display:flex;gap:12px;margin-top:6px"><div><label>width:</label><input id="gs-w" value="32" style="width:36px;height:22px;border:2px inset #d0d0d0;background:#fff"></div><div><label>height:</label><input id="gs-h" value="32" style="width:36px;height:22px;border:2px inset #d0d0d0;background:#fff"></div></div>'
      + '<p style="margin:10px 0 8px">Instead of a single sprite you can also add an array of sprites, each of the specified width and height. A grid is layed on the image, starting at (x,y). The first sprite in the upper left corner gets the sprite index typed in above.</p>'
      + '<div style="display:flex;gap:12px"><div><label>grid width:</label><input id="gs-gw" value="1" style="width:36px;height:22px;border:2px inset #d0d0d0;background:#fff"></div><div><label>grid height:</label><input id="gs-gh" value="1" style="width:36px;height:22px;border:2px inset #d0d0d0;background:#fff"></div></div>'
      + '</fieldset></div>'
      + '<div style="display:flex;justify-content:flex-end;gap:8px;padding:0 8px 8px"><button id="gs-add" style="min-width:75px;height:23px">Add</button><button id="gs-cancel" style="min-width:75px;height:23px">Cancel</button></div>'
      + '</div>';
    document.body.appendChild(overlay);
    var close = function() { if (overlay.parentNode) overlay.parentNode.removeChild(overlay); };
    document.getElementById('gs-cancel').addEventListener('click', close);
    document.getElementById('gs-close').addEventListener('click', close);
    document.getElementById('gs-add').addEventListener('click', function() {
      var v = function(id) { return document.getElementById(id).value; };
      var n = function(id, fallback) { var value = parseInt(v(id), 10); return isNaN(value) ? fallback : value; };
      var image = stringToNewUTF8(v('gs-image'));
      var description = stringToNewUTF8(v('gs-desc'));
      Module._graalshop_add_sprite(image, description, n('gs-index', 0), n('gs-x', 0), n('gs-y', 0), n('gs-w', 32), n('gs-h', 32), n('gs-gw', 1), n('gs-gh', 1));
      _free(image);
      _free(description);
      close();
    });
  });
#else
  SDL_Window* dialog = SDL_CreateWindow("Add Sprites", 577, 361, SDL_WINDOW_RESIZABLE);
  if (!dialog) return;
  int wx = 0, wy = 0;
  if (window_) {
    SDL_GetWindowPosition(window_, &wx, &wy);
    SDL_SetWindowPosition(dialog, wx + 60, wy + 60);
  }
  SDL_Renderer* dialogRenderer = SDL_CreateRenderer(dialog, nullptr);
  if (!dialogRenderer) {
    SDL_DestroyWindow(dialog);
    return;
  }
  const std::filesystem::path assets = assetDir();
  FontSlot dialogFont;
  ensureFont(dialogFont, dialogRenderer, assets, false, 13.0f);
  std::string image = "SPRITES";
  std::string description = "sprite";
  std::string index = "20";
  std::string xText = "0";
  std::string yText = "0";
  std::string widthText = "32";
  std::string heightText = "32";
  std::string gridWidthText = "1";
  std::string gridHeightText = "1";
  int focus = 0;
  bool comboOpen = false;
  bool running = true;
  bool added = false;
  const char* comboItems[] = {"SPRITES", "HEAD", "BODY", "SHIELD", "HORSE", "SWORD", "HAT"};
  SDL_StartTextInput(dialog);
  const SDL_WindowID dialogID = SDL_GetWindowID(dialog);
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
        break;
      }
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == dialogID) {
        running = false;
        break;
      }
      if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        const float mx = event.button.x;
        const float my = event.button.y;
        const SDL_FRect edits[] = {{104, 123, 161, 24}, {104, 221, 161, 24}, {104, 311, 161, 24},
                                   {368, 70, 49, 24},   {504, 70, 49, 24},   {368, 99, 49, 24},
                                   {504, 99, 49, 24},   {376, 262, 41, 24},  {512, 262, 41, 24}};
        focus = -1;
        for (int field = 0; field < 9; ++field)
          if (inside(edits[field], mx, my)) focus = field;
        comboOpen = false;
        if (inside(SDL_FRect{104, 123, 161, 24}, mx, my)) {
          focus = 0;
          if (mx >= 246.0f) comboOpen = true;
        }
        if (comboOpen) {
          for (int item = 0; item < 7; ++item)
            if (inside(SDL_FRect{104, 147 + static_cast<float>(item) * 17, 161, 17}, mx, my)) {
              image = comboItems[item];
              comboOpen = false;
            }
        }
        if (inside(SDL_FRect{376, 322, 89, 28}, mx, my)) {
          running = false;
          added = true;
        } else if (inside(SDL_FRect{472, 322, 89, 28}, mx, my)) {
          running = false;
        }
        continue;
      }
      if (event.type == SDL_EVENT_TEXT_INPUT && focus >= 0) {
        std::string* target = focus == 0 ? &image : focus == 1 ? &description : focus == 2 ? &index
                              : focus == 3 ? &xText : focus == 4 ? &yText : focus == 5 ? &widthText
                              : focus == 6 ? &heightText : focus == 7 ? &gridWidthText : &gridHeightText;
        for (const char character : std::string(event.text.text)) {
          if (focus >= 2 && focus != 1 && !(character >= '0' && character <= '9') && character != '-') continue;
          *target += character;
        }
        continue;
      }
      if (event.type == SDL_EVENT_KEY_DOWN) {
        const SDL_Keycode key = event.key.key;
        if (key == SDLK_ESCAPE) {
          running = false;
        } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
          running = false;
          added = true;
        } else if (key == SDLK_TAB) {
          focus = (focus + (event.key.mod & SDL_KMOD_SHIFT ? 8 : 1)) % 9;
        } else if (key == SDLK_BACKSPACE && focus >= 0) {
          std::string* target = focus == 0 ? &image : focus == 1 ? &description : focus == 2 ? &index
                                : focus == 3 ? &xText : focus == 4 ? &yText : focus == 5 ? &widthText
                                : focus == 6 ? &heightText : focus == 7 ? &gridWidthText : &gridHeightText;
          if (!target->empty()) target->pop_back();
        }
        continue;
      }
    }
    SDL_SetRenderDrawColor(dialogRenderer, 240, 240, 240, 255);
    SDL_RenderClear(dialogRenderer);
    const SDL_Color mid{160, 160, 160, 255};
    const SDL_Color white{255, 255, 255, 255};
    const SDL_Color black{0, 0, 0, 255};
    const SDL_FRect attributes{8, 8, 273, 345};
    const SDL_FRect sizes{288, 8, 281, 297};
    fill(dialogRenderer, attributes, SDL_Color{240, 240, 240, 255});
    fill(dialogRenderer, sizes, SDL_Color{240, 240, 240, 255});
    outline(dialogRenderer, attributes, mid);
    outline(dialogRenderer, SDL_FRect{9, 9, 271, 343}, white);
    outline(dialogRenderer, sizes, mid);
    outline(dialogRenderer, SDL_FRect{289, 9, 279, 295}, white);
    fill(dialogRenderer, SDL_FRect{16, 3, 100, 10}, SDL_Color{240, 240, 240, 255});
    fill(dialogRenderer, SDL_FRect{296, 3, 80, 10}, SDL_Color{240, 240, 240, 255});
    setColor(dialogRenderer, black);
    drawTextEx(dialogRenderer, dialogFont, 16, 2, "Sprite attributes", assets);
    drawTextEx(dialogRenderer, dialogFont, 296, 2, "Sprite size", assets);
    auto label = [&](float x, float y, float width, const std::string& text) {
      if (!dialogFont.font.ready()) return;
      setColor(dialogRenderer, black);
      std::istringstream words(text);
      std::string word;
      std::string line;
      float lineY = y;
      while (words >> word) {
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (dialogFont.font.width(candidate) > width && !line.empty()) {
          drawTextEx(dialogRenderer, dialogFont, x, lineY, line, assets);
          lineY += 13.0f;
          line = word;
        } else {
          line = candidate;
        }
      }
      if (!line.empty()) drawTextEx(dialogRenderer, dialogFont, x, lineY, line, assets);
    };
    auto edit = [&](const SDL_FRect& rect, const std::string& value, int field) {
      sunken(dialogRenderer, rect);
      setColor(dialogRenderer, black);
      drawTextEx(dialogRenderer, dialogFont, rect.x + 3, rect.y + 3, value, assets);
      if (focus == field) {
        const float caret = rect.x + 3 + dialogFont.font.width(value);
        vLine(dialogRenderer, caret, rect.y + 3, rect.y + rect.h - 3, black);
      }
    };
    label(24, 32, 241, "Specify the image for the sprite. This can be a normal bitmap (*.png) or a name of a default game image which will be replaced by the actual image of the player in the game.");
    label(24, 128, 200, "Image:");
    label(24, 160, 241, "The description is the text displayed when you move the mouse over the sprite list on the left side of the screen.");
    label(24, 225, 200, "Description:");
    label(24, 257, 241, "The sprite index specifies the order of the sprite in the sprite list and must be unique.");
    label(24, 315, 200, "Sprite index:");
    label(304, 32, 249, "Specify the rectangle of the image which is drawn:");
    label(304, 74, 40, "x:");
    label(432, 74, 40, "y:");
    label(304, 103, 50, "width:");
    label(432, 103, 50, "height:");
    label(304, 136, 249, "Instead of a single sprite you can also add an array of sprites, each of the specified width and height. A grid is layed on the image, starting at (x,y). The first sprite in the upper left corner gets the sprite index typed in above.");
    label(304, 266, 70, "grid width:");
    label(432, 266, 70, "grid height:");
    comboBox(dialogRenderer, SDL_FRect{104, 123, 161, 24});
    setColor(dialogRenderer, black);
    drawTextEx(dialogRenderer, dialogFont, 107, 127, image, assets);
    if (focus == 0 && dialogFont.font.width(image) < 130) {
      const float caret = 107 + dialogFont.font.width(image);
      vLine(dialogRenderer, caret, 126, 144, black);
    }
    edit(SDL_FRect{104, 221, 161, 24}, description, 1);
    edit(SDL_FRect{104, 311, 161, 24}, index, 2);
    edit(SDL_FRect{368, 70, 49, 24}, xText, 3);
    edit(SDL_FRect{504, 70, 49, 24}, yText, 4);
    edit(SDL_FRect{368, 99, 49, 24}, widthText, 5);
    edit(SDL_FRect{504, 99, 49, 24}, heightText, 6);
    edit(SDL_FRect{376, 262, 41, 24}, gridWidthText, 7);
    edit(SDL_FRect{512, 262, 41, 24}, gridHeightText, 8);
    const SDL_FRect addButton{376, 322, 89, 28};
    const SDL_FRect cancelButton{472, 322, 89, 28};
    raised(dialogRenderer, addButton);
    raised(dialogRenderer, cancelButton);
    setColor(dialogRenderer, black);
    drawTextEx(dialogRenderer, dialogFont, addButton.x + 30, addButton.y + 7, "Add", assets);
    drawTextEx(dialogRenderer, dialogFont, cancelButton.x + 20, cancelButton.y + 7, "Cancel", assets);
    if (comboOpen) {
      fill(dialogRenderer, SDL_FRect{104, 147, 161, 119}, white);
      outline(dialogRenderer, SDL_FRect{104, 147, 161, 119}, mid);
      for (int item = 0; item < 7; ++item) {
        const SDL_FRect row{105, 148 + static_cast<float>(item) * 17, 159, 17};
        if (item == 0) fill(dialogRenderer, row, SDL_Color{0, 120, 215, 255});
        setColor(dialogRenderer, item == 0 ? white : black);
        drawTextEx(dialogRenderer, dialogFont, row.x + 3, row.y + 2, comboItems[item], assets);
      }
    }
    SDL_RenderPresent(dialogRenderer);
    SDL_Delay(1);
  }
  SDL_StopTextInput(dialog);
  if (added) {
    auto number = [](const std::string& text, int fallback) {
      char* end = nullptr;
      const long value = std::strtol(text.c_str(), &end, 10);
      return end != text.c_str() && *end == '\0' ? static_cast<int>(value) : fallback;
    };
    addSpriteDefinition(image, description, number(index, 0), number(xText, 0), number(yText, 0),
                        number(widthText, 32), number(heightText, 32), number(gridWidthText, 1),
                        number(gridHeightText, 1));
  }
  SDL_DestroyRenderer(dialogRenderer);
  SDL_DestroyWindow(dialog);
  if (window_) SDL_RaiseWindow(window_);
#endif
}

void Editor::openSpriteDialog() {
  static const SDL_DialogFileFilter filters[] = {{"Images", "png;jpg;jpeg;bmp;gif"}};
  SDL_ShowOpenFileDialog(
      [](void* userdata, const char* const* files, int) {
        if (files && files[0]) static_cast<Editor*>(userdata)->addSpriteFromFile(files[0]);
      },
      this, window_, filters, 1, nullptr, false);
}

void Editor::importSprites() {
  static const SDL_DialogFileFilter filters[] = {{"Graal animations", "gani"}};
  SDL_ShowOpenFileDialog(
      [](void* userdata, const char* const* files, int) {
        if (files && files[0]) static_cast<Editor*>(userdata)->importSpritesFrom(files[0]);
      },
      this, window_, filters, 1, nullptr, false);
}

void Editor::importSpritesFrom(const std::filesystem::path& path) {
  ensureRemoteFile(path);
  const gani::ParseResult result = gani::load(path);
  if (!result) {
    status_ = "import failed";
    return;
  }
  pushUndo();
  int next = 0;
  for (const gani::Sprite& sprite : document_.sprites) next = std::max(next, sprite.id + 1);
  for (gani::Sprite sprite : result.document->sprites) {
    sprite.id = next++;
    document_.sprites.push_back(std::move(sprite));
  }
  status_ = "sprites imported";
}

void Editor::removeUnusedSprites() {
  std::vector<bool> used(document_.sprites.size(), false);
  const auto mark = [&](int id) {
    for (std::size_t index = 0; index < document_.sprites.size(); ++index)
      if (document_.sprites[index].id == id) used[index] = true;
  };
  for (const gani::Frame& frame : document_.frames)
    for (const std::vector<gani::FrameSprite>& row : frame.rows)
      for (const gani::FrameSprite& part : row)
        if (part.sprite >= 0) mark(part.sprite);
  if (document_.movie)
    for (const gani::MovieActor& actor : document_.movie->actors)
      for (const gani::MovieKeyframe& keyframe : actor.keyframes) {
        const auto entry = keyframe.values.find("sprite #");
        if (entry != keyframe.values.end()) mark(std::atoi(entry->second.c_str()));
      }
  const int unused = static_cast<int>(std::count(used.begin(), used.end(), false));
  if (unused == 0) return;
  pushUndo();
  for (std::size_t index = document_.sprites.size(); index-- > 0;)
    if (!used[index]) document_.sprites.erase(document_.sprites.begin() + static_cast<std::ptrdiff_t>(index));
  if (selected_ >= static_cast<int>(document_.sprites.size())) selected_ = static_cast<int>(document_.sprites.size()) - 1;
  status_ = "unused sprites removed";
}

bool Editor::loadDocument(const std::filesystem::path& path) {
  ensureRemoteFile(path);
  const gani::ParseResult result = gani::load(path);
  if (!result) {
    status_ = "open failed";
    return false;
  }
  document_ = std::move(*result.document);
  path_ = path;
  frame_ = 0;
  framePanel_ = false;
  selected_ = document_.sprites.empty() ? -1 : 0;
  undo_.clear();
  playing_ = true;
  dirty_ = false;
  status_ = "opened";
  std::error_code error;
  for (std::filesystem::path probe = path.parent_path(); !probe.empty(); probe = probe.parent_path()) {
    if (std::filesystem::is_directory(probe / "levels" / "heads", error)) {
      gameRoot_ = probe;
      break;
    }
    if (probe == probe.parent_path()) break;
  }
  if (window_) SDL_SetWindowTitle(window_, ("Graal Shop - The Graal Animation Editor - " + path.filename().string()).c_str());
  writeSettings();
  return true;
}

void Editor::openDocumentDialog() {
#ifdef __EMSCRIPTEN__
  EM_ASM({
    var input = document.createElement('input');
    input.type = 'file';
    input.accept = '.gani';
    input.style.display = 'none';
    document.body.appendChild(input);
    var finish = function() { if (input.parentNode) input.parentNode.removeChild(input); };
    input.addEventListener('change', function() {
      var file = input.files && input.files[0];
      if (!file) { finish(); return; }
      var reader = new FileReader();
      reader.onload = function() {
        try {
          var name = (file.name || 'opened.gani').replace(/[^A-Za-z0-9._-]/g, '_');
          if (!/\.gani$/i.test(name)) name += '.gani';
          var path = '/game/levels/ganis/' + name;
          if (typeof FS.mkdirTree === 'function') FS.mkdirTree('/game/levels/ganis');
          FS.writeFile(path, new Uint8Array(reader.result));
          var ptr = stringToNewUTF8(path);
          Module._graalshop_open_file(ptr);
          _free(ptr);
        } catch (err) {
          console.error('Graal Shop open failed', err);
        }
        finish();
      };
      reader.onerror = finish;
      reader.readAsArrayBuffer(file);
    });
    input.addEventListener('cancel', finish);
    input.click();
  });
#else
  static const SDL_DialogFileFilter filters[] = {{"Graal animations", "gani"}};
  SDL_ShowOpenFileDialog(
      [](void* userdata, const char* const* files, int) {
        if (files && files[0]) static_cast<Editor*>(userdata)->loadDocument(files[0]);
      },
      this, window_, filters, 1, nullptr, false);
#endif
}

void Editor::openDefaultDialog(int field) {
  pendingDefault_ = field;
  static const SDL_DialogFileFilter filters[] = {{"Images", "png;jpg;jpeg;bmp;gif"}};
  SDL_ShowOpenFileDialog(
      [](void* userdata, const char* const* files, int) {
        if (files && files[0]) static_cast<Editor*>(userdata)->applyDefault(files[0]);
      },
      this, window_, filters, 1, nullptr, false);
}

void Editor::addMovieActor(int spriteIndex) { addMovieActorAt(spriteIndex, 0, 0); }

void Editor::addMovieActorAt(int spriteIndex, int dx, int dy) {
  if (!document_.movie || spriteIndex < 0 || spriteIndex >= static_cast<int>(document_.sprites.size())) return;
  pushUndo();
  gani::MovieActor actor;
  actor.type = "SPRITE";
  actor.name = "Sprite" + std::to_string(document_.movie->actors.size() + 1);
  gani::MovieKeyframe keyframe;
  keyframe.frame = static_cast<int>(frame_);
  keyframe.values["sprite #"] = std::to_string(document_.sprites[static_cast<std::size_t>(spriteIndex)].id);
  keyframe.values["dx"] = std::to_string(dx);
  keyframe.values["dy"] = std::to_string(dy);
  keyframe.values["visible"] = "true";
  actor.keyframes.push_back(std::move(keyframe));
  document_.movie->actors.push_back(std::move(actor));
  selectedActor_ = static_cast<int>(document_.movie->actors.size()) - 1;
  selected_ = spriteIndex;
  status_ = "actor added";
}

void Editor::switchToMovieMode() {
  gani::Document document;
  document.version = gani::Version::gani0002;
  document.movie.emplace();
  document.frameCount = 100;
  document.sprites = document_.sprites;
  document.defaults = document_.defaults;
  document_ = std::move(document);
  path_.clear();
  frame_ = 0;
  framePanel_ = false;
  selected_ = -1;
  selectedActor_ = 0;
  playing_ = true;
  dirty_ = true;
  popup_ = Popup::none;
  undo_.clear();
  redo_.clear();
  status_ = "movie mode";
  if (window_) SDL_SetWindowTitle(window_, "Graal Shop - The Graal Animation Editor - new (modified)");
}

void Editor::switchToSpriteMode() {
  gani::Document document;
  document.sprites = document_.sprites;
  document.defaults = document_.defaults;
  gani::Frame frame;
  frame.rows.resize(4);
  document.frames.push_back(frame);
  document_ = std::move(document);
  path_.clear();
  frame_ = 0;
  framePanel_ = false;
  selected_ = document_.sprites.empty() ? -1 : 0;
  playing_ = true;
  dirty_ = true;
  popup_ = Popup::none;
  undo_.clear();
  redo_.clear();
  status_ = "sprite mode";
  if (window_) SDL_SetWindowTitle(window_, "Graal Shop - The Graal Animation Editor - new (modified)");
}

void Editor::removeActor() {
  if (!document_.movie || selectedActor_ < 0 ||
      selectedActor_ >= static_cast<int>(document_.movie->actors.size()))
    return;
  pushUndo();
  document_.movie->actors.erase(document_.movie->actors.begin() + selectedActor_);
  if (selectedActor_ >= static_cast<int>(document_.movie->actors.size()))
    selectedActor_ = static_cast<int>(document_.movie->actors.size()) - 1;
  status_ = "actor removed";
}

void Editor::moveActorKeyframe(int dx, int dy) {
  if (!document_.movie || selectedActor_ < 0 ||
      selectedActor_ >= static_cast<int>(document_.movie->actors.size()))
    return;
  gani::MovieActor& actor = document_.movie->actors[static_cast<std::size_t>(selectedActor_)];
  gani::MovieKeyframe* keyframe = const_cast<gani::MovieKeyframe*>(nearestKeyframe(actor, static_cast<int>(frame_)));
  if (!keyframe) return;
  pushUndo();
  keyframe->values["dx"] = std::to_string(keyframeInt(*keyframe, "dx", 0) + dx);
  keyframe->values["dy"] = std::to_string(keyframeInt(*keyframe, "dy", 0) + dy);
  status_ = "keyframe moved";
}

void Editor::addTextActor() {
  if (!document_.movie) return;
  pushUndo();
  gani::MovieActor actor;
  actor.type = "TEXT";
  actor.name = "Text" + std::to_string(document_.movie->actors.size() + 1);
  gani::MovieKeyframe keyframe;
  keyframe.frame = static_cast<int>(frame_);
  keyframe.values["text"] = "Text";
  keyframe.values["dx"] = "0";
  keyframe.values["dy"] = "0";
  actor.keyframes.push_back(std::move(keyframe));
  document_.movie->actors.push_back(std::move(actor));
  selectedActor_ = static_cast<int>(document_.movie->actors.size()) - 1;
  status_ = "text actor added";
}

void Editor::insertKeyframe() {
  if (!document_.movie || selectedActor_ < 0 ||
      selectedActor_ >= static_cast<int>(document_.movie->actors.size()))
    return;
  gani::MovieActor& actor = document_.movie->actors[static_cast<std::size_t>(selectedActor_)];
  if (const gani::MovieKeyframe* existing = nearestKeyframe(actor, static_cast<int>(frame_));
      existing && existing->frame == static_cast<int>(frame_)) {
    status_ = "keyframe exists";
    return;
  }
  pushUndo();
  gani::MovieKeyframe keyframe;
  keyframe.frame = static_cast<int>(frame_);
  if (const gani::MovieKeyframe* previous = nearestKeyframe(actor, static_cast<int>(frame_))) keyframe.values = previous->values;
  actor.keyframes.push_back(std::move(keyframe));
  std::sort(actor.keyframes.begin(), actor.keyframes.end(),
            [](const gani::MovieKeyframe& left, const gani::MovieKeyframe& right) { return left.frame < right.frame; });
  status_ = "keyframe added";
}

void Editor::beginTextEdit() {
  if (!document_.movie || selectedActor_ < 0 ||
      selectedActor_ >= static_cast<int>(document_.movie->actors.size()))
    return;
  gani::MovieActor& actor = document_.movie->actors[static_cast<std::size_t>(selectedActor_)];
  if (actor.type != "TEXT") return;
  const gani::MovieKeyframe* keyframe = nearestKeyframe(actor, static_cast<int>(frame_));
  textBuffer_ = keyframe ? keyframe->values.count("text") ? keyframe->values.at("text") : std::string() : std::string();
  editingText_ = true;
  SDL_StartTextInput(window_);
  status_ = "editing text";
}

void Editor::commitTextEdit() {
  editingText_ = false;
  SDL_StopTextInput(window_);
  if (!document_.movie || selectedActor_ < 0 ||
      selectedActor_ >= static_cast<int>(document_.movie->actors.size()))
    return;
  gani::MovieActor& actor = document_.movie->actors[static_cast<std::size_t>(selectedActor_)];
  gani::MovieKeyframe* keyframe = const_cast<gani::MovieKeyframe*>(nearestKeyframe(actor, static_cast<int>(frame_)));
  if (!keyframe) return;
  pushUndo();
  keyframe->values["text"] = textBuffer_;
  status_ = "text updated";
}

void Editor::applyDefault(const std::filesystem::path& path) {
  const int field = pendingDefault_;
  pendingDefault_ = -1;
  if (field < 0) return;
  pushUndo();
  std::error_code error;
  const std::filesystem::path relative = std::filesystem::relative(path, gameRoot_, error);
  const bool inside = !error && !relative.empty() && relative.string().rfind("..", 0) != 0;
  const std::string name = (inside ? path.filename() : path).string();
  if (field == 0) document_.defaults.head = name;
  else if (field == 1) document_.defaults.body = name;
  else if (field == 2) document_.defaults.attrs[1] = name;
  else document_.defaults.params[1] = name;
  status_ = "default updated";
}

bool Editor::handleEvent(const SDL_Event& event, int width, int height) {
  const Layout frame = layout(width, height);
  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
    if (panelEditTarget_ != 0) commitPanelEdit();
    const float x = event.button.x;
    const float y = event.button.y;
    if (popup_ != Popup::none) {
      const bool onDirection = x >= 376.0f + panelShift_ && x < 529.0f + panelShift_ && y >= 55.0f && y < 77.0f;
      const bool onSprites = x >= 376.0f + panelShift_ && x < 529.0f + panelShift_ && y >= 100.0f && y < 122.0f;
      if (popup_ == Popup::direction && onDirection) {
        popup_ = Popup::none;
        return true;
      }
      if (popup_ == Popup::sprites && onSprites) {
        popup_ = Popup::none;
        return true;
      }
      if (onDirection || onSprites) {
        popup_ = onDirection ? Popup::direction : Popup::sprites;
        popupScroll_ = 0;
        if (onSprites) {
          const std::vector<int> items = rowSpriteIndices();
          for (std::size_t index = 0; index < items.size(); ++index)
            if (items[index] == selected_ && index >= 4) popupScroll_ = static_cast<int>(index) - 3;
        }
        return true;
      }
      const std::vector<int> items = popup_ == Popup::sprites ? rowSpriteIndices() : std::vector<int>{};
      const int count = popup_ == Popup::direction ? 4 : static_cast<int>(items.size());
      const int visible = std::min(count, 8);
      const float top = popup_ == Popup::direction ? 76.0f : 121.0f;
      const SDL_FRect rect{376.0f + panelShift_, top, 155.0f, static_cast<float>(visible * 13 + 2)};
      if (inside(rect, x, y) && count > 0) {
        const int row = popupScroll_ + static_cast<int>((y - rect.y - 1.0f) / 13.0f);
        if (row >= 0 && row < count) {
          if (popup_ == Popup::direction) direction_ = row;
          else selected_ = items[static_cast<std::size_t>(row)];
        }
      }
      popup_ = Popup::none;
      return true;
    }
    const int cell = toolbarCellAt(x, y);
    if (cell >= 0) {
      if ((cell == 5 && undo_.empty()) || (cell == 6 && redo_.empty())) return true;
      pressedRect_ = SDL_FRect{kToolbarCells[cell], 4.0f, 25.0f, 25.0f};
      pressed_ = true;
      if (cell == 0) newDocument();
      else if (cell == 1) openDocumentDialog();
      else if (cell == 2) save();
      else if (cell == 3) saveAs();
      else if (cell == 5) undo();
      else if (cell == 6) redo();
      else if (cell == 8) openAddSpriteDialog();
      else if (cell == 9) removeUnusedSprites();
      else if (cell == 10) importSprites();
      return true;
    }
    const float appWidth = std::max(538.0f, frame.panelLeft + 162.0f);
    const float buttonX = std::max(448.0f, appWidth - 90.0f);
    if (x >= buttonX && x < buttonX + 90.0f && y >= 4.0f && y < 29.0f) {
      const SDL_MessageBoxButtonData buttons[] = {
          {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "OK"},
          {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Cancel"}};
      const std::string message = document_.movie ? "Switching to sprite mode will remove all actors. Continue?"
                                                  : "Switching to movie mode will remove all sprites. Continue?";
      SDL_MessageBoxData data{};
      data.flags = SDL_MESSAGEBOX_WARNING;
      data.window = window_;
      data.title = "Warning";
      data.message = message.c_str();
      data.buttons = buttons;
      data.numbuttons = 2;
      int result = -1;
      if (SDL_ShowMessageBox(&data, &result) && result == 0) {
        if (document_.movie) switchToSpriteMode();
        else switchToMovieMode();
      }
      return true;
    }
    if (y >= frame.bar.y && x < 376.0f) {
      const SDL_FRect buttons[] = {{72.0f, frame.bar.y + 8.0f, 25.0f, 25.0f},
                                   {97.0f, frame.bar.y + 8.0f, 25.0f, 25.0f},
                                   {132.0f, frame.bar.y + 8.0f, 25.0f, 25.0f},
                                   {157.0f, frame.bar.y + 8.0f, 25.0f, 25.0f},
                                   {182.0f, frame.bar.y + 8.0f, 25.0f, 25.0f},
                                   {207.0f, frame.bar.y + 8.0f, 25.0f, 25.0f},
                                   {232.0f, frame.bar.y + 8.0f, 25.0f, 25.0f},
                                   {267.0f, frame.bar.y + 8.0f, 25.0f, 25.0f}};
      for (int index = 0; index < 8; ++index) {
        if (!inside(buttons[index], x, y)) continue;
        pressedRect_ = buttons[index];
        pressed_ = true;
        if (document_.movie) {
          const int count = std::max(1, document_.frameCount.value_or(40));
          if (index == 0) playing_ = !playing_;
          else if (index == 1) playing_ = false;
          else if (index == 2) {
            playing_ = false;
            frame_ = frame_ == 0 ? static_cast<std::size_t>(count - 1) : frame_ - 1;
          } else if (index == 3) {
            playing_ = false;
            frame_ = (frame_ + 1) % static_cast<std::size_t>(count);
          }
          return true;
        }
        if (index == 0) playing_ = !document_.frames.empty() && !playing_;
        else if (index == 1) playing_ = false;
        else if (index == 2 && !document_.frames.empty()) {
          playing_ = false;
          frame_ = frame_ == 0 ? document_.frames.size() - 1 : frame_ - 1;
        } else if (index == 3 && !document_.frames.empty()) {
          playing_ = false;
          frame_ = (frame_ + 1) % document_.frames.size();
        } else if (index == 4) insertFrame();
        else if (index == 5) duplicateFrame();
        else if (index == 6) deleteFrame();
        else if (index == 7) save();
        return true;
      }
      return true;
    }
    if (x < 19.0f && y >= frame.strip.y && y < frame.strip.y + frame.strip.h) {
      const int maximum = stripMaxScroll();
      const float stripBottom = frame.strip.y + frame.strip.h;
      const float trackHeight = std::max(kThumbHeight + 1.0f, frame.strip.h - 34.0f);
      const float travel = trackHeight - kThumbHeight;
      if (y < 50.0f) stripScroll_ -= 40;
      else if (y >= stripBottom - 17.0f) stripScroll_ += 40;
      else {
        const float thumbY = 52.0f + (maximum > 0 ? static_cast<float>(stripScroll_) / maximum : 0.0f) * travel;
        if (y >= thumbY && y < thumbY + kThumbHeight) {
          stripDrag_ = true;
          stripDragOffset_ = y - thumbY;
        } else {
          stripScroll_ += y < thumbY ? -static_cast<int>(frame.strip.h) : static_cast<int>(frame.strip.h);
        }
      }
      stripScroll_ = std::clamp(stripScroll_, 0, maximum);
      return true;
    }
    if (x < 69.0f && y >= frame.strip.y && y < frame.strip.y + frame.strip.h) {
      int cursor = static_cast<int>(frame.strip.y) + kStripMargin - stripScroll_;
      for (int index = 0; index < static_cast<int>(document_.sprites.size()); ++index) {
        const int rowHeight = document_.sprites[static_cast<std::size_t>(index)].height;
        if (y >= static_cast<float>(cursor) && y < static_cast<float>(cursor + rowHeight)) {
          selected_ = index;
          framePanel_ = false;
          stripPress_ = true;
          stripPressIndex_ = index;
          stripPressX_ = x;
          stripPressY_ = y;
          stripDragSprite_ = -1;
          if (event.button.clicks >= 2) {
            if (document_.movie) addMovieActor(index);
            else addSpriteToFrame(index);
          }
          break;
        }
        cursor += rowHeight + kStripGap;
      }
      return true;
    }
    if (inside(frame.timeline, x, y) && x >= frame.timeline.x + 12.0f) {
      framePanel_ = true;
      if (!document_.movie && !document_.frames.empty() && y >= frame.timeline.y + 5.0f &&
          y < frame.timeline.y + 28.0f) {
        float total = 0.0f;
        std::vector<float> starts;
        std::vector<float> ends;
        starts.reserve(document_.frames.size());
        ends.reserve(document_.frames.size());
        for (const gani::Frame& item : document_.frames) {
          starts.push_back(total);
          total += 0.05f * static_cast<float>(std::max(1, item.waitRepeats + 1));
          ends.push_back(total);
        }
        const float rulerWidth = std::max(1.0f, frame.timeline.w - 27.0f);
        const float pxPerSecond = rulerWidth / std::max(0.0001f, total + 0.05f);
        int bubble = -1;
        if (y < frame.timeline.y + 16.0f) {
          for (std::size_t index = 0; index < document_.frames.size(); ++index) {
            const float leftEdge = frame.timeline.x + 12.0f + starts[index] * pxPerSecond;
            const float rightEdge = frame.timeline.x + 12.0f + ends[index] * pxPerSecond;
            if (x >= leftEdge && x < rightEdge) {
              bubble = static_cast<int>(index);
              break;
            }
          }
        }
        int candidate = -1;
        float best = 10.0f;
        for (std::size_t index = 0; index < document_.frames.size(); ++index) {
          const float edge = frame.timeline.x + 12.0f + ends[index] * pxPerSecond;
          const float distance = std::fabs(x - edge);
          if (distance <= best) {
            best = distance;
            candidate = static_cast<int>(index);
          }
        }
        const int targetFrame = bubble >= 0 ? bubble : candidate;
        if (targetFrame >= 0) {
          timelineResize_ = true;
          timelineResizeFrame_ = targetFrame;
          timelineResizeStartX_ = x;
          timelineResizePxPerSecond_ = pxPerSecond;
          timelineResizeStartWait_ = document_.frames[static_cast<std::size_t>(targetFrame)].waitRepeats;
          pushUndo();
          playing_ = false;
          elapsed_ = 0.0f;
          return true;
        }
      }
      timelineDrag_ = true;
      scrubTimeline(x, frame);
      return true;
    }
    if (x >= frame.panelLeft) {
      const float px = x - panelShift_;
      if (document_.movie) {
        if (y >= 38.0f && y < 57.0f) {
          automaticGani_ = !automaticGani_;
          writeSettings();
          status_ = "automatic gani setting";
          return true;
        }
        if (y >= 57.0f && y < 78.0f) {
          graal2002_ = !graal2002_;
          writeSettings();
          status_ = "graal2002 ganis";
          return true;
        }
        if (y >= 127.0f && y < 335.0f && px >= 455.0f) {
          const int row = std::clamp(static_cast<int>((y - 127.0f) / 16.0f), 0, 12);
          if (row == 1 || row == 2) togglePanelBool(row);
          else beginPanelEdit(1, row);
        }
        return true;
      }
      if (y >= 55.0f && y < 77.0f) {
        popup_ = Popup::direction;
        popupScroll_ = 0;
        return true;
      }
      if (y >= 100.0f && y < 122.0f) {
        popup_ = Popup::sprites;
        const std::vector<int> items = rowSpriteIndices();
        popupScroll_ = 0;
        for (std::size_t index = 0; index < items.size(); ++index)
          if (items[index] == selected_ && index >= 4) popupScroll_ = static_cast<int>(index) - 3;
        return true;
      }
      if (y >= 123.0f && y < 146.0f) {
        static const SDL_FRect buttons[] = {{379.0f, 123.0f, 25.0f, 23.0f}, {413.0f, 123.0f, 25.0f, 23.0f},
                                            {438.0f, 123.0f, 25.0f, 23.0f}, {474.0f, 123.0f, 25.0f, 23.0f},
                                            {498.0f, 123.0f, 25.0f, 23.0f}};
        for (int index = 0; index < 5; ++index) {
          if (!inside(buttons[index], px, y)) continue;
          pressedRect_ = SDL_FRect{buttons[index].x + panelShift_, buttons[index].y, buttons[index].w,
                                   buttons[index].h};
          pressed_ = true;
          if (index == 0) addSelectedToFrame();
          else if (index == 1) nudgeSelected(-1, 0);
          else if (index == 2) nudgeSelected(1, 0);
          else if (index == 3) nudgeSelected(0, -1);
          else if (index == 4) nudgeSelected(0, 1);
          break;
        }
      } else if (y >= 146.0f && y < 171.0f && px >= 428.0f && px < 464.0f) {
        beginPanelEdit(2, 0);
      } else if (y >= 146.0f && y < 171.0f && px >= 488.0f && px < 524.0f) {
        beginPanelEdit(3, 0);
      } else if (y >= 146.0f && y < 171.0f && px < 402.0f) {
        pressedRect_ = SDL_FRect{379.0f + panelShift_, 146.0f, 23.0f, 22.0f};
        pressed_ = true;
        removeSelectedFromFrame();
      } else if (y >= kRowTop && y < 453.0f && px >= 455.0f) {
        const int row = static_cast<int>((y - kRowTop) / kRowHeight);
        if (row == 2 || row == 3 || row == 5) togglePanelBool(row);
        else if (row == 0) {
          if (framePanel_) beginPanelEdit(1, 0);
        } else if (row != 1 && row != 7) beginPanelEdit(1, row);
      }
      return true;
    }
    if (inside(frame.canvas, x, y)) {
      int spriteIndex = selected_;
      int partIndex = -1;
      if (gani::FrameSprite* part = hitTest(x, y, frame, spriteIndex, partIndex)) {
        pushUndo();
        selected_ = spriteIndex;
        dragging_ = true;
        dragSprite_ = part->sprite;
        dragIndex_ = partIndex;
        dragOffsetX_ = x - (originX(frame.canvas) + static_cast<float>(part->dx) * zoom_);
        dragOffsetY_ = y - (originY(frame.canvas) + static_cast<float>(part->dy) * zoom_);
      } else {
        canvasPan_ = true;
        canvasPanStartX_ = x;
        canvasPanStartY_ = y;
        canvasPanX_ = panX_;
        canvasPanY_ = panY_;
      }
      return true;
    }
  }
  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_MIDDLE &&
      inside(frame.canvas, event.button.x, event.button.y)) {
    canvasPan_ = true;
    canvasPanStartX_ = event.button.x;
    canvasPanStartY_ = event.button.y;
    canvasPanX_ = panX_;
    canvasPanY_ = panY_;
    return true;
  }
  if (event.type == SDL_EVENT_MOUSE_MOTION && canvasPan_) {
    panX_ = canvasPanX_ + (event.motion.x - canvasPanStartX_);
    panY_ = canvasPanY_ + (event.motion.y - canvasPanStartY_);
    return true;
  }
  if (event.type == SDL_EVENT_MOUSE_MOTION && timelineResize_) {
    if (timelineResizeFrame_ >= 0 && timelineResizeFrame_ < static_cast<int>(document_.frames.size())) {
      const float delta = (event.motion.x - timelineResizeStartX_) / std::max(1.0f, timelineResizePxPerSecond_);
      const int extra = static_cast<int>(std::lround(delta / 0.05f));
      const int wait = std::max(0, timelineResizeStartWait_ + extra);
      gani::Frame& target = document_.frames[static_cast<std::size_t>(timelineResizeFrame_)];
      if (target.waitRepeats != wait) {
        target.waitRepeats = wait;
        dirty_ = true;
      }
    }
    return true;
  }
  if (event.type == SDL_EVENT_MOUSE_MOTION && timelineDrag_) {
    scrubTimeline(event.motion.x, frame);
    return true;
  }
  if (event.type == SDL_EVENT_MOUSE_MOTION && stripDrag_) {
    const int maximum = stripMaxScroll();
    const float trackHeight = std::max(kThumbHeight + 1.0f, frame.strip.h - 34.0f);
    const float travel = std::max(1.0f, trackHeight - kThumbHeight);
    const float target = std::clamp(event.motion.y - stripDragOffset_, 52.0f, 52.0f + travel);
    stripScroll_ = maximum > 0 ? static_cast<int>(std::lround((target - 52.0f) / travel * maximum)) : 0;
    return true;
  }
  if (event.type == SDL_EVENT_MOUSE_MOTION && stripPress_) {
    if (stripDragSprite_ < 0 && stripPressIndex_ >= 0 &&
        (std::fabs(event.motion.x - stripPressX_) > 4.0f || std::fabs(event.motion.y - stripPressY_) > 4.0f))
      stripDragSprite_ = stripPressIndex_;
    if (stripDragSprite_ >= 0) return true;
  }
  if (event.type == SDL_EVENT_MOUSE_MOTION && dragging_) {
    gani::Frame* current = currentFrame();
    if (!current || direction_ < 0 || static_cast<std::size_t>(direction_) >= current->rows.size()) return true;
    auto& row = current->rows[static_cast<std::size_t>(direction_)];
    if (dragIndex_ < 0 || dragIndex_ >= static_cast<int>(row.size())) return true;
    gani::FrameSprite& part = row[static_cast<std::size_t>(dragIndex_)];
    const float baseX = originX(frame.canvas);
    const float baseY = originY(frame.canvas);
    part.dx = static_cast<int>(std::lround((event.motion.x - dragOffsetX_ - baseX) / zoom_));
    part.dy = static_cast<int>(std::lround((event.motion.y - dragOffsetY_ - baseY) / zoom_));
    dirty_ = true;
    return true;
  }
  if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
    canvasPan_ = false;
    if (stripPress_) {
      if (stripDragSprite_ >= 0) {
        const float x = event.button.x;
        const float y = event.button.y;
        if (inside(frame.canvas, x, y)) {
          const gani::Sprite& dragged = document_.sprites[static_cast<std::size_t>(stripDragSprite_)];
          const int dx = static_cast<int>(std::lround((x - originX(frame.canvas)) / zoom_ -
                                                     static_cast<float>(dragged.width) / 2.0f));
          const int dy = static_cast<int>(std::lround((y - originY(frame.canvas)) / zoom_ -
                                                     static_cast<float>(dragged.height) / 2.0f));
          if (document_.movie) addMovieActorAt(stripDragSprite_, dx, dy);
          else addSpriteToFrameAt(stripDragSprite_, dx, dy);
        }
        stripDragSprite_ = -1;
      }
      stripPress_ = false;
      return true;
    }
    dragging_ = false;
    stripDrag_ = false;
    timelineDrag_ = false;
    timelineResize_ = false;
    timelineResizeFrame_ = -1;
    pressed_ = false;
  }
  if (event.type == SDL_EVENT_MOUSE_WHEEL) {
    float mx = event.wheel.mouse_x;
    float my = event.wheel.mouse_y;
    if (mx == 0.0f && my == 0.0f) mousePosition(renderer_, mx, my);
    if (popup_ != Popup::none) {
      const int count = popup_ == Popup::direction ? 4 : static_cast<int>(rowSpriteIndices().size());
      popupScroll_ = std::clamp(popupScroll_ + (event.wheel.y > 0 ? -1 : 1), 0, std::max(0, count - 8));
      return true;
    }
    if (inside(frame.canvas, mx, my)) {
      const float before = zoom_;
      zoom_ = std::clamp(zoom_ * (event.wheel.y > 0 ? 1.1f : 1.0f / 1.1f), 0.5f, 16.0f);
      const float centerX = frame.canvas.x + frame.canvas.w / 2.0f;
      const float centerY = frame.canvas.y + frame.canvas.h / 2.0f;
      panX_ = (panX_ + centerX - mx) * (zoom_ / before) - centerX + mx;
      panY_ = (panY_ + centerY - my) * (zoom_ / before) - centerY + my;
      return true;
    }
    if (mx < 69.0f && my >= frame.strip.y && my < frame.strip.y + frame.strip.h) {
      stripScroll_ = std::clamp(stripScroll_ - static_cast<int>(event.wheel.y) * 40, 0, stripMaxScroll());
      return true;
    }
  }
  if (event.type == SDL_EVENT_TEXT_INPUT && panelEditTarget_ != 0) {
    if (panelEditFresh_ || panelEditText_ == panelEditOriginal_) {
      panelEditText_.clear();
      panelEditFresh_ = false;
    }
    for (const char character : std::string(event.text.text)) {
      const bool digit = character >= '0' && character <= '9';
      const bool hex = digit || (character >= 'a' && character <= 'f') || (character >= 'A' && character <= 'F');
      if (panelEditTarget_ == 2 || panelEditTarget_ == 3) {
        if (!digit && !(character == '-' && panelEditText_.empty())) continue;
      } else if (document_.movie && panelEditRow_ == 0) {
        if (!digit) continue;
      } else if ((document_.movie && panelEditRow_ == 3) || (!document_.movie && panelEditRow_ == 4)) {
        if (!hex || panelEditText_.size() >= 6) continue;
      }
      panelEditText_ += character;
    }
    return true;
  }
  if (event.type == SDL_EVENT_TEXT_INPUT && editingText_) {
    textBuffer_ += event.text.text;
    return true;
  }
  if (event.type == SDL_EVENT_KEY_DOWN) {
    const SDL_Keycode key = event.key.key;
    const bool control = (event.key.mod & SDL_KMOD_CTRL) != 0;
    if (panelEditTarget_ != 0) {
      if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        commitPanelEdit();
        return true;
      }
      if (key == SDLK_ESCAPE) {
        cancelPanelEdit();
        return true;
      }
      if (key == SDLK_BACKSPACE) {
        if (panelEditFresh_) {
          panelEditText_.clear();
          panelEditFresh_ = false;
        } else if (!panelEditText_.empty()) {
          panelEditText_.pop_back();
        }
        return true;
      }
      return true;
    }
    if (editingText_) {
      if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        commitTextEdit();
        return true;
      }
      if (key == SDLK_ESCAPE) {
        editingText_ = false;
        SDL_StopTextInput(window_);
        return true;
      }
      if (key == SDLK_BACKSPACE) {
        if (!textBuffer_.empty()) textBuffer_.pop_back();
        return true;
      }
      return true;
    }
    if (key == SDLK_K && control) {
      insertKeyframe();
      return true;
    }
    if (key == SDLK_T && control) {
      addTextActor();
      return true;
    }
    if ((key == SDLK_RETURN || key == SDLK_KP_ENTER) && document_.movie) {
      beginTextEdit();
      return true;
    }
    if (key == SDLK_SPACE) {
      playing_ = !document_.frames.empty() && !playing_;
      return true;
    }
    if (key == SDLK_DELETE) {
      if (document_.movie) removeActor();
      else if (control) deleteFrame();
      else removeSelected();
      return true;
    }
    if (key == SDLK_D && control) {
      duplicateFrame();
      return true;
    }
    if (key == SDLK_N && control) {
      if (event.key.mod & SDL_KMOD_SHIFT) newDocument();
      else insertFrame();
      return true;
    }
    if (key == SDLK_Z && control) {
      undo();
      return true;
    }
    if (control && (key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_UP || key == SDLK_DOWN)) {
      if (document_.movie) {
        if (key == SDLK_LEFT) moveActorKeyframe(-1, 0);
        else if (key == SDLK_RIGHT) moveActorKeyframe(1, 0);
        else if (key == SDLK_UP) moveActorKeyframe(0, -1);
        else moveActorKeyframe(0, 1);
        return true;
      }
      if (selected_ >= 0 && selected_ < static_cast<int>(document_.sprites.size())) {
        gani::Sprite& sprite = document_.sprites[static_cast<std::size_t>(selected_)];
        pushUndo();
        const bool shift = (event.key.mod & SDL_KMOD_SHIFT) != 0;
        if (shift) {
          if (key == SDLK_LEFT) sprite.width = std::max(1, sprite.width - 1);
          else if (key == SDLK_RIGHT) ++sprite.width;
          else if (key == SDLK_UP) sprite.height = std::max(1, sprite.height - 1);
          else ++sprite.height;
        } else {
          if (key == SDLK_LEFT) --sprite.x;
          else if (key == SDLK_RIGHT) ++sprite.x;
          else if (key == SDLK_UP) --sprite.y;
          else ++sprite.y;
        }
        status_ = "rect updated";
      }
      return true;
    }
    if (key >= SDLK_1 && key <= SDLK_4) {
      direction_ = static_cast<int>(key - SDLK_1);
      return true;
    }
    if (document_.movie && !control && (key == SDLK_LEFT || key == SDLK_RIGHT)) {
      const int count = std::max(1, document_.frameCount.value_or(40));
      playing_ = false;
      if (key == SDLK_RIGHT) frame_ = (frame_ + 1) % static_cast<std::size_t>(count);
      else frame_ = frame_ == 0 ? static_cast<std::size_t>(count - 1) : frame_ - 1;
      return true;
    }
    if (key == SDLK_RIGHT && !document_.frames.empty()) {
      playing_ = false;
      frame_ = (frame_ + 1) % document_.frames.size();
      return true;
    }
    if (key == SDLK_LEFT && !document_.frames.empty()) {
      playing_ = false;
      frame_ = frame_ == 0 ? document_.frames.size() - 1 : frame_ - 1;
      return true;
    }
    if (key == SDLK_EQUALS || key == SDLK_PLUS) {
      zoom_ = std::min(16.0f, zoom_ * 1.25f);
      return true;
    }
    if (key == SDLK_MINUS) {
      zoom_ = std::max(0.5f, zoom_ / 1.25f);
      return true;
    }
    if (key == SDLK_S && (event.key.mod & SDL_KMOD_CTRL)) return save();
  }
  return false;
}
}
