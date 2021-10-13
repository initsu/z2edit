#include "music.h"

#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>

namespace z2music {

template <typename E>
constexpr uint8_t to_byte(E e) noexcept {
  return static_cast<std::uint8_t>(e);
}

Note::Note(uint8_t value) : value_(value) {}

Note::Note(Duration d, Pitch p) : value_(to_byte(d) | to_byte(p)) {}

Note Note::from_midi(int note, int ticks) {
  if (kMidiPitchMap.count(note) == 0) {
    fprintf(stderr, "Note %d is not usable\n", note);
    exit(1);
  }

  if (kMidiDurationMap.count(ticks) == 0) {
    fprintf(stderr, "Duration %d is not usable\n", ticks);
    exit(1);
  }

  return kMidiPitchMap.at(note) | kMidiDurationMap.at(ticks);
}

Note::Duration Note::duration() const {
  return static_cast<Duration>(value_ & 0xc1);
}

void Note::duration(Note::Duration d) {
  value_ = to_byte(d) | to_byte(pitch());
}

Note::Pitch Note::pitch() const {
  return static_cast<Pitch>(value_ & 0x3e);
}

void Note::pitch(Note::Pitch p) {
  value_ = to_byte(duration()) | to_byte(p);
}

size_t Note::length() const {
  // MIDI generally uses 96 ppqn, so we'll use the same
  switch (duration()) {
    case Duration::Sixteenth:       return 96 / 4;
    case Duration::DottedQuarter:   return 96 * 3/2;
    case Duration::DottedEighth:    return 96 / 2 * 3/2;
    case Duration::Half:            return 96 * 2;
    case Duration::Eighth:          return 96 / 2;
    case Duration::EighthTriplet:   return 96 / 2 * 2/3;
    case Duration::Quarter:         return 96;
    case Duration::QuarterTriplet:  return 96 * 2/3;
  }

  return 0;
}

std::string Note::pitch_string() const {
  switch (pitch()) {
    case Pitch::Rest: return "---.";
    case Pitch::Cs3:  return "C#3.";
    case Pitch::E3:   return "E3..";
    case Pitch::G3:   return "G3..";
    case Pitch::Gs3:  return "G#3.";
    case Pitch::A3:   return "A3..";
    case Pitch::As3:  return "A#3.";
    case Pitch::B3:   return "B3..";
    case Pitch::C4:   return "C4..";
    case Pitch::Cs4:  return "C#4.";
    case Pitch::D4:   return "D4..";
    case Pitch::Ds4:  return "D#4.";
    case Pitch::E4:   return "E4..";
    case Pitch::F4:   return "F4..";
    case Pitch::Fs4:  return "F#4.";
    case Pitch::G4:   return "G4..";
    case Pitch::Gs4:  return "G#4.";
    case Pitch::A4:   return "A4..";
    case Pitch::As4:  return "A#4.";
    case Pitch::B4:   return "B4..";
    case Pitch::C5:   return "C5..";
    case Pitch::Cs5:  return "C#5.";
    case Pitch::D5:   return "D5..";
    case Pitch::Ds5:  return "D#5.";
    case Pitch::E5:   return "E5..";
    case Pitch::F5:   return "F5..";
    case Pitch::Fs5:  return "F#5.";
    case Pitch::G5:   return "G5..";
    case Pitch::A5:   return "A5..";
    case Pitch::As5:  return "A#5.";
    case Pitch::B5:   return "B5..";
  }

  return "???.";
}

Note::operator uint8_t() const {
  return value_;
}

const std::unordered_map<int, uint8_t> Note::kMidiPitchMap = {
  {  0, 0x02 }, // special pitch value for rest
  { 49, 0x3e }, { 52, 0x04 }, { 55, 0x06 }, { 56, 0x08 }, { 57, 0x0a }, { 58, 0x0c },
  { 59, 0x0e }, { 60, 0x10 }, { 61, 0x12 }, { 62, 0x14 }, { 63, 0x16 }, { 64, 0x18 },
  { 65, 0x1a }, { 66, 0x1c }, { 67, 0x1e }, { 68, 0x20 }, { 69, 0x22 }, { 70, 0x24 },
  { 71, 0x26 }, { 72, 0x28 }, { 73, 0x2a }, { 74, 0x2c }, { 75, 0x2e }, { 76, 0x30 },
  { 77, 0x32 }, { 78, 0x34 }, { 79, 0x36 }, { 81, 0x38 }, { 82, 0x3a }, { 83, 0x3c },
};

const std::unordered_map<int, uint8_t> Note::kMidiDurationMap = {
  {  6, 0x00 }, { 36, 0x01 }, { 18, 0x40 }, { 48, 0x41 },
  { 12, 0x80 }, {  8, 0x81 }, { 24, 0xc0 }, { 16, 0xc1 },
};

Pattern::Pattern() : tempo_(0x18) {
  clear();
}

Pattern::Pattern(const Rom& rom, size_t address) {
  clear();

  uint8_t header[6];
  rom.read(header, address, 6);

  tempo_ = header[0];

  size_t note_base = (header[2] << 8) + header[1] + 0x10000;

  read_notes(Channel::Pulse1, rom, note_base);

  if (header[3] > 0) read_notes(Channel::Triangle, rom, note_base + header[3]);
  if (header[4] > 0) read_notes(Channel::Pulse2, rom, note_base + header[4]);
  if (header[5] > 0) read_notes(Channel::Noise, rom, note_base + header[5]);
}

Pattern::Pattern(uint8_t tempo,
    std::vector<Note> pw1,
    std::vector<Note> pw2,
    std::vector<Note> triangle,
    std::vector<Note> noise):
tempo_(tempo) {
  clear();
  add_notes(Channel::Pulse1, pw1);
  add_notes(Channel::Pulse2, pw2);
  add_notes(Channel::Triangle, triangle);
  add_notes(Channel::Noise, noise);
}

Pattern::Pattern(uint8_t v1, uint8_t v2,
    std::vector<Note> pw1,
    std::vector<Note> pw2,
    std::vector<Note> triangle,
    std::vector<Note> noise):
tempo_(0x00), voice1_(v1), voice2_(v2) {
  clear();
  add_notes(Channel::Pulse1, pw1);
  add_notes(Channel::Pulse2, pw2);
  add_notes(Channel::Triangle, triangle);
  add_notes(Channel::Noise, noise);
}

size_t Pattern::length() const {
  return length(Channel::Pulse1);
}

void Pattern::add_notes(Pattern::Channel ch, std::vector<Note> notes) {
  for (auto n : notes) {
    notes_[ch].push_back(n);
  }
}

void Pattern::clear() {
  notes_[Channel::Pulse1].clear();
  notes_[Channel::Pulse2].clear();
  notes_[Channel::Triangle].clear();
  notes_[Channel::Noise].clear();
}

std::vector<Note> Pattern::notes(Pattern::Channel ch) const {
  return notes_.at(ch);
}

void Pattern::tempo(uint8_t tempo) {
  tempo_ = tempo;
}

uint8_t Pattern::tempo() const {
  return tempo_;
}

bool Pattern::validate() const {
  // TODO validate pattern

  // check that pw1 length is <= 16 quarter notes
  // check that other lengths are equal or valid partial lengths
  // TODO does the game handle unusual lengths?  maybe this isn't really needed

  return true;
}

bool Pattern::voiced() const {
  return tempo_ == 0x00;
}

uint8_t Pattern::voice1() const {
  return voice1_;
}

uint8_t Pattern::voice2() const {
  return voice2_;
}

void Pattern::set_voicing(uint8_t v1, uint8_t v2) {
  tempo_ = 0x00;
  voice1_ = v1;
  voice2_ = v2;
}

size_t Pattern::metadata_length() const {
  return voiced() ? 8 : 6;
}

std::vector<uint8_t> Pattern::note_data() const {
  std::vector<uint8_t> b;

  const std::array<Channel, 4> channels = {
    Channel::Pulse1,
    Channel::Pulse2,
    Channel::Triangle,
    Channel::Noise,
  };

  for (auto ch : channels) {
    const std::vector<uint8_t> c = note_data(ch);
    b.insert(b.end(), c.begin(), c.end());
  }

  return b;
}

std::vector<uint8_t> Pattern::meta_data(size_t notes) const {
  // FIXME calculate which channels need extra bytes :(
  const size_t pw1 = note_data_length(Channel::Pulse1);
  const size_t pw2 = note_data_length(Channel::Pulse2);
  const size_t tri = note_data_length(Channel::Triangle);
  const size_t noi = note_data_length(Channel::Noise);

  std::vector<uint8_t> b;
  b.reserve(metadata_length());

  b.push_back(tempo_);
  b.push_back(notes % 256);
  b.push_back(notes >> 8);
  b.push_back(tri == 0 ? 0 : pw1 + pw2);
  b.push_back(pw2 == 0 ? 0 : pw1);
  b.push_back(noi == 0 ? 0 : pw1 + pw2 + tri);

  if (voiced()) {
    b.push_back(voice1_);
    b.push_back(voice2_);
  }

  return b;
}

size_t Pattern::length(Pattern::Channel ch) const {
  size_t length = 0;
  for (auto n : notes_.at(ch)) {
    length += n.length();
  }
  return length;
}

bool Pattern::pad_note_data(Pattern::Channel ch) const {
  if (ch == Channel::Pulse1) return true;

  const size_t l = length(ch);
  return l > 0 && l < length();
}

std::vector<uint8_t> Pattern::note_data(Pattern::Channel ch) const {
  std::vector<uint8_t> b;
  b.reserve(notes_.at(ch).size() + 1);

  for (auto n : notes_.at(ch)) {
    b.push_back(n);
  }
  if (pad_note_data(ch)) b.push_back(0);

  return b;
}

size_t Pattern::note_data_length(Pattern::Channel ch) const {
  return notes_.at(ch).size() + (pad_note_data(ch) ? 1 : 0);
}

void Pattern::read_notes(Pattern::Channel ch, const Rom& rom, size_t address) {
  const size_t max_length = ch == Channel::Pulse1 ? 64 * 96 : length();
  size_t length = 0;

  while (length < max_length) {
    Note n = Note(rom.getc(address++));
    // Note data can terminate early on 00 byte
    if (n == 0x00) break;

    length += n.length();
    add_notes(ch, {n});

    // The QuarterTriplet duration has special meaning when preceeded by
    // two EighthTriplets, which differs based on a tempo flag.
    if (n.duration() == Note::Duration::QuarterTriplet) {
      const size_t i = notes_[ch].size() - 3;
      if (notes_[ch][i + 0].duration() == Note::Duration::EighthTriplet &&
          notes_[ch][i + 1].duration() == Note::Duration::EighthTriplet) {
        if (tempo_ & 0x08) {
          // If flag 0x08 is set, just count 0xc1 as a third EighthTriplet
          notes_[ch][i + 2].duration(Note::Duration::EighthTriplet);
        } else {
          // If flag 0x08 is not set, rewrite the whole sequence
          notes_[ch][i + 0].duration(Note::Duration::DottedEighth);
          notes_[ch][i + 1].duration(Note::Duration::DottedEighth);
          notes_[ch][i + 2].duration(Note::Duration::Eighth);
        }
      }
    }
  }
}

std::vector<Note> Pattern::parse_notes(const std::string& data, int transpose) {
  std::vector<Note> notes;

  int duration = 0;
  int pitch = 0;
  int octave = 0;

  size_t i = 0;

  while (i < data.length()) {
    const char c = data[i++];

    switch (c) {
      case 'C':
      case 'c':
        pitch = 1;
        break;

      case 'D':
      case 'd':
        pitch = 3;
        break;

      case 'E':
      case 'e':
        pitch = 5;
        break;

      case 'F':
      case 'f':
        pitch = 6;
        break;

      case 'G':
      case 'g':
        pitch = 8;
        break;

      case 'A':
      case 'a':
        pitch = 10;
        break;

      case 'B':
        pitch = 12;
        break;

      case 'b':
        if (pitch == 0) { // B note
          pitch = 12;
        } else { // flat
          --pitch;
        }
        break;

      case '#':
      case 's':
        ++pitch;
        break;

      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
        if (octave == 0) {
          octave = c - '0';
        } else {
          duration = c - '0';
        }
        break;

      case '.':
        // ignored
        break;

      case 'x':
        // snare drum (G#3)
        pitch = 9;
        octave = 3;
        break;

      case 'r':
      case '-':
        // rest
        pitch = -1;
        octave = -1;
        break;

      case ' ':
        if (pitch && octave && duration) {
          const int note = pitch > 0 ? pitch + 12 * octave + 11 + transpose : 0;
          notes.push_back(z2music::Note::from_midi(note, 6 * duration));

          // Keep duration for later notes, but reset pitch and octave
          pitch = 0;
          octave = 0;
        }
        break;

      default:
        fprintf(stderr, "Unknown char '%c' when parsing notes\n", c);
        break;
    }
  }

  // Add final note
  if (pitch && octave && duration) {
    const int note = pitch > 0 ? pitch + 12 * octave + 11 + transpose : 0;
    notes.push_back(z2music::Note::from_midi(note, 6 * duration));
  }

  return notes;
}

Song::Song() {}

Song::Song(const Rom& rom, size_t address, size_t entry) {
  if (entry > 7) return;

  uint8_t table[8];
  rom.read(table, address, 8);

  std::unordered_map<uint8_t, size_t> offset_map;
  std::vector<size_t> seq;
  size_t n = 0;

  for (size_t i = 0; true; ++i) {
    uint8_t offset = rom.getc(address + table[entry] + i);

    if (offset == 0) break;
    if (offset_map.find(offset) == offset_map.end()) {
      offset_map[offset] = n++;
      add_pattern(Pattern(rom, address + offset));
    }
    append_sequence(offset_map.at(offset));
  }
}

void Song::add_pattern(const Pattern& pattern) {
  patterns_.push_back(pattern);
}

void Song::set_sequence(const std::vector<size_t>& seq) {
  sequence_ = seq;
}

void Song::append_sequence(size_t n) {
  sequence_.push_back(n);
}

std::vector<uint8_t> Song::sequence_data(uint8_t first) const {
  std::vector<uint8_t> b;
  b.reserve(sequence_.size() + 1);

  std::vector<uint8_t> offsets;
  offsets.reserve(sequence_.size());
  for (const auto& p : patterns_) {
    offsets.push_back(first);
    first += p.metadata_length();
  }

  for (size_t n : sequence_) {
    b.push_back(offsets[n]);
  }

  b.push_back(0);

  return b;
}

size_t Song::sequence_length() const {
  return sequence_.size();
}

size_t Song::pattern_count() const {
  return patterns_.size();
}

size_t Song::metadata_length() const {
  size_t length = sequence_length() + 1;
  for (const auto& p : patterns_) {
    length += p.metadata_length();
  }
  return length;
}

void Song::clear() {
  patterns_.clear();
  sequence_.clear();
}

std::vector<Pattern> Song::patterns() {
  return patterns_;
}

Pattern* Song::at(size_t i) {
  if (i < 0 || i >= sequence_.size()) return nullptr;
  return &(patterns_.at(sequence_.at(i)));
}

const Pattern* Song::at(size_t i) const {
  if (i < 0 || i >= sequence_.size()) return nullptr;
  return &(patterns_.at(sequence_.at(i)));
}

char z2_decode_(uint8_t data) {
  switch (data) {
    case 0x07: return '!';
    case 0xce: return '/';
    case 0xcf: return '.';
    case 0xf4: return ' ';
    case 0xf5: return ' ';
  }

  if (data >= 0xd0 && data <= 0xd9) return data - 0xa0;
  if (data >= 0xda && data <= 0xf3) return data - 0x99;

  std::cerr << "Cannot decode byte '" << data << "'" << std::endl;

  return 0x00;
}

uint8_t z2_encode_(char data) {
  switch (data) {
    case ' ': return 0xf4;
    case '.': return 0xcf;
    case '/': return 0xce;
    case '!': return 0x07;
  }

  if (data >= 0x30 && data <= 0x39) return data + 0xa0;
  if (data >= 0x41 && data <= 0x5a) return data + 0x99;
  if (data >= 0x61 && data <= 0x7a) return data + 0x79;

  std::cerr << "Cannot encode character '" << data << "'" << std::endl;

  return 0x00;
}

std::string parse_string_(const Rom& rom, size_t address) {
  const uint8_t flag = rom.getc(address);
  if (flag != 0x22) return "";

  const uint8_t length = rom.getc(address + 2);

  std::string s = "";
  for (uint8_t i = 0; i < length; ++i) {
    s.append(1, z2_decode_(rom.getc(address + i + 3)));
  }

  fprintf(stderr, "Found string at %06lx - [%s]\n", address, s.c_str());

  return s;
}

size_t write_string_(Rom& rom, size_t address, const std::string& s) {
  uint8_t length = s.length();
  rom.putc(address, length);
  for (uint8_t i = 0; i < length; ++i) {
    rom.putc(address + i + 1, z2_encode_(s.at(i)));
  }
  return address + length + 1;
}

Credits::Credits() {}

Credits::Credits(const Rom& rom) {
  for (size_t i = 0; i < kCreditsPages; ++i) {
    const size_t addr = kCreditsTableAddress + 4 * i;

    const size_t title = rom.getw(addr) + kCreditsBankOffset;
    const size_t names = rom.getw(addr + 2) + kCreditsBankOffset;

    Text t;
    t.title = parse_string_(rom, title);
    t.name1 = parse_string_(rom, names);
    t.name2 = parse_string_(rom, names + t.name1.length() + 3);

    credits_[i] = t;
  }
}

void Credits::set(size_t page, const Credits::Text& text) {
  if (page < kCreditsPages) credits_[page] = text;
}

Credits::Text Credits::get(size_t page) const {
  if (page < kCreditsPages) return credits_[page];
  return {"","",""};
}

void Credits::commit(Rom& rom) const {
  size_t table = kCreditsTableAddress;
  size_t data = kCreditsTableAddress + 4 * kCreditsPages;

  for (size_t i = 0; i < kCreditsPages; ++i) {
    // Add entry for title
    if (credits_[i].title.length() > 0) {
      rom.putw(table, data - kCreditsBankOffset);
      rom.write(data, {0x22, 0x47});
      data = write_string_(rom, data + 2, credits_[i].title);
      rom.putc(data++, 0xff);
    } else {
      // optimization if the title is empty, just point to the previous title
      rom.putw(table, rom.getw(table - 4));
    }

    // Add entry for name1
    rom.putw(table + 2, data - kCreditsBankOffset);
    rom.write(data, {0x22, 0x8b});
    data = write_string_(rom, data + 2, credits_[i].name1);

    // Add entry for name2 if present
    if (credits_[i].name2.length() > 0) {
      rom.write(data, {0x22, 0xcb});
      data = write_string_(rom, data + 2, credits_[i].name2);
    }

    rom.putc(data++, 0xff);
    table += 4;
  }
}

Rom::Rom(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary);
  if (file.is_open()) {
    file.read(reinterpret_cast<char *>(&header_[0]), kHeaderSize);
    file.read(reinterpret_cast<char *>(&data_[0]), kRomSize);

    title_screen_table      = get_song_table_address(kTitleScreenLoader);
    overworld_song_table    = get_song_table_address(kOverworldLoader);
    town_song_table         = get_song_table_address(kTownLoader);
    palace_song_table       = get_song_table_address(kPalaceLoader);
    great_palace_song_table = get_song_table_address(kGreatPalaceLoader);

    songs_[SongTitle::TitleIntro] = Song(*this, title_screen_table, 0);
    songs_[SongTitle::TitleThemeStart] = Song(*this, title_screen_table, 1);
    songs_[SongTitle::TitleThemeBuildup] = Song(*this, title_screen_table, 2);
    songs_[SongTitle::TitleThemeMain] = Song(*this, title_screen_table, 3);
    songs_[SongTitle::TitleThemeBreakdown] = Song(*this, title_screen_table, 4);

    songs_[SongTitle::OverworldIntro] = Song(*this, overworld_song_table, 0);
    songs_[SongTitle::OverworldTheme] = Song(*this, overworld_song_table, 1);
    songs_[SongTitle::BattleTheme] = Song(*this, overworld_song_table, 2);
    songs_[SongTitle::CaveItemFanfare] = Song(*this, overworld_song_table, 4);

    songs_[SongTitle::TownIntro] = Song(*this, town_song_table, 0);
    songs_[SongTitle::TownTheme] = Song(*this, town_song_table, 1);
    songs_[SongTitle::HouseTheme] = Song(*this, town_song_table, 2);
    songs_[SongTitle::TownItemFanfare] = Song(*this, town_song_table, 4);

    songs_[SongTitle::PalaceIntro] = Song(*this, palace_song_table, 0);
    songs_[SongTitle::PalaceTheme] = Song(*this, palace_song_table, 1);
    songs_[SongTitle::BossTheme] = Song(*this, palace_song_table, 3);
    songs_[SongTitle::PalaceItemFanfare] = Song(*this, palace_song_table, 4);
    songs_[SongTitle::CrystalFanfare] = Song(*this, palace_song_table, 6);

    songs_[SongTitle::GreatPalaceIntro] = Song(*this, great_palace_song_table, 0);
    songs_[SongTitle::GreatPalaceTheme] = Song(*this, great_palace_song_table, 1);
    songs_[SongTitle::ZeldaTheme] = Song(*this, great_palace_song_table, 2);
    songs_[SongTitle::CreditsTheme] = Song(*this, great_palace_song_table, 3);
    songs_[SongTitle::GreatPalaceItemFanfare] = Song(*this, great_palace_song_table, 4);
    songs_[SongTitle::TriforceFanfare] = Song(*this, great_palace_song_table, 5);
    songs_[SongTitle::FinalBossTheme] = Song(*this, great_palace_song_table, 6);

    credits_ = Credits(*this);
  }
}

