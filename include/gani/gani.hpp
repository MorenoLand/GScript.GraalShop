#pragma once
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gani {
enum class Version { gani0001, gani0002 };

struct Sprite {
  int id{};
  std::string type;
  int x{};
  int y{};
  int width{};
  int height{};
  std::string name;
};

struct Defaults {
  std::optional<std::string> head;
  std::optional<std::string> body;
  std::map<int, std::string> params;
  std::map<int, std::string> attrs;
};

struct Attach {
  int sprite{};
  int attached{};
  int dx{};
  int dy{};
};

struct FrameSprite {
  int sprite{-1};
  std::string type;
  int dx{};
  int dy{};
};

struct FrameSound {
  std::string file;
  double dx{};
  double dy{};
};

struct Frame {
  std::vector<std::vector<FrameSprite>> rows;
  int waitRepeats{};
  std::optional<FrameSound> sound;
};

struct MovieKeyframe {
  int frame{};
  std::map<std::string, std::string> values;
};

struct MovieActor {
  std::string type;
  std::string name;
  std::vector<MovieKeyframe> keyframes;
};

struct Movie {
  std::vector<MovieActor> actors;
};

struct Document {
  Version version{Version::gani0001};
  std::vector<Sprite> sprites;
  Defaults defaults;
  std::vector<Attach> attachments;
  bool singleDirection{};
  bool loop{};
  bool continuous{};
  unsigned int backColor{0x008000};
  std::optional<int> frameCount;
  std::optional<std::string> setbackTo;
  std::optional<std::string> script;
  std::optional<Movie> movie;
  std::vector<Frame> frames;
};

struct ParseError {
  std::size_t line{};
  std::string message;
};

struct ParseResult {
  std::optional<Document> document;
  std::optional<ParseError> error;
  [[nodiscard]] bool ok() const { return document.has_value(); }
  explicit operator bool() const { return ok(); }
};

ParseResult parse(std::string_view text);
ParseResult load(const std::filesystem::path& path);
std::string serialize(const Document& document);
}
