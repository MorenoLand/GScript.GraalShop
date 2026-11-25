#include "gani/gani.hpp"
#include <cctype>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <utility>

namespace gani {
namespace {
constexpr std::size_t kNoLine = 0;

ParseResult failure(std::size_t line, std::string message) {
  return ParseResult{std::nullopt, ParseError{line, std::move(message)}};
}

std::string_view trim(std::string_view text) {
  while (!text.empty() && (text.front() == ' ' || text.front() == '\t' || text.front() == '\r')) text.remove_prefix(1);
  while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r')) text.remove_suffix(1);
  return text;
}

std::vector<std::string_view> splitWords(std::string_view text) {
  std::vector<std::string_view> words;
  std::size_t cursor = 0;
  while (cursor < text.size()) {
    while (cursor < text.size() && (text[cursor] == ' ' || text[cursor] == '\t')) ++cursor;
    const std::size_t begin = cursor;
    while (cursor < text.size() && text[cursor] != ' ' && text[cursor] != '\t') ++cursor;
    if (begin != cursor) words.push_back(text.substr(begin, cursor - begin));
  }
  return words;
}

bool parseInteger(std::string_view token, int& value) {
  if (token.empty()) return false;
  const char* begin = token.data();
  const char* end = begin + token.size();
  const auto result = std::from_chars(begin, end, value);
  return result.ec == std::errc{} && result.ptr == end;
}

bool parsePositiveIndex(std::string_view token, std::string_view prefix, int& index) {
  if (!token.starts_with(prefix)) return false;
  const std::string_view digits = token.substr(prefix.size());
  return parseInteger(digits, index) && index > 0;
}

std::vector<std::string_view> splitLines(std::string_view text) {
  std::vector<std::string_view> lines;
  std::size_t begin = 0;
  while (begin <= text.size()) {
    const std::size_t end = text.find('\n', begin);
    if (end == std::string_view::npos) {
      lines.push_back(text.substr(begin));
      break;
    }
    lines.push_back(text.substr(begin, end - begin));
    begin = end + 1;
  }
  return lines;
}

bool spriteExists(const Document& document, int id) {
  for (const Sprite& sprite : document.sprites)
    if (sprite.id == id) return true;
  return false;
}

bool parseNumber(std::string_view token, double& value) {
  if (token.empty()) return false;
  const std::string text(token);
  char* end = nullptr;
  value = std::strtod(text.c_str(), &end);
  return end != text.c_str() && *end == '\0';
}

bool parseMovieKeyframe(std::string_view line, MovieKeyframe& keyframe, std::string& message) {
  std::string_view rest = trim(line);
  if (!rest.starts_with("FRAME")) {
    message = "movie keyframe must start with FRAME";
    return false;
  }
  rest = trim(rest.substr(5));
  const std::size_t space = rest.find_first_of(" \t");
  if (!parseInteger(rest.substr(0, space), keyframe.frame)) {
    message = "movie keyframe requires a frame number";
    return false;
  }
  if (space == std::string_view::npos) return true;
  rest = trim(rest.substr(space));
  while (!rest.empty()) {
    std::size_t comma = std::string_view::npos;
    for (std::size_t probe = rest.find(','); probe != std::string_view::npos; probe = rest.find(',', probe + 1)) {
      const std::size_t next = rest.find(',', probe + 1);
      const std::string_view segment =
          rest.substr(probe + 1, (next == std::string_view::npos ? rest.size() : next) - probe - 1);
      if (segment.find('=') != std::string_view::npos) {
        comma = probe;
        break;
      }
    }
    const std::string_view item = trim(rest.substr(0, comma));
    if (!item.empty()) {
      const std::size_t equals = item.find('=');
      if (equals == std::string_view::npos) {
        keyframe.values[std::string(trim(item))] = std::string();
      } else {
        keyframe.values[std::string(trim(item.substr(0, equals)))] = std::string(trim(item.substr(equals + 1)));
      }
    }
    if (comma == std::string_view::npos) break;
    rest = rest.substr(comma + 1);
  }
  return true;
}

bool resolveSpriteToken(const Document& document, std::string_view token, int& id) {
  if (parseInteger(token, id)) return true;
  for (const Sprite& sprite : document.sprites) {
    if (sprite.type.size() != token.size()) continue;
    bool matches = true;
    for (std::size_t index = 0; index < token.size() && matches; ++index)
      matches = std::tolower(static_cast<unsigned char>(sprite.type[index])) ==
                std::tolower(static_cast<unsigned char>(token[index]));
    if (matches) {
      id = sprite.id;
      return true;
    }
  }
  return false;
}

bool parseFrameRow(const Document& document, std::string_view line, std::vector<FrameSprite>& parts, std::string& message) {
  std::size_t begin = 0;
  while (begin <= line.size()) {
    const std::size_t comma = line.find(',', begin);
    const std::size_t end = comma == std::string_view::npos ? line.size() : comma;
    const std::string_view part = trim(line.substr(begin, end - begin));
    if (part.empty()) {
      message = "empty sprite entry in frame row";
      return false;
    }
    const std::vector<std::string_view> fields = splitWords(part);
    if (fields.size() != 3) {
      message = "frame sprite entry must contain a sprite id and two offsets";
      return false;
    }
    FrameSprite entry;
    if (parseInteger(fields[0], entry.sprite)) {
      entry.type.clear();
    } else if (resolveSpriteToken(document, fields[0], entry.sprite)) {
      entry.type.clear();
    } else {
      entry.sprite = -1;
      entry.type = std::string(fields[0]);
    }
    if (!parseInteger(fields[1], entry.dx) || !parseInteger(fields[2], entry.dy)) {
      message = "frame sprite must name a sprite id or type with two integer offsets";
      return false;
    }
    parts.push_back(entry);
    if (comma == std::string_view::npos) break;
    begin = comma + 1;
  }
  return true;
}
}

ParseResult parse(std::string_view text) {
  const std::vector<std::string_view> lines = splitLines(text);
  Document document;
  std::size_t firstLine = 0;
  while (firstLine < lines.size() && trim(lines[firstLine]).empty()) ++firstLine;
  if (firstLine == lines.size()) return failure(1, "empty document");
  const std::string_view header = trim(lines[firstLine]);
  if (header.starts_with("GANI")) {
    if (header == "GANI0001") document.version = Version::gani0001;
    else if (header == "GANI0002") document.version = Version::gani0002;
    else return failure(firstLine + 1, "unsupported header " + std::string(header));
    ++firstLine;
  }

  enum class Section { definitions, frames, movie, finished };
  Section section = Section::definitions;
  std::size_t expectedRows = 4;
  bool hasSetbackTo = false;
  std::vector<std::vector<FrameSprite>> pendingRows;
  int pendingWait = 0;
  bool pendingHasWait = false;
  std::optional<FrameSound> pendingSound;
  std::size_t lastFrameLine = 1;

  const auto clearFrame = [&] {
    pendingRows.clear();
    pendingWait = 0;
    pendingHasWait = false;
    pendingSound.reset();
  };

  const auto finishFrame = [&](std::size_t line) -> std::optional<ParseError> {
    if (pendingRows.size() != expectedRows) {
      return ParseError{line, "frame must contain " + std::to_string(expectedRows) + " row(s), found " + std::to_string(pendingRows.size())};
    }
    document.frames.push_back(Frame{pendingRows, pendingWait, pendingSound});
    clearFrame();
    return std::nullopt;
  };

  for (std::size_t index = firstLine; index < lines.size(); ++index) {
    const std::size_t lineNumber = index + 1;
    const std::string_view line = trim(lines[index]);
    if (line.empty()) {
      if (section == Section::frames && !pendingRows.empty()) {
        if (auto error = finishFrame(lineNumber)) return ParseResult{std::nullopt, std::move(error)};
        lastFrameLine = lineNumber;
      }
      continue;
    }
    if (section == Section::finished) return failure(lineNumber, "unexpected content after ANIEND");
    const std::vector<std::string_view> words = splitWords(line);
    const std::string_view directive = words.front();

    if (section == Section::movie) {
      if (directive == "MOVIEEND") {
        if (words.size() != 1) return failure(lineNumber, "MOVIEEND does not take arguments");
        section = Section::finished;
        continue;
      }
      if (directive == "ACTOR") {
        if (words.size() != 3) return failure(lineNumber, "ACTOR requires a type and a name");
        document.movie->actors.push_back(MovieActor{std::string(words[1]), std::string(words[2]), {}});
        continue;
      }
      if (directive == "ACTOREND") {
        if (words.size() != 1) return failure(lineNumber, "ACTOREND does not take arguments");
        continue;
      }
      if (directive == "FRAME") {
        if (document.movie->actors.empty()) return failure(lineNumber, "FRAME without ACTOR");
        MovieKeyframe keyframe;
        std::string message;
        if (!parseMovieKeyframe(line, keyframe, message)) return failure(lineNumber, message);
        document.movie->actors.back().keyframes.push_back(std::move(keyframe));
        continue;
      }
      return failure(lineNumber, "unexpected movie directive " + std::string(directive));
    }

    if (section == Section::frames) {
      if (directive == "ANIEND") {
        if (words.size() != 1) return failure(lineNumber, "ANIEND does not take arguments");
        if (!pendingRows.empty()) {
          if (auto error = finishFrame(lineNumber)) return ParseResult{std::nullopt, std::move(error)};
        }
        if (document.frames.empty()) return failure(lineNumber, "animation contains no frames");
        section = Section::finished;
        continue;
      }
      if (directive.starts_with("WAIT")) {
        if (pendingRows.size() != expectedRows) return failure(lineNumber, "WAIT must follow a complete frame");
        if (pendingHasWait) return failure(lineNumber, "duplicate WAIT in frame");
        int value = 0;
        const bool parsed = directive == "WAIT" ? words.size() == 2 && parseInteger(words[1], value)
                                                : words.size() == 1 && parseInteger(directive.substr(4), value);
        if (!parsed || value < 0) return failure(lineNumber, "WAIT value must be a non-negative integer");
        pendingWait = value;
        pendingHasWait = true;
        continue;
      }
      if (directive == "PLAYSOUND") {
        if (pendingRows.size() != expectedRows) return failure(lineNumber, "PLAYSOUND must follow a complete frame");
        if (pendingSound) return failure(lineNumber, "duplicate PLAYSOUND in frame");
        if (words.size() != 4) return failure(lineNumber, "PLAYSOUND requires a file and two offsets");
        FrameSound sound{std::string(words[1]), 0.0, 0.0};
        if (!parseNumber(words[2], sound.dx) || !parseNumber(words[3], sound.dy)) return failure(lineNumber, "PLAYSOUND offsets must be numbers");
        pendingSound = std::move(sound);
        continue;
      }
      if (pendingHasWait || pendingSound) return failure(lineNumber, "frame rows must precede WAIT and PLAYSOUND");
      if (pendingRows.size() >= expectedRows) return failure(lineNumber, "frame contains too many rows");
      std::vector<FrameSprite> parts;
      std::string message;
      if (!parseFrameRow(document, line, parts, message)) return failure(lineNumber, message);
      for (const FrameSprite& part : parts)
        if (part.sprite >= 0 && !spriteExists(document, part.sprite))
          return failure(lineNumber, "frame references undefined sprite " + std::to_string(part.sprite));
      pendingRows.push_back(std::move(parts));
      lastFrameLine = lineNumber;
      continue;
    }

    if (directive == "SCRIPT") {
      if (document.script) return failure(lineNumber, "duplicate SCRIPT block");
      std::string script;
      std::size_t cursor = index + 1;
      for (; cursor < lines.size(); ++cursor) {
        if (trim(lines[cursor]) == "SCRIPTEND") break;
        if (!script.empty()) script.push_back('\n');
        script.append(lines[cursor]);
      }
      if (cursor == lines.size()) return failure(lineNumber, "missing SCRIPTEND");
      document.script = std::move(script);
      index = cursor;
      continue;
    }
    if (directive == "MOVIE") {
      if (words.size() != 1) return failure(lineNumber, "MOVIE does not take arguments");
      if (document.movie) return failure(lineNumber, "duplicate MOVIE block");
      document.movie = Movie{};
      section = Section::movie;
      continue;
    }
    if (directive == "ATTACHSPRITE") {
      if (words.size() != 5) return failure(lineNumber, "ATTACHSPRITE requires two sprite ids and two offsets");
      Attach attach;
      if (!parseInteger(words[1], attach.sprite) || !parseInteger(words[2], attach.attached) ||
          !parseInteger(words[3], attach.dx) || !parseInteger(words[4], attach.dy))
        return failure(lineNumber, "ATTACHSPRITE values must be integers");
      if (!spriteExists(document, attach.sprite))
        return failure(lineNumber, "ATTACHSPRITE references undefined sprite " + std::to_string(attach.sprite));
      if (!spriteExists(document, attach.attached))
        return failure(lineNumber, "ATTACHSPRITE references undefined sprite " + std::to_string(attach.attached));
      document.attachments.push_back(attach);
      continue;
    }
    if (directive == "FRAMES") {
      if (words.size() != 2) return failure(lineNumber, "FRAMES requires a frame count");
      int count = 0;
      if (!parseInteger(words[1], count) || count < 0) return failure(lineNumber, "FRAMES count must be a non-negative integer");
      if (document.frameCount) return failure(lineNumber, "duplicate FRAMES");
      document.frameCount = count;
      continue;
    }
    if (directive == "SPRITE") {
      if (words.size() < 7) return failure(lineNumber, "SPRITE requires an id, type, x, y, width and height");
      Sprite sprite;
      if (!parseInteger(words[1], sprite.id) || !parseInteger(words[3], sprite.x) || !parseInteger(words[4], sprite.y) ||
          !parseInteger(words[5], sprite.width) || !parseInteger(words[6], sprite.height)) {
        return failure(lineNumber, "SPRITE id, x, y, width and height must be integers");
      }
      if (spriteExists(document, sprite.id)) return failure(lineNumber, "duplicate sprite id " + std::to_string(sprite.id));
      sprite.type = std::string(words[2]);
      for (std::size_t word = 7; word < words.size(); ++word) {
        if (!sprite.name.empty()) sprite.name.push_back(' ');
        sprite.name.append(words[word]);
      }
      document.sprites.push_back(std::move(sprite));
      continue;
    }
    if (directive == "DEFAULTHEAD" || directive == "DEFAULTBODY") {
      if (words.size() > 2) return failure(lineNumber, std::string(directive) + " takes at most one image name");
      const bool isHead = directive == "DEFAULTHEAD";
      std::optional<std::string>& target = isHead ? document.defaults.head : document.defaults.body;
      if (target) return failure(lineNumber, "duplicate " + std::string(directive));
      target = words.size() == 2 ? std::string(words[1]) : std::string();
      continue;
    }
    if (directive.starts_with("DEFAULTPARAM")) {
      int parameterIndex = 0;
      if (!parsePositiveIndex(directive, "DEFAULTPARAM", parameterIndex)) return failure(lineNumber, "DEFAULTPARAM requires a positive numeric suffix");
      if (words.size() != 2) return failure(lineNumber, "DEFAULTPARAM requires one value");
      if (!document.defaults.params.emplace(parameterIndex, std::string(words[1])).second) return failure(lineNumber, "duplicate DEFAULTPARAM" + std::to_string(parameterIndex));
      continue;
    }
    if (directive.starts_with("DEFAULTATTR")) {
      int attributeIndex = 0;
      if (!parsePositiveIndex(directive, "DEFAULTATTR", attributeIndex)) return failure(lineNumber, "DEFAULTATTR requires a positive numeric suffix");
      if (words.size() != 2) return failure(lineNumber, "DEFAULTATTR requires one value");
      if (!document.defaults.attrs.emplace(attributeIndex, std::string(words[1])).second) return failure(lineNumber, "duplicate DEFAULTATTR" + std::to_string(attributeIndex));
      continue;
    }
    if (directive == "SINGLEDIRECTION" || directive == "LOOP" || directive == "CONTINUOUS") {
      if (words.size() != 1) return failure(lineNumber, std::string(directive) + " does not take arguments");
      if (directive == "SINGLEDIRECTION") {
        document.singleDirection = true;
        expectedRows = 1;
      } else if (directive == "LOOP") {
        document.loop = true;
      } else {
        document.continuous = true;
      }
      continue;
    }
    if (directive == "SETBACKTO") {
      if (words.size() != 2) return failure(lineNumber, "SETBACKTO requires one animation name");
      if (hasSetbackTo) return failure(lineNumber, "duplicate SETBACKTO");
      document.setbackTo = std::string(words[1]);
      hasSetbackTo = true;
      continue;
    }
    if (directive == "BACKCOLOR") {
      if (words.size() != 2) return failure(lineNumber, "BACKCOLOR requires one value");
      char* end = nullptr;
      const unsigned long value = std::strtoul(std::string(words[1]).c_str(), &end, 16);
      if (end == nullptr || *end != '\0') return failure(lineNumber, "BACKCOLOR must be hexadecimal");
      document.backColor = static_cast<unsigned int>(value & 0xFFFFFFu);
      continue;
    }
    if (directive == "ANI") {
      if (words.size() != 1) return failure(lineNumber, "ANI does not take arguments");
      section = Section::frames;
      continue;
    }
    if (directive == "ANIEND") return failure(lineNumber, "ANIEND without a matching ANI");
    return failure(lineNumber, "unsupported directive " + std::string(directive));
  }

  if (section == Section::frames) return failure(lastFrameLine, "missing ANIEND");
  if (section == Section::movie) return failure(lines.size(), "missing MOVIEEND");
  if (section == Section::definitions) return failure(lines.size(), "missing ANI section");
  return ParseResult{std::move(document), std::nullopt};
}

ParseResult load(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) return failure(kNoLine, "unable to open " + path.string());
  const std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  return parse(text);
}