uint8_t Rom::getc(size_t address) const {
  if (address > kRomSize) return 0xff;
  return data_[address];
}

uint16_t Rom::getw(size_t address) const {
  return getc(address) + (getc(address + 1) << 8);
}

void Rom::read(uint8_t* buffer, size_t address, size_t length) const {
  // Could use std::copy or std::memcpy but this handles out of range addresses
  for (size_t i = 0; i < length; ++i) {
    buffer[i] = getc(address + i);
  }
}

void Rom::putc(size_t address, uint8_t data) {
  if (address > kRomSize) return;
  data_[address] = data;
}

void Rom::putw(size_t address, uint16_t data) {
  putc(address, data & 0xff);
  putc(address + 1, data >> 8);
}

void Rom::write(size_t address, std::vector<uint8_t> data) {
  for (size_t i = 0; i < data.size(); ++i) {
    putc(address + i, data[i]);
  }
}

bool Rom::commit() {
  commit(title_screen_table, {
      SongTitle::TitleIntro,
      SongTitle::TitleThemeStart,
      SongTitle::TitleThemeBuildup,
      SongTitle::TitleThemeMain,
      SongTitle::TitleThemeBreakdown});

  commit(overworld_song_table, {
      SongTitle::OverworldIntro,
      SongTitle::OverworldTheme,
      SongTitle::BattleTheme,
      SongTitle::CaveItemFanfare});

  commit(town_song_table, {
      SongTitle::TownIntro,
      SongTitle::TownTheme,
      SongTitle::HouseTheme,
      SongTitle::TownItemFanfare});

  commit(palace_song_table, {
      SongTitle::PalaceIntro,
      SongTitle::PalaceTheme,
      SongTitle::BossTheme,
      SongTitle::PalaceItemFanfare,
      SongTitle::CrystalFanfare});

  commit(great_palace_song_table, {
      SongTitle::GreatPalaceIntro,
      SongTitle::GreatPalaceTheme,
      SongTitle::ZeldaTheme,
      SongTitle::CreditsTheme,
      SongTitle::GreatPalaceItemFanfare,
      SongTitle::TriforceFanfare,
      SongTitle::FinalBossTheme});

  credits_.commit(*this);

  return true;
}

