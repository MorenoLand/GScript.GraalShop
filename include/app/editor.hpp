#pragma once
#include "app/canvas.hpp"
#include "app/sheets.hpp"
#include "gani/gani.hpp"
#include <SDL3/SDL.h>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

struct SDL_Renderer;

namespace app {
class Editor {
public:
  Editor(SDL_Renderer* renderer, SDL_Window* window, SheetCache& sheets, gani::Document document,
         std::filesystem::path path, std::filesystem::path gameRoot, std::filesystem::path settingsPath);
  void update(float deltaSeconds);
  void draw(int width, int height);
  bool handleEvent(const SDL_Event& event, int width, int height);
  bool save();
  bool saveTo(const std::filesystem::path& path);
  void newDocument();
  void addSpriteFromFile(const std::filesystem::path& path);
  void openSpriteDialog();
  void openAddSpriteDialog();
  void addSpriteDefinition(const std::string& image, const std::string& description, int index, int x, int y, int width,
                           int height, int gridWidth, int gridHeight);
  void importSprites();
  void importSpritesFrom(const std::filesystem::path& path);
  void removeUnusedSprites();
  void openDocumentDialog();
  bool loadDocument(const std::filesystem::path& path);
  void openDefaultDialog(int field);
  void applyDefault(const std::filesystem::path& path);
  void addMovieActor(int spriteIndex);
  void removeActor();
  void moveActorKeyframe(int dx, int dy);
  void addTextActor();
  void insertKeyframe();
  void beginTextEdit();
  void commitTextEdit();
  void writeSettings();
  [[nodiscard]] bool dirty() const { return dirty_; }
  [[nodiscard]] const std::filesystem::path& path() const { return path_; }

private:
  struct Layout {
    SDL_FRect strip;
    SDL_FRect canvas;
    SDL_FRect timeline;
    SDL_FRect bar;
    float panelLeft{376.0f};
  };

  Layout layout(int width, int height) const;
  void drawBase(const Layout& layout, const std::filesystem::path& assets, bool interactive);
  void drawChrome();
  void drawToolbarGlyphs();
  void drawBottomBar();
  void drawPanelStatic();
  void drawMoviePanelStatic();
  void ensureGlyphs();
  void drawGlyph(const char* name, float x, float y, Uint8 r = 255, Uint8 g = 255, Uint8 b = 255);
  void drawGlyphSilhouette(const char* name, float x, float y, SDL_Color color);
  void drawGlyphDisabled(const char* name, float x, float y);
  void ensureUiTexture();
  std::filesystem::path assetDir() const;
  int stripContentHeight() const;
  int stripMaxScroll() const;
  float originX(const SDL_FRect& canvas) const;
  float originY(const SDL_FRect& canvas) const;
  gani::Frame* currentFrame();
  gani::FrameSprite* hitTest(float x, float y, const Layout& layout, int& spriteIndex, int& partIndex);
  void pushUndo();
  void addSpriteToFrame(int spriteIndex);
  void addSpriteToFrameAt(int spriteIndex, int dx, int dy);
  void addMovieActorAt(int spriteIndex, int dx, int dy);
  void switchToMovieMode();
  void switchToSpriteMode();
  void removeSelected();
  void duplicateFrame();
  void insertFrame();
  void deleteFrame();
  void undo();
  void redo();
  void saveAs();
  void addSelectedToFrame();
  void removeSelectedFromFrame();
  void nudgeSelected(int dx, int dy);
  std::vector<int> rowSpriteIndices() const;
  int toolbarCellAt(float x, float y) const;
  void drawHover(const std::filesystem::path& assets);
  void drawUndoRedo(const std::filesystem::path& assets);
  void drawPopup(const std::filesystem::path& assets);
  void drawTooltip(const std::filesystem::path& assets);
  void drawMoviePanel(const std::filesystem::path& assets);
  void drawModeButton(const std::filesystem::path& assets, float appWidth);
  void drawDragGhost(const std::filesystem::path& assets);
  void drawStrip(const Layout& layout, const std::filesystem::path& assets);
  void drawCanvas(const Layout& layout, const std::filesystem::path& assets);
  void drawPanel(const Layout& layout, const std::filesystem::path& assets);
  void drawTimeline(const Layout& layout, const std::filesystem::path& assets);
  void scrubTimeline(float x, const Layout& layout);
  int movieLengthSeconds() const;
  std::string spritePanelValue(int row) const;
  std::string moviePanelValue(int row) const;
  gani::FrameSprite* selectedFrameSprite();
  void beginPanelEdit(int target, int row);
  void commitPanelEdit();
  void cancelPanelEdit();
  void drawPanelEditor(const std::filesystem::path& assets);
  void ensureTimeFont();
  void drawTimeString(float x, float y, const std::string& text);
  void ensureTickFont();
  void drawTickString(float x, float y, const std::string& text);
  void togglePanelBool(int row);
  void setPanelValue(int row, const std::string& value);

  SDL_Renderer* renderer_;
  SDL_Window* window_;
  SheetCache& sheets_;
  gani::Document document_;
  std::filesystem::path path_;
  std::filesystem::path gameRoot_;
  std::filesystem::path settingsPath_;
  int direction_{2};
  std::size_t frame_{};
  bool playing_{true};
  float zoom_{1.0f};
  float panX_{0.0f};
  float panY_{0.0f};
  int selected_{0};
  int stripScroll_{0};
  bool dragging_{false};
  int dragSprite_{-1};
  int dragIndex_{-1};
  float dragOffsetX_{0.0f};
  float dragOffsetY_{0.0f};
  float elapsed_{0.0f};
  bool dirty_{false};
  std::string status_;
  int pendingDefault_{-1};
  int selectedActor_{0};
  bool editingText_{false};
  std::string textBuffer_;
  std::vector<gani::Document> undo_;
  std::vector<gani::Document> redo_;
  enum class Popup { none, direction, sprites };
  Popup popup_{Popup::none};
  int popupScroll_{0};
  bool stripDrag_{false};
  float stripDragOffset_{0.0f};
  int tooltipTarget_{-1};
  unsigned long long tooltipSince_{0};
  std::string tooltipText_;
  bool pressed_{false};
  SDL_FRect pressedRect_{};
  Layout view_{};
  bool reflow_{false};
  float panelShift_{0.0f};
  SDL_Texture* uiTexture_{};
  bool stripPress_{false};
  bool timelineDrag_{false};
  bool timelineResize_{false};
  int timelineResizeFrame_{-1};
  float timelineResizeStartX_{0.0f};
  float timelineResizePxPerSecond_{1.0f};
  int timelineResizeStartWait_{0};
  int stripPressIndex_{-1};
  float stripPressX_{0.0f};
  float stripPressY_{0.0f};
  int stripDragSprite_{-1};
  int panelEditTarget_{0};
  int panelEditRow_{-1};
  std::string panelEditText_;
  std::string panelEditOriginal_;
  bool panelEditFresh_{false};
  bool automaticGani_{true};
  bool graal2002_{false};
  bool framePanel_{false};
  SDL_Texture* timeFont_{};
  SDL_Texture* tickFont_{};
  SDL_Texture* glyphTexture_{};
  SDL_Texture* checkerTexture_{};
};
}
