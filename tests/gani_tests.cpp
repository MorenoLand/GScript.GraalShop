#include "gani/gani.hpp"
#include <iostream>
#include <string_view>

namespace {
int failures = 0;

void check(bool condition, const char* expression, int line) {
  if (condition) return;
  std::cerr << "line " << line << ": check failed: " << expression << '\n';
  ++failures;
}

#define CHECK(expression) check((expression), #expression, __LINE__)

void checkRejected(std::string_view text, const char* label) {
  const gani::ParseResult result = gani::parse(text);
  const bool rejected = !result.ok() && result.error.has_value() && !result.error->message.empty();
  if (!rejected) {
    std::cerr << "expected rejection: " << label << '\n';
    ++failures;
  }
}

const gani::Document* requireDocument(const gani::ParseResult& result, const char* label) {
  if (result.ok()) return &*result.document;
  std::cerr << "expected successful parse: " << label;
  if (result.error) std::cerr << " (line " << result.error->line << ": " << result.error->message << ')';
  std::cerr << '\n';
  ++failures;
  return nullptr;
}

void testMinimalDocument() {
  const gani::ParseResult result = gani::parse(R"(GANI0001
SPRITE	0	SPRITES	0	0	24	12	shadow
DEFAULTHEAD	head19.png
DEFAULTBODY	body.png
DEFAULTPARAM1	block.png
DEFAULTATTR2	hat0.png
ANI
0 0 0
0 0 0
0 0 0
0 0 0
ANIEND
)");
  const gani::Document* document = requireDocument(result, "minimal document");
  if (!document) return;
  CHECK(document->version == gani::Version::gani0001);
  CHECK(document->sprites.size() == 1);
  CHECK(document->sprites[0].id == 0);
  CHECK(document->sprites[0].type == "SPRITES");
  CHECK(document->sprites[0].x == 0);
  CHECK(document->sprites[0].y == 0);
  CHECK(document->sprites[0].width == 24);
  CHECK(document->sprites[0].height == 12);
  CHECK(document->sprites[0].name == "shadow");
  CHECK(document->defaults.head == "head19.png");
  CHECK(document->defaults.body == "body.png");
  CHECK(document->defaults.params.size() == 1);
  CHECK(document->defaults.params.at(1) == "block.png");
  CHECK(document->defaults.attrs.size() == 1);
  CHECK(document->defaults.attrs.at(2) == "hat0.png");
  CHECK(!document->singleDirection);
  CHECK(!document->loop);
  CHECK(!document->continuous);
  CHECK(!document->setbackTo.has_value());
  CHECK(document->frames.size() == 1);
  CHECK(document->frames[0].rows.size() == 4);
  CHECK(document->frames[0].rows[0].size() == 1);
  CHECK(document->frames[0].rows[3][0].sprite == 0);
  CHECK(document->frames[0].waitRepeats == 0);
  CHECK(!document->frames[0].sound.has_value());
}

void testVersionTwoAndDirectives() {
  const gani::ParseResult result = gani::parse(R"(GANI0002
LOOP
CONTINUOUS
SINGLEDIRECTION
SETBACKTO idle
SPRITE 100 BODY -16 -28 32 64 body.png up
SPRITE 42 SPRITES 0 0 24 12 shadow
DEFAULTHEAD head19.png
ANI
100 -16 -28, 42 0 0
WAIT 2
PLAYSOUND step.wav 0 -8
ANIEND
)");
  const gani::Document* document = requireDocument(result, "version two document");
  if (!document) return;
  CHECK(document->version == gani::Version::gani0002);
  CHECK(document->loop);
  CHECK(document->continuous);
  CHECK(document->singleDirection);
  CHECK(document->setbackTo == "idle");
  CHECK(document->sprites.size() == 2);
  CHECK(document->sprites[0].type == "BODY");
  CHECK(document->sprites[0].x == -16);
  CHECK(document->sprites[0].y == -28);
  CHECK(document->sprites[0].width == 32);
  CHECK(document->sprites[0].height == 64);
  CHECK(document->sprites[0].name == "body.png up");
  CHECK(document->frames.size() == 1);
  CHECK(document->frames[0].rows.size() == 1);
  CHECK(document->frames[0].rows[0].size() == 2);
  CHECK(document->frames[0].rows[0][0].dx == -16);
  CHECK(document->frames[0].rows[0][0].dy == -28);
  CHECK(document->frames[0].rows[0][1].sprite == 42);
  CHECK(document->frames[0].waitRepeats == 2);
  CHECK(document->frames[0].sound.has_value());
  CHECK(document->frames[0].sound->file == "step.wav");
  CHECK(document->frames[0].sound->dx == 0);
  CHECK(document->frames[0].sound->dy == -8);
}

void testMultipleFramesAndDirections() {
  const gani::ParseResult result = gani::parse(R"(GANI0001
SPRITE 7 SPRITES 0 0 24 12 shadow
ANI
7 0 1
7 2 3
7 4 5
7 6 7

7 8 9
7 10 11
7 12 13
7 14 15
ANIEND
)");
  const gani::Document* document = requireDocument(result, "multiple frames");
  if (!document) return;
  CHECK(document->frames.size() == 2);
  CHECK(document->frames[0].rows.size() == 4);
  CHECK(document->frames[0].rows[0][0].dy == 1);
  CHECK(document->frames[0].rows[1][0].dx == 2);
  CHECK(document->frames[0].rows[2][0].dy == 5);
  CHECK(document->frames[1].rows[3][0].sprite == 7);
  CHECK(document->frames[1].rows[3][0].dx == 14);
  CHECK(document->frames[1].rows[3][0].dy == 15);
}

void testLoadFailure() {
  const gani::ParseResult result = gani::load("gani_model_missing_file_fixture.gani");
  CHECK(!result.ok());
  CHECK(result.error.has_value());
  CHECK(result.error && result.error->line == 0);
}

void testMalformedInput() {
  checkRejected("", "empty input");
  checkRejected("GANI0003\n", "unknown header");
  checkRejected("GANI0001\n", "missing animation");
  checkRejected(R"(GANI0001
SPRITE 0 SPRITES 0 0 24
ANI
)", "incomplete sprite definition");
  checkRejected(R"(GANI0001
SPRITE 0 SPRITES 0 nope 24 12
ANI
)", "non-numeric sprite coordinate");
  checkRejected(R"(GANI0001
SPRITE 0 SPRITES 0 0 24 12 first
SPRITE 0 SPRITES 0 0 24 12 second
ANI
)", "duplicate sprite id");
  checkRejected(R"(GANI0001
FOO
)", "unsupported directive");
  checkRejected(R"(GANI0001
SPRITE 0 SPRITES 0 0 24 12 sprite
ANI
0 0 0
0 0 0
0 0 0
0 0 0
)", "missing ANIEND");
  checkRejected(R"(GANI0001
SPRITE 0 SPRITES 0 0 24 12 sprite
ANI
0 0
0 0 0
0 0 0
0 0 0
ANIEND
)", "malformed frame row");
  checkRejected(R"(GANI0001
SPRITE 0 SPRITES 0 0 24 12 sprite
ANI
0 0 0
0 0 0
0 0 0
0 0 0
WAIT -1
ANIEND
)", "negative WAIT");
  checkRejected(R"(GANI0001
SPRITE 0 SPRITES 0 0 24 12 sprite
ANI
WAIT 1
0 0 0
0 0 0
0 0 0
0 0 0
ANIEND
)", "WAIT before frame rows");
  checkRejected(R"(GANI0001
SPRITE 0 SPRITES 0 0 24 12 sprite
ANI
1 0 0
1 0 0
1 0 0
1 0 0
ANIEND
)", "undefined sprite reference");
  checkRejected(R"(GANI0001
SPRITE 0 SPRITES 0 0 24 12 sprite
ANI
0 0 0
0 0 0
0 0 0
0 0 0
ANIEND
SPRITE 1 SPRITES 0 0 24 12 extra
)", "content after ANIEND");
  checkRejected(R"(GANI0001
SPRITE 0 SPRITES 0 0 24 12 sprite
ANI
0 0 0
0 0 0
0 0 0
0 0 0
0 0 0
ANIEND
)", "too many frame rows");
  checkRejected(R"(GANI0001
DEFAULTPARAM1 first.png
DEFAULTPARAM1 second.png
)", "duplicate parameter default");
  checkRejected("GANI0001\nDEFAULTATTR0 hat.png\n", "invalid attribute suffix");
  checkRejected(R"(GANI0001
SPRITE 0 SPRITES 0 0 24 12 sprite
ANI
0 0 0
0 0 0
0 0 0
0 0 0
PLAYSOUND first.wav 0 0
PLAYSOUND second.wav 0 0
ANIEND
)", "duplicate PLAYSOUND");
  checkRejected(R"(GANI0001
SETBACKTO idle
SETBACKTO walk
)", "duplicate SETBACKTO");
  checkRejected(R"(GANI0001
ANIEND
)", "ANIEND without ANI");
  checkRejected(R"(GANI0001
SPRITE 0 SPRITES 0 0 24 12 sprite
ANI
0 0 0
0 0 0
0 0 0
ANIEND
)", "incomplete frame directions");
}

void testExtendedDirectives() {
  const gani::ParseResult result = gani::parse(R"(GANI0001
SPRITE 0 SPRITES 0 0 24 12 shadow
SPRITE 3 PARAM1 0 0 47 67 Bubble 3
SPRITE 1 gralats.png 0 0 32 32 gralats (0, 0)
ATTACHSPRITE 3 0 2 -4
FRAMES 40
DEFAULTPARAM1 1
DEFAULTHEAD 
DEFAULTBODY 
SCRIPT
if (created) { showimg 0,light2.png,0,0; }
SCRIPTEND
ANI
PARAM1 8 16, 1 -6 5
0 0 0
0 0 0
0 0 0
PLAYSOUND lift.wav 1.5 2
WAIT4
ANIEND
)");
  const gani::Document* document = requireDocument(result, "extended directives");
  if (!document) return;
  CHECK(document->defaults.head == "");
  CHECK(document->defaults.body == "");
  CHECK(document->script.has_value());
  CHECK(document->script && document->script->find("showimg") != std::string::npos);
  CHECK(document->frameCount == 40);
  CHECK(document->attachments.size() == 1);
  CHECK(document->attachments[0].sprite == 3);
  CHECK(document->attachments[0].attached == 0);
  CHECK(document->attachments[0].dx == 2);
  CHECK(document->attachments[0].dy == -4);
  CHECK(document->frames.size() == 1);
  CHECK(document->frames[0].rows.size() == 4);
  CHECK(document->frames[0].rows[0].size() == 2);
  CHECK(document->frames[0].rows[0][0].sprite == 3);
  CHECK(document->frames[0].rows[0][0].dx == 8);
  CHECK(document->frames[0].rows[0][1].sprite == 1);
  CHECK(document->frames[0].rows[1].size() == 1);
  CHECK(document->frames[0].waitRepeats == 4);
  CHECK(document->frames[0].sound.has_value());
  CHECK(document->frames[0].sound && document->frames[0].sound->dx == 1.5);
  CHECK(document->frames[0].sound && document->frames[0].sound->dy == 2.0);
}

void testImplicitTypeSprite() {
  const gani::ParseResult result = gani::parse(R"(GANI0001
SPRITE 0 SPRITES 0 0 24 12 shadow
SINGLEDIRECTION
ANI
0 12 34, PARAM1 8 16
ANIEND
)");
  const gani::Document* document = requireDocument(result, "implicit type sprite");
  if (!document) return;
  CHECK(document->frames.size() == 1);
  CHECK(document->frames[0].rows[0].size() == 2);
  CHECK(document->frames[0].rows[0][1].sprite == -1);
  CHECK(document->frames[0].rows[0][1].type == "PARAM1");
}

void testMovieDocument() {
  const gani::ParseResult result = gani::parse(R"(GANI0002
SPRITE 20 tutorial_arrowdown.png 0 0 16 16 arrow
FRAMES 40
LOOP
MOVIE
  ACTOR SPRITE Sprite1
    FRAME 0 dx=16,dy=-29,layer=3,sprite #=20,visible=true
    FRAME 20 dx=16,dy=-24
  ACTOREND
  ACTOR TEXT Text1
    FRAME 0 bold=false,text=Touch to get information,font=Tempus Sans ITC
  ACTOREND
MOVIEEND
)");
  const gani::Document* document = requireDocument(result, "movie document");
  if (!document) return;
  CHECK(document->frameCount == 40);
  CHECK(document->frames.empty());
  CHECK(document->movie.has_value());
  CHECK(document->movie->actors.size() == 2);
  CHECK(document->movie->actors[0].type == "SPRITE");
  CHECK(document->movie->actors[0].name == "Sprite1");
  CHECK(document->movie->actors[0].keyframes.size() == 2);
  CHECK(document->movie->actors[0].keyframes[1].frame == 20);
  CHECK(document->movie->actors[0].keyframes[0].values.at("dx") == "16");
  CHECK(document->movie->actors[0].keyframes[0].values.at("sprite #") == "20");
  CHECK(document->movie->actors[1].keyframes[0].values.at("text") == "Touch to get information");
  CHECK(document->movie->actors[1].keyframes[0].values.at("font") == "Tempus Sans ITC");
}

void testHeaderlessDocument() {
  const gani::ParseResult result = gani::parse(R"(SPRITE 0 SPRITES 0 0 24 12 shadow
ANI
0 0 0
0 0 0
0 0 0
0 0 0
ANIEND
)");
  const gani::Document* document = requireDocument(result, "headerless document");
  if (!document) return;
  CHECK(document->version == gani::Version::gani0001);
  CHECK(document->sprites.size() == 1);
  CHECK(document->frames.size() == 1);
  CHECK(document->frames[0].rows.size() == 4);
}

void testSerializeRoundTrip() {
  const gani::ParseResult first = gani::parse(R"(GANI0001
SPRITE 0 SPRITES 0 0 24 12 shadow
SPRITE 42 ATTR1 96 0 48 48 hat down
DEFAULTHEAD head19.png
DEFAULTBODY body.png
DEFAULTATTR1 hat0.png
ATTACHSPRITE 42 0 1 -2
LOOP
ANI
0 12 34, 42 0 -16
0 12 34
0 12 34
0 12 34
WAIT 2
PLAYSOUND lift.wav 1.5 2
ANIEND
)");
  const gani::Document* document = requireDocument(first, "round trip source");
  if (!document) return;
  const std::string text = gani::serialize(*document);
  const gani::ParseResult second = gani::parse(text);
  const gani::Document* copy = requireDocument(second, "round trip copy");
  if (!copy) return;
  CHECK(copy->sprites.size() == document->sprites.size());
  CHECK(copy->sprites[1].id == 42);
  CHECK(copy->sprites[1].type == "ATTR1");
  CHECK(copy->defaults.head == "head19.png");
  CHECK(copy->defaults.attrs.at(1) == "hat0.png");
  CHECK(copy->attachments.size() == 1);
  CHECK(copy->attachments[0].dy == -2);
  CHECK(copy->loop);
  CHECK(copy->frames.size() == 1);
  CHECK(copy->frames[0].rows[0].size() == 2);
  CHECK(copy->frames[0].rows[0][1].dx == 0);
  CHECK(copy->frames[0].waitRepeats == 2);
  CHECK(copy->frames[0].sound && copy->frames[0].sound->dx == 1.5);
}

void testErrorLocations() {
  const gani::ParseResult duplicate = gani::parse(R"(GANI0001
SPRITE 0 SPRITES 0 0 24 12 first
SPRITE 0 SPRITES 0 0 24 12 second
)");
  CHECK(duplicate.error.has_value());
  CHECK(duplicate.error && duplicate.error->line == 3);

  const gani::ParseResult unknown = gani::parse("GANI0001\nUNKNOWN\n");
  CHECK(unknown.error.has_value());
  CHECK(unknown.error && unknown.error->line == 2);
}
}

int main() {
  testMinimalDocument();
  testVersionTwoAndDirectives();
  testMultipleFramesAndDirections();
  testLoadFailure();
  testMalformedInput();
  testExtendedDirectives();
  testImplicitTypeSprite();
  testMovieDocument();
  testHeaderlessDocument();
  testErrorLocations();
  if (failures != 0) {
    std::cerr << failures << " test check(s) failed\n";
    return 1;
  }
  std::cout << "all gani model tests passed\n";
  return 0;
}