void Rom::save(const std::string& filename) {
  if (commit()) {
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
      file.write(reinterpret_cast<char *>(&header_[0]), kHeaderSize);
      file.write(reinterpret_cast<char *>(&data_[0]), kRomSize);
    }
  }
}

void Rom::move_song_table(size_t loader_address, uint16_t base_address) {
  if (loader_address == kTitleScreenLoader) {
    title_screen_table = base_address + 0x010000;
  } else if (loader_address == kOverworldLoader) {
    overworld_song_table = base_address + 0x010000;
  } else if (loader_address == kTownLoader) {
    town_song_table = base_address + 0x010000;
  } else if (loader_address == kPalaceLoader) {
    palace_song_table = base_address + 0x010000;
  } else if (loader_address == kGreatPalaceLoader) {
    great_palace_song_table = base_address + 0x010000;
  } else {
    fprintf(stderr, "Unsure what loader is at %06lx, need manual update\n", loader_address);
  }

  const uint16_t old_base = getw(loader_address + 1);

  // Rewind a bit because there is a load before the main section
  loader_address -= 11;

  while (true) {
    const uint8_t byte = getc(loader_address);
    if (byte == 0xb9) {
      const uint16_t addr = getw(loader_address + 1);
      const uint16_t new_addr = base_address + addr - old_base;
      fprintf(stderr, "Found LDA, replacing %04X with %04X\n", addr, new_addr);
      putw(loader_address + 1, new_addr);
      loader_address += 3;
    } else if (byte == 0x4c) {
      fprintf(stderr, "Found JMP, done moving table\n");
      break;
    } else if (loader_address >= 0x19c74) {
      fprintf(stderr, "Got to music reset code, done moving table\n");
      break;
    } else {
      ++loader_address;
    }
  }
}