std::string serialize(const Document& document) {
  const auto number = [](double value) {
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%g", value);
    return std::string(buffer);
  };
  std::string out = document.version == Version::gani0002 ? "GANI0002\n" : "GANI0001\n";
  for (const Sprite& sprite : document.sprites) {
    out += "SPRITE " + std::to_string(sprite.id) + " " + sprite.type + " " + std::to_string(sprite.x) + " " +
           std::to_string(sprite.y) + " " + std::to_string(sprite.width) + " " + std::to_string(sprite.height);
    if (!sprite.name.empty()) out += " " + sprite.name;
    out += '\n';
  }
  for (const Attach& attach : document.attachments)
    out += "ATTACHSPRITE " + std::to_string(attach.sprite) + " " + std::to_string(attach.attached) + " " +
           std::to_string(attach.dx) + " " + std::to_string(attach.dy) + '\n';
  if (document.defaults.head) out += "DEFAULTHEAD " + *document.defaults.head + '\n';
  if (document.defaults.body) out += "DEFAULTBODY " + *document.defaults.body + '\n';
  for (const auto& [index, value] : document.defaults.params)
    out += "DEFAULTPARAM" + std::to_string(index) + " " + value + '\n';
  for (const auto& [index, value] : document.defaults.attrs)
    out += "DEFAULTATTR" + std::to_string(index) + " " + value + '\n';
  if (document.frameCount) out += "FRAMES " + std::to_string(*document.frameCount) + '\n';
  if (document.backColor != 0x008000u) {
    char color[16];
    std::snprintf(color, sizeof color, "BACKCOLOR %06X\n", document.backColor & 0xFFFFFFu);
    out += color;
  }
  if (document.singleDirection) out += "SINGLEDIRECTION\n";
  if (document.loop) out += "LOOP\n";
  if (document.continuous) out += "CONTINUOUS\n";
  if (document.setbackTo) out += "SETBACKTO " + *document.setbackTo + '\n';
  if (document.script) out += "SCRIPT\n" + *document.script + "\nSCRIPTEND\n";
  if (!document.frames.empty()) {
    out += "ANI\n";
    for (const Frame& frame : document.frames) {
      for (const std::vector<FrameSprite>& row : frame.rows) {
        std::string line;
        for (const FrameSprite& part : row) {
          if (!line.empty()) line += ", ";
          line += part.sprite >= 0 ? std::to_string(part.sprite) : part.type;
          line += " " + std::to_string(part.dx) + " " + std::to_string(part.dy);
        }
        out += line + '\n';
      }
      if (frame.sound)
        out += "PLAYSOUND " + frame.sound->file + " " + number(frame.sound->dx) + " " + number(frame.sound->dy) + '\n';
      if (frame.waitRepeats != 0) out += "WAIT " + std::to_string(frame.waitRepeats) + '\n';
      out += '\n';
    }
    out += "ANIEND\n";
  }
  if (document.movie) {
    out += "MOVIE\n";
    for (const MovieActor& actor : document.movie->actors) {
      out += "  ACTOR " + actor.type + " " + actor.name + '\n';
      for (const MovieKeyframe& keyframe : actor.keyframes) {
        out += "    FRAME " + std::to_string(keyframe.frame);
        bool first = true;
        for (const auto& [key, value] : keyframe.values) {
          out += first ? " " : ",";
          out += key + "=" + value;
          first = false;
        }
        out += '\n';
      }
      out += "  ACTOREND\n";
    }
    out += "MOVIEEND\n";
  }
  return out;
}
}