Song* Rom::song(Rom::SongTitle title) {
  return &songs_[title];
}

Credits* Rom::credits() {
  return &credits_;
}

void Rom::commit(size_t address, std::vector<Rom::SongTitle> songs) {
  std::array<uint8_t, 8> table;

  // TODO make these changeable.
  // This will require rearchitecting things so that there is a Score object
  // which is a list of 8 (possibly duplicate) songs.  For now, it's just
  // hardcode which songs are where in each table.
  if (address == title_screen_table) {
    table = {0, 1, 2, 3, 4, 5, 5, 5 };
  } else if (address == overworld_song_table || address == town_song_table) {
    table = {0, 1, 2, 2, 3, 4, 4, 4 };
  } else if (address == palace_song_table) {
    table = { 0, 1, 1, 2, 3, 5, 4, 5 };
  } else if (address == great_palace_song_table) {
    table = { 0, 1, 2, 3, 4, 5, 6, 7 };
  } else {
    return;
  }

  /**************
   * SONG TABLE *
   **************/

  uint8_t offset = 8;
  std::vector<uint8_t> offsets;
  offsets.reserve(8);

  // Calculate song offset table
  for (auto s : songs) {
    offsets.push_back(offset);
    fprintf(stderr, "Offset for next song: %02x\n", offset);
    offset += songs_.at(s).sequence_length() + 1;
  }

  // One extra offset for the "empty" song at the end
  // We could save a whole byte by pointing this at the end of some other
  // sequence but it's kind of nice to see the double 00 to mean the end of the
  // sequence data.
  offsets.push_back(offset);

  // Write song table to ROM
  for (size_t i = 0; i < 8; ++i) {
    putc(address + i, offsets[table[i]]);
  }

  /******************
   * SEQUENCE TABLE *
   ******************/

  const uint8_t first_pattern = offset + 1;
  uint8_t seq_offset = 8;
  uint8_t pat_offset = first_pattern;

  for (auto s : songs) {
    const auto& song = songs_.at(s);

    fprintf(stderr, "Writing seq at %02x with pat at %02x: ", seq_offset, pat_offset);
    const std::vector<uint8_t> seq = song.sequence_data(pat_offset);
    write(address + seq_offset, seq);

    for (auto b : seq) fprintf(stderr, "%02x ", b);
    fprintf(stderr, "\n");

    for (size_t i = 0; i < song.pattern_count(); ++i) {
      pat_offset += song.at(i)->metadata_length();
    }

    seq_offset += seq.size();
  }

  // Write an empty sequence for the empty song
  putc(address + seq_offset, 0);

  /*******************************
   * PATTERN TABLE AND NOTE DATA *
   *******************************/

  size_t note_address = pat_offset + address;
  pat_offset = first_pattern;

  fprintf(stderr, "Note data to start at %06lx\n", note_address);

  for (auto s : songs) {
    for (auto p : songs_.at(s).patterns()) {
      const std::vector<uint8_t> note_data = p.note_data();
      const std::vector<uint8_t> meta_data = p.meta_data(note_address);

      fprintf(stderr, "Pattern at %06lx, notes at %06lx\n", address + pat_offset, note_address);
      fprintf(stderr, "Pattern metadata: ");
      for (size_t i = 0; i < meta_data.size(); i += 2) {
        fprintf(stderr, "%02x%02x ", meta_data[i], meta_data[i + 1]);
      }
      fprintf(stderr, "\n");

      write(address + pat_offset, meta_data);
      write(note_address, note_data);

      pat_offset += meta_data.size();
      note_address += note_data.size();
    }
  }
}

size_t Rom::get_song_table_address(size_t loader_address) const {
  // Ensure that we are seing an LDA $addr,y instruction
  assert(getc(loader_address) == 0xb9);

  // Add the bank offset to the address read
  const size_t addr = getw(loader_address + 1) + 0x10000;

  fprintf(stderr, "Got address %06lx, from LDA $%04lX,y at %06lx\n", addr, (addr & 0xffff), loader_address);
  return addr;
}

} // namespace z2music
