#include <cstdio>
#include <map>
#include <vector>

#include <gflags/gflags.h>
#include "app.h"
#include "imgui.h"
#include "imwidget/error_dialog.h"
#include "imwidget/map_connect.h"
#include "imwidget/multimap.h"
#include "nes/cpu6502.h"
#include "nes/chr_util.h"
#include "nes/text_encoding.h"
#include "proto/rominfo.pb.h"
#include "util/browser.h"
#include "util/config.h"
#include "util/os.h"
#include "util/logging.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"

#include "version.h"

#ifdef HAVE_NFD
#include "nfd.h"
#endif

DEFINE_string(emulator, "fceux", "Emulator to run for testing");
DEFINE_string(romtmp, "zelda2-test.nes", "Temporary filename for running under test");
DECLARE_bool(move_from_keepout);
DECLARE_string(config);

namespace z2util {

Z2Edit::~Z2Edit() {
    SaveUiState();
}

void Z2Edit::Init() {
    RegisterCommand("load", "Load a NES ROM or project file.", this, &Z2Edit::LoadFile);
    RegisterCommand("save", "Save a NES ROM or project file.", this, &Z2Edit::SaveFile);
    RegisterCommand("wm", "Write mapper register.", this, &Z2Edit::WriteMapper);
    RegisterCommand("header", "Print iNES header.",
            &cartridge_, &Cartridge::PrintHeader);

    RegisterCommand("db", "Hexdump bytes (via mapper).", this, &Z2Edit::HexdumpBytes);
    RegisterCommand("dbp", "Hexdump PRG bytes.", this, &Z2Edit::HexdumpBytes);
    RegisterCommand("dbc", "Hexdump CHR bytes.", this, &Z2Edit::HexdumpBytes);
    RegisterCommand("dw", "Hexdump words (via mapper).", this, &Z2Edit::HexdumpWords);
    RegisterCommand("dwp", "Hexdump PRG words.", this, &Z2Edit::HexdumpWords);
    RegisterCommand("dwc", "Hexdump CHR words.", this, &Z2Edit::HexdumpWords);
    RegisterCommand("dtt", "Dump Town Text.", this, &Z2Edit::DumpTownText);
    RegisterCommand("wb", "Write bytes (via mapper).", this, &Z2Edit::WriteBytes);
    RegisterCommand("wbp", "Write PRG bytes.", this, &Z2Edit::WriteBytes);
    RegisterCommand("wbc", "Write CHR bytes.", this, &Z2Edit::WriteBytes);
    RegisterCommand("ww", "Write words (via mapper).", this, &Z2Edit::WriteWords);
    RegisterCommand("wwp", "Write PRG words.", this, &Z2Edit::WriteWords);
    RegisterCommand("wwc", "Write CHR words.", this, &Z2Edit::WriteWords);
    RegisterCommand("wt", "Write text bytes (via mapper).", this, &Z2Edit::WriteText);
    RegisterCommand("wtp", "Write PRG text bytes.", this, &Z2Edit::WriteText);
    RegisterCommand("wtc", "Write CHR text bytes.", this, &Z2Edit::WriteText);
    RegisterCommand("elist", "Dump Enemy List.", this, &Z2Edit::EnemyList);
    RegisterCommand("u", "Disassemble Code.", this, &Z2Edit::Unassemble);
    RegisterCommand("asm", "Assemble Code.", this, &Z2Edit::Assemble);
    RegisterCommand("insertprg", "Insert a PRG bank.", this, &Z2Edit::InsertPrg);
    RegisterCommand("copyprg", "Copy a PRG bank to another bank.", this, &Z2Edit::CopyPrg);
    RegisterCommand("insertchr", "Insert a CHR bank.", this, &Z2Edit::InsertChr);
    RegisterCommand("copychr", "Copy a CHR bank to another bank.", this, &Z2Edit::CopyChr);
    RegisterCommand("charclear", "Clear an individual char.", this, &Z2Edit::CharClear);
    RegisterCommand("charcopy", "Copy an individual char.", this, &Z2Edit::CharCopy);
    RegisterCommand("charswap", "Swap an individual char.", this, &Z2Edit::CharCopy);
    RegisterCommand("memmove", "Move memory within a PRG bank.", this, &Z2Edit::MemMove);
    RegisterCommand("swap", "Swap memory within a PRG bank.", this, &Z2Edit::Swap);
    RegisterCommand("bcopy", "Copy memory between PRG banks.", this, &Z2Edit::BCopy);
    RegisterCommand("set", "Set variables.", this, &Z2Edit::SetVar);
    RegisterCommand("source", "Read and execute debugconsole commands from file.", this, &Z2Edit::Source);
    RegisterCommand("restore", "Read/restore a PRG bank from a NES file.", this, &Z2Edit::RestoreBank);
    RegisterCommand("conntable", "Show the connection table for a given overworld/subworld", this, &Z2Edit::ConnTable);
    RegisterCommand("sendmessage", "Send a message to the editor refresh loop", this, &Z2Edit::SendMessage);

    loaded_ = false;
    ibase_ = 0;
    bank_ = 0;
    text_encoding_ = 0;
    hwpal_ = NesHardwarePalette::Get();
    chrview_.reset(new NesChrView);
    simplemap_.reset(new z2util::SimpleMap);
    palace_gfx_.reset(new z2util::PalaceGraphics);
    palette_editor_.reset(new z2util::PaletteEditor);
    rom_memory_.reset(new z2util::RomMemory);
    start_values_.reset(new z2util::StartValues);
    text_table_.reset(new z2util::TextTableEditor);
    tile_transform_.reset(new z2util::TileTransform);
    item_effects_.reset(new z2util::ItemEffects);
    object_table_.reset(new z2util::ObjectTable);
    overworld_tiles_.reset(new z2util::OverworldTiles);
    enemy_editor_.reset(new z2util::EnemyEditor);
    experience_table_.reset(new z2util::ExperienceTable);
    drops_.reset(new z2util::Drops);
    editor_.reset(z2util::Editor::New());
    project_.set_cartridge(&cartridge_);
    project_.set_visible(true);
    RestoreRememberWindowPositions();
    RestoreMultiMapConfig();
    if (remember_window_positions_) {
        RestoreUiState();
        RestoreWindowGeometry();
    }
    ini_path_ = os::path::Join({os::path::DataPath(), "imgui.ini"});
    ImGui::GetIO().IniFilename = remember_window_positions_
                                     ? ini_path_.c_str()
                                     : nullptr;
    for (const auto& w : UiWindowList()) {
        auto it = ui_visibility_.find(w.first);
        if (it != ui_visibility_.end()) {
            w.second->set_visible(it->second);
}
    }
}

std::vector<std::pair<const char*, ImWindowBase*>> Z2Edit::UiWindowList() {
    return {
        std::make_pair("console", &console_),
        std::make_pair("project", &project_),
        std::make_pair("simplemap", simplemap_.get()),
        std::make_pair("drops", drops_.get()),
        std::make_pair("enemyattr", enemy_editor_.get()),
        std::make_pair("experience", experience_table_.get()),
        std::make_pair("item_effects", item_effects_.get()),
        std::make_pair("overworld", editor_.get()),
        std::make_pair("palace_gfx", palace_gfx_.get()),
        std::make_pair("palette", palette_editor_.get()),
        std::make_pair("start_values", start_values_.get()),
        std::make_pair("text_table", text_table_.get()),
        std::make_pair("tile_transform", tile_transform_.get()),
        std::make_pair("hwpalette", hwpal_),
        std::make_pair("chrview", chrview_.get()),
        std::make_pair("object_table", object_table_.get()),
        std::make_pair("rom_memory", rom_memory_.get()),
    };
}

void Z2Edit::SaveUiState() {
    (void)File::MakeDirs(os::path::DataPath());
    std::string data;
    std::string path = os::path::Join({os::path::DataPath(), "ui_state.txt"});
    for (const auto& w : UiWindowList()) {
        absl::StrAppend(&data, "visible ", w.first, "=",
                        w.second->visible() ? "1" : "0", "\n");
    }
    if (loaded_) {
        absl::StrAppend(&data, "map primary=", simplemap_->map_name(), "\n");
        for (const auto& w : draw_callback_) {
            auto* sm = dynamic_cast<SimpleMap*>(w.get());
            if (sm && sm->visible() && !sm->map_name().empty()) {
                absl::StrAppend(&data, "map spawn=", sm->map_name(), "\n");
            }
            auto* mm = dynamic_cast<MultiMap*>(w.get());
            if (mm && mm->visible()) {
                absl::StrAppend(&data, "map multimap=", mm->world(), ":",
                                mm->overworld(), ":", mm->subworld(), ":",
                                mm->start(), "\n");
            }
        }
        // Persist the shared MultiMap settings template so newly-opened
        // windows start from it after a restart.  The textproto is flattened
        // onto a single line (newlines are insignificant).
        const auto& sc = ConfigLoader<SessionConfig>::GetConfig();
        auto it = sc.multimap().find(MultiMap::ConfigKey());
        if (it != sc.multimap().end() && it->second.initialized()) {
            std::string text;
            (void)google::protobuf::TextFormat::PrintToString(it->second, &text);
            absl::StrAppend(&data, "multimap config ", MultiMap::ConfigKey(), "=");
            for (char c : text)
                data += (c == '\n') ? ' ' : c;
            data += '\n';
        }
    }
    absl::StrAppend(&data, "remember_window_positions=",
                    remember_window_positions_ ? "1" : "0", "\n");
    if (remember_window_positions_) {
        int wx = 0, wy = 0, ww = 0, wh = 0;
        GetWindowGeometry(&wx, &wy, &ww, &wh);
        absl::StrAppend(&data, "window x=", wx, " y=", wy,
                        " width=", ww, " height=", wh, "\n");
    }
    File::SetContents(path, data);
}

void Z2Edit::RestoreRememberWindowPositions() {
    std::string data;
    std::string path = os::path::Join({os::path::DataPath(), "ui_state.txt"});
    if (!File::GetContents(path, &data)) {
        return;
    }
    for (const auto& line : absl::StrSplit(data, '\n', absl::SkipEmpty())) {
        const std::string l(absl::StripAsciiWhitespace(line));
        if (absl::StartsWith(l, "remember_window_positions=")) {
            remember_window_positions_ = l.substr(26) == "1";
            break;
        }
    }
}

void Z2Edit::RestoreMultiMapConfig() {
    std::string data;
    std::string path = os::path::Join({os::path::DataPath(), "ui_state.txt"});
    if (!File::GetContents(path, &data)) {
        return;
    }
    for (const auto& line : absl::StrSplit(data, '\n', absl::SkipEmpty())) {
        const std::string l(absl::StripAsciiWhitespace(line));
        if (absl::StartsWith(l, "multimap config ")) {
            std::string rest = l.substr(16);
            size_t eq = rest.find('=');
            if (eq != std::string::npos) {
                ui_multimap_config_text_ = rest.substr(eq + 1);
            }
        }
    }
}

void Z2Edit::RestoreUiState() {
    std::string data;
    std::string path = os::path::Join({os::path::DataPath(), "ui_state.txt"});
    if (!File::GetContents(path, &data)) {
        return;
    }
    for (const auto& line : absl::StrSplit(data, '\n', absl::SkipEmpty())) {
        const std::string l(absl::StripAsciiWhitespace(line));
        if (absl::StartsWith(l, "visible ")) {
            size_t eq = l.find('=');
            if (eq != std::string::npos) {
                ui_visibility_[l.substr(8, eq - 8)] = l.substr(eq + 1) == "1";
            }
        } else if (absl::StartsWith(l, "map primary=")) {
            ui_primary_map_ = l.substr(12);
        } else if (absl::StartsWith(l, "map spawn=")) {
            ui_spawned_maps_.push_back(l.substr(10));
        } else if (absl::StartsWith(l, "map multimap=")) {
            ui_multimap_maps_.push_back(l.substr(13));
        } else if (absl::StartsWith(l, "window ")) {
            int wx = 0, wy = 0, ww = 0, wh = 0;
            if (sscanf(l.c_str(), "window x=%d y=%d width=%d height=%d",
                       &wx, &wy, &ww, &wh) == 4) {
                ui_window_x_ = wx;
                ui_window_y_ = wy;
                ui_window_width_ = ww;
                ui_window_height_ = wh;
            }
        }
    }
}

// Restore the main program window to its saved position and size, unless it
// would end up entirely off-screen (e.g. it was on a monitor that is no
// longer connected), in which case the default placement is kept.
void Z2Edit::RestoreWindowGeometry() {
    int x = ui_window_x_, y = ui_window_y_;
    int w = ui_window_width_, h = ui_window_height_;
    if (w > 0 && h > 0 && WindowGeometryVisible(x, y, w, h)) {
        SetWindowGeometry(x, y, w, h);
    }
}

bool Z2Edit::WindowGeometryVisible(int x, int y, int w, int h) {
    int ndisplays = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&ndisplays);
    bool touched = false;
    for (int i = 0; i < ndisplays; ++i) {
        SDL_Rect r;
        if (!SDL_GetDisplayUsableBounds(displays[i], &r)) {
            continue;
        }
        // The window intersects the usable area of this display.
        if (x < r.x + r.w && x + w > r.x &&
            y < r.y + r.h && y + h > r.y) {
            touched = true;
            break;
        }
    }
    SDL_free(displays);
    return touched;
}

void Z2Edit::Load(const std::string& filename) {
    project_.Load(filename, false);
}

void Z2Edit::LoadPostProcess(int movekeepout) {
    mapper_.reset(MapperRegistry::New(&cartridge_, cartridge_.mapper()));
    if (movekeepout == -1) {
        movekeepout = FLAGS_move_from_keepout;
    }
    memory_.Reset();
    memory_.set_mapper(mapper_.get());
    memory_.CheckAllBanksForKeepout();
    if (movekeepout) {
        memory_.CheckAllBanksForKeepout(true);
    }
    loaded_ = true;

    chrview_->set_mapper(mapper_.get());
    simplemap_->set_mapper(mapper_.get());
    auto* ri = ConfigLoader<RomInfo>::MutableConfig();
    const auto& maps = ri->map();
    // Index of the first non-overworld map.
    int start = 0;
    while(start < maps.size() &&
          maps[start].type() == z2util::MapType::OVERWORLD) {
        start++;
        }
    // Select the map the user had open, falling back to North Palace.
    int selected = -1;
    for(int i=start; i<maps.size(); ++i) {
        const auto& m = maps[i];
        if (selected < 0 &&
            ((!ui_primary_map_.empty() && m.name() == ui_primary_map_) ||
             (m.name().find("North Palace") != std::string::npos))) {
            selected = i - start;
        }
    }
    if (selected >= 0) {
        simplemap_->SetMap(maps[start + selected], selected);
    }

    // Re-create the sideview and MultiMap windows that were open last session.
    // Any of these left over from a previous load reference the old Mapper,
    // which was just freed by mapper_.reset() above, so retire them before
    // spawning fresh ones against the new mapper.  They are marked for
    // disposal (not erased) because this can run while BaseDraw is iterating
    // draw_callback_.
    for (auto& w : draw_callback_) {
        auto* sm = dynamic_cast<SimpleMap*>(w.get());
        auto* mm = dynamic_cast<MultiMap*>(w.get());
        if (sm != nullptr || mm != nullptr) {
            w->set_visible(false);
            w->set_want_dispose(true);
        }
    }
    for (const auto& name : ui_spawned_maps_) {
        for (const auto& m : maps) {
            if (m.type() == z2util::MapType::OVERWORLD) continue;
            if (m.name() == name) {
                SimpleMap::Spawn(mapper_.get(), m);
            break;
        }
    }
    }
    // Apply the MultiMap settings template restored from the last session,
    // unless the loaded project already provides one.
    if (!ui_multimap_config_text_.empty()) {
        auto& tmpl = (*ConfigLoader<SessionConfig>::MutableConfig()
                          ->mutable_multimap())[MultiMap::ConfigKey()];
        if (!tmpl.initialized()) {
            (void)google::protobuf::TextFormat::ParseFromString(
                ui_multimap_config_text_, &tmpl);
        }
    }
    for (const auto& spec : ui_multimap_maps_) {
        int w = 0, o = 0, s = 0, st = 0;
        if (sscanf(spec.c_str(), "%d:%d:%d:%d", &w, &o, &s, &st) == 4) {
            MultiMap::Spawn(mapper_.get(), w, o, s, st);
        }
    }

    editor_->set_mapper(mapper_.get());
    overworld_tiles_->set_mapper(mapper_.get());
    overworld_tiles_->Refresh();
    editor_->Refresh();
    palace_gfx_->set_mapper(mapper_.get());
    palace_gfx_->Refresh();
    palette_editor_->set_mapper(mapper_.get());
    palette_editor_->Refresh();
    rom_memory_->set_mapper(mapper_.get());
    rom_memory_->Refresh();
    start_values_->set_mapper(mapper_.get());
    start_values_->Refresh();
    simplemap_->set_mapper(mapper_.get());
    simplemap_->Refresh();
    text_table_->set_mapper(mapper_.get());
    text_table_->Refresh();
    tile_transform_->set_mapper(mapper_.get());
    tile_transform_->Refresh();
    item_effects_->set_mapper(mapper_.get());
    item_effects_->Refresh();

    drops_->set_mapper(mapper_.get());
    drops_->Refresh();

    object_table_->set_mapper(mapper_.get());
    object_table_->Refresh();
    enemy_editor_->set_mapper(mapper_.get());
    enemy_editor_->Refresh();
    experience_table_->set_mapper(mapper_.get());
    experience_table_->Refresh();

    object_table_->Init();
    palette_editor_->Init();
    enemy_editor_->Init();
    experience_table_->Init();

    for(auto it=draw_callback_.begin(); it != draw_callback_.end(); ++it) {
        (*it)->Refresh();
    }
}

void Z2Edit::LoadFile(DebugConsole* console, int argc, char **argv) {
    bool move = FLAGS_move_from_keepout;
    if (argc < 2) {
        console->AddLog("[error] Usage: %s [filename] [move=<0,1>]", argv[0]);
        return;
    }
    if (argc == 3) {
        if (!strcmp(argv[2], "move=0")) move = false;
        if (!strcmp(argv[2], "move=1")) move = true;
    }
    const char *filename = argv[1];
    if (!strcmp(filename, "_")) {
        if (save_filename_.empty()) {
            console->AddLog("[error] No file to reload!");
            return;
        }
        filename = save_filename_.c_str();
    }
    if (absl::EndsWith(filename, "nes") || absl::EndsWith(filename, "NES")) {
        cartridge_.LoadFile(filename);
        LoadPostProcess(move);
    } else {
        project_.Load(filename, false);
    }
}

void Z2Edit::WriteMapper(DebugConsole* console, int argc, char **argv) {
    mapper_->DebugWriteReg(console, argc, argv);
}

void Z2Edit::SaveFile(DebugConsole* console, int argc, char **argv) {
    if (argc != 2) {
        console->AddLog("[error] Usage: %s [filename]", argv[0]);
        return;
    }
    if (absl::EndsWith(argv[1], "nes") || absl::EndsWith(argv[1], "NES")) {
        cartridge_.SaveFile(argv[1]);
    } else if (absl::EndsWith(argv[1], "ips") || absl::EndsWith(argv[1], "IPS")) {
        auto result = project_.ExportIps(argv[1]);
        if (!result.ok()) {
            console->AddLog("[error] %s", result.ToString().c_str());
        }
    } else {
        bool as_text = absl::EndsWith(argv[1], "textpb");
        project_.Save(argv[1], as_text);
    }
}

int Z2Edit::EncodedText(int ch) {
    if (text_encoding_ == 1) {
        ch = TextEncoding::FromZelda2(ch);
    } else {
        ch = TextEncoding::Identity(ch);
    }
    return ch ? ch : '.';
}

void Z2Edit::HexdumpBytes(DebugConsole* console, int argc, char **argv) {
    // The hexdump command will be one of 'db', 'dbp' or 'dbc', standing
    // for 'dump bytes', 'dump bytes prg' and 'dump bytes chr'.  The
    // prg and chr versions can optionally take a bank number.
    int bank = bank_;
    int mode = argv[0][2];
    int index = 0;
    if (argc < 2) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        if (mode) {
            console->AddLog("[error] %s [b=<bank>] <addr> <length>", argv[0]);
        } else {
            console->AddLog("[error] %s <addr> <length>", argv[0]);
        }
        return;
    }

    if (mode && !strncmp(argv[1], "b=", 2)) {
        bank = strtoul(argv[1]+2, 0, ibase_);
        index++;
    }
    uint32_t addr = strtoul(argv[index+1], 0, ibase_);
    int len = (argc == 3 + index) ? strtoul(argv[index+2], 0, ibase_) : 64;

    char line[128], chr[17];
    int i, n;
    uint8_t val;

    for(i=n=0; i < len; i++) {
        if (mode == 'p') {
            val = mapper_->ReadPrgBank(bank, addr+i);
        } else if (mode == 'c') {
            val = mapper_->ReadChrBank(bank, addr+i);
        } else {
            val = mapper_->Read(addr+i);
        }
        if (i % 16 == 0) {
            if (i) {
                n += sprintf(line+n, "  %s", chr);
                console->AddLog("%s", line);
            }
            n = sprintf(line, "%04x: ", addr+i);
            memset(chr, 0, sizeof(chr));
        }
        n += sprintf(line+n, " %02x", val);
        chr[i%16] = EncodedText(val);
    }
    if (i % 16) {
        i = 3*(16 - i%16);
    } else {
        i = 0;
    }
    n += sprintf(line+n, " %*c%s", i, ' ', chr);
    console->AddLog("%s", line);
}

void Z2Edit::WriteBytes(DebugConsole* console, int argc, char **argv) {
    int bank = bank_;
    int mode = argv[0][2];
    int index = 0;
    if (argc < 3) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        if (mode) {
            console->AddLog("[error] %s [b=<bank>] <addr> <val> ...", argv[0]);
        } else {
            console->AddLog("[error] %s <addr> <val> ...", argv[0]);
        }
        return;
    }

    if (mode && !strncmp(argv[1], "b=", 2)) {
        bank = strtoul(argv[1]+2, 0, ibase_);
        index++;
    }
    uint32_t addr = strtoul(argv[index+1], 0, ibase_);

    for(int i=2+index; i<argc; i++) {
        uint8_t val = strtoul(argv[i], 0, ibase_);
        if (mode == 'p') {
            mapper_->WritePrgBank(bank, addr++, val);
        } else if (mode == 'c') {
            mapper_->WriteChrBank(bank, addr++, val);
        } else {
            mapper_->Write(addr++, val);
        }
    }
}

void Z2Edit::WriteText(DebugConsole* console, int argc, char **argv) {
    int bank = bank_;
    int mode = argv[0][2];
    int index = 0;
    if (argc < 3) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        if (mode) {
            console->AddLog("[error] %s [b=<bank>] <addr> <val> ...", argv[0]);
        } else {
            console->AddLog("[error] %s <addr> <val> ...", argv[0]);
        }
        return;
    }

    if (mode && !strncmp(argv[1], "b=", 2)) {
        bank = strtoul(argv[1]+2, 0, ibase_);
        index++;
    }
    uint32_t addr = strtoul(argv[index+1], 0, ibase_);

    for(int i=2+index; i<argc; i++) {
        for(char *val = argv[i]; *val; val++) {
            int ch = *val;
            if (text_encoding_ == 1) {
                ch = TextEncoding::ToZelda2(ch);
                if (ch == 0) ch = 0xf4;
            }
            if (mode == 'p') {
                mapper_->WritePrgBank(bank, addr++, ch);
            } else if (mode == 'c') {
                mapper_->WriteChrBank(bank, addr++, ch);
            } else {
                mapper_->Write(addr++, ch);
            }
        }
    }
}

void Z2Edit::HexdumpWords(DebugConsole* console, int argc, char **argv) {
    int bank = bank_;
    int mode = argv[0][2];
    int index = 0;
    if (argc < 2) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        if (mode) {
            console->AddLog("[error] %s [b=<bank>] <addr> <length>", argv[0]);
        } else {
            console->AddLog("[error] %s <addr> <length>", argv[0]);
        }
        return;
    }

    if (mode && !strncmp(argv[1], "b=", 2)) {
        bank = strtoul(argv[1]+2, 0, ibase_);
        index++;
    }
    uint32_t addr = strtoul(argv[index+1], 0, ibase_);
    int len = (argc == 3 + index) ? strtoul(argv[index+2], 0, ibase_) : 64;

    char line[128], chr[17];
    int i, n;
    uint16_t val;

    for(i=n=0; i < len; i+=2) {
        if (i % 16 == 0) {
            if (i) {
                n += sprintf(line+n, "  %s", chr);
                console->AddLog("%s", line);
            }
            n = sprintf(line, "%04x: ", addr+i);
            memset(chr, 0, sizeof(chr));
        }
        if (mode == 'p') {
            val = uint16_t(mapper_->ReadPrgBank(bank, addr+i+1)) << 8 |
                  uint16_t(mapper_->ReadPrgBank(bank, addr+i));
        } else if (mode == 'c') {
            val = uint16_t(mapper_->ReadChrBank(bank, addr+i+1)) << 8 |
                  uint16_t(mapper_->ReadChrBank(bank, addr+i));
        } else {
            val = uint16_t(mapper_->Read(addr+i+1)) << 8 |
                  uint16_t(mapper_->Read(addr+i));
        }
        n += sprintf(line+n, " %04x", val);
        chr[i%16] = EncodedText(uint8_t(val));
        val >>= 8;
        chr[i%16] = EncodedText(uint8_t(val));
    }
    if (i % 16) {
        i = 3*(16 - i%16);
    } else {
        i = 0;
    }
    n += sprintf(line+n, " %*c%s", i, ' ', chr);
    console->AddLog("%s", line);
}

void Z2Edit::WriteWords(DebugConsole* console, int argc, char **argv) {
    int bank = bank_;
    int mode = argv[0][2];
    int index = 0;
    if (argc < 3) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        if (mode) {
            console->AddLog("[error] %s [b=<bank>] <addr> <val> ...", argv[0]);
        } else {
            console->AddLog("[error] %s <addr> <val> ...", argv[0]);
        }
        return;
    }

    if (mode && !strncmp(argv[1], "b=", 2)) {
        bank = strtoul(argv[1]+2, 0, ibase_);
        index++;
    }
    uint32_t addr = strtoul(argv[index+1], 0, ibase_);

    for(int i=2+index; i<argc; i++) {
        uint16_t val = strtoul(argv[i], 0, ibase_);
        if (mode == 'p') {
            mapper_->WritePrgBank(bank, addr++, val);
            mapper_->WritePrgBank(bank, addr++, val>>8);
        } else if (mode == 'c') {
            mapper_->WriteChrBank(bank, addr++, val);
            mapper_->WriteChrBank(bank, addr++, val>>8);
        } else {
            mapper_->Write(addr++, val);
            mapper_->Write(addr++, val>>8);
        }
    }
}

void Z2Edit::MemMove(DebugConsole* console, int argc, char **argv) {
    int bank = -1;
    if (argc < 5 || strncmp(argv[1], "b=", 2) != 0) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        console->AddLog("[error] %s b=<bank> <dst> <src> <len>",  argv[0]);
        return;
    }

    bank = strtoul(argv[1]+2, 0, ibase_);
    int32_t dst = strtoul(argv[2], 0, ibase_);
    int32_t src = strtoul(argv[3], 0, ibase_);
    int32_t len = strtoul(argv[4], 0, ibase_);

    if (dst < src) {
        for(int i=0; i<len; i++, dst++, src++) {
            mapper_->WritePrgBank(bank, dst, mapper_->ReadPrgBank(bank, src));
        }
    } else if (dst > src) {
        dst += len-1; src += len-1;
        for(int i=0; i<len; i++, dst--, src--) {
            mapper_->WritePrgBank(bank, dst, mapper_->ReadPrgBank(bank, src));
        }
    } else {
        console->AddLog("[error] dst and src are the same!");
    }
}

void Z2Edit::Swap(DebugConsole* console, int argc, char **argv) {
    int bank = -1;
    if (argc < 5 || strncmp(argv[1], "b=", 2) != 0) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        console->AddLog("[error] %s b=<bank> <dst> <src> <len>",  argv[0]);
        return;
    }

    bank = strtoul(argv[1]+2, 0, ibase_);
    int32_t dst = strtoul(argv[2], 0, ibase_);
    int32_t src = strtoul(argv[3], 0, ibase_);
    int32_t len = strtoul(argv[4], 0, ibase_);

    for(int i=0; i<len; i++, dst++, src++) {
        uint8_t a = mapper_->ReadPrgBank(bank, src);
        uint8_t b = mapper_->ReadPrgBank(bank, dst);
        mapper_->WritePrgBank(bank, dst, a);
        mapper_->WritePrgBank(bank, src, b);
    }
}

void Z2Edit::BCopy(DebugConsole* console, int argc, char **argv) {
    if (argc < 4) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        console->AddLog("[error] %s  <bank:dst> <bank:src> <len>",  argv[0]);
        return;
    }

    int32_t srcb, srca, dstb, dsta, len;
    char *endp;

    dstb = strtoul(argv[1], &endp, ibase_);
    if (*endp != ':') {
        console->AddLog("[error] bad dst argument.  Expected bank:address.");
        return;
    }
    dsta = strtoul(endp+1, 0, ibase_);

    srcb = strtoul(argv[2], &endp, ibase_);
    if (*endp != ':') {
        console->AddLog("[error] bad src argument.  Expected bank:address.");
        return;
    }
    srca = strtoul(endp+1, 0, ibase_);

    len = strtoul(argv[3], 0, ibase_);

    for(int i=0; i<len; i++, dsta++, srca++) {
        mapper_->WritePrgBank(dstb, dsta, mapper_->ReadPrgBank(srcb, srca));
    }
}

void Z2Edit::EnemyList(DebugConsole* console, int argc, char **argv) {
    char buf[1024];
    int bank = bank_;
    int mode = argv[0][2];
    int index = 0;
    if (argc < 3) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        if (mode) {
            console->AddLog("[error] %s [b=<bank>] <addr> <val> ...", argv[0]);
        } else {
            console->AddLog("[error] %s <addr> <val> ...", argv[0]);
        }
        return;
    }

    if (mode && !strncmp(argv[1], "b=", 2)) {
        bank = strtoul(argv[1]+2, 0, ibase_);
        index++;
    }
    uint32_t addr = strtoul(argv[index+1], 0, ibase_);
    int len = (argc == 3 + index) ? strtoul(argv[index+2], 0, ibase_) : 64;

    while(len > 0) {
        int listlen = mapper_->ReadPrgBank(bank, addr);
        buf[0] = buf[1] = 0;
        console->AddLog("%04x: %02x (copied to %04x)", addr, listlen, addr-0x18a0);

        addr++; len--;
        int j=0;
        for(int i=1; i<listlen && len; i++, len--) {
            j += sprintf(buf+j, " %02x", mapper_->ReadPrgBank(bank, addr++));
        }
        console->AddLog("    [%s]", buf+1);
    }
}

void Z2Edit::Unassemble(DebugConsole* console, int argc, char **argv) {
    int bank = bank_;
    static uint16_t addr;

    int index = 0;
    if (argc < 2) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        console->AddLog("[error] %s [b=<bank>] <addr> <length>", argv[0]);
        return;
    }

    if (!strncmp(argv[1], "b=", 2)) {
        bank = strtoul(argv[1]+2, 0, ibase_);
        index++;
    }
    addr = strtoul(argv[index+1], 0, ibase_);
    int len = (argc == 3 + index) ? strtoul(argv[index+2], 0, ibase_) : 10;

    Cpu cpu(mapper_.get());
    cpu.set_bank(bank);
    for(int i=0; i<len; i++) {
        std::string instruction = cpu.Disassemble(&addr);
        console->AddLog("%s", instruction.c_str());
    }
}

void Z2Edit::Assemble(DebugConsole* console, int argc, char **argv) {
    int bank = bank_;
    uint16_t addr;

    int index = 0;
    if (argc < 2) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        console->AddLog("[error] %s [b=<bank>] <addr>", argv[0]);
        return;
    }

    if (!strncmp(argv[1], "b=", 2)) {
        bank = strtoul(argv[1]+2, 0, ibase_);
        index++;
    }
    addr = (index+1 < argc) ? strtoul(argv[index+1], 0, ibase_) : 0;

    struct State {
        uint16_t addr;
        Cpu cpu;
    };

    State* state = new State{addr, mapper_.get()};
    state->cpu.set_bank(bank);
    console->AddLog("#{88f}Entering assembler mode (bank=%d).  '.end' to leave.", bank);
    console->PushLineCallback([state](DebugConsole* console, const char *cmdline) {
        const char *errors[] = {
            "None", "End", "Meta", "Invalid Opcode", "Invalid Operand",
            "Invalid Addressing Mode",
        };
        std::string line(cmdline);
        if (line == "quit") {
            console->AddLog("#{88f}No 'quit' in assembler mode. Did you mean '.end'?");
            return;
        }
        if (line == "help" || line == ".help") {
            for(const auto& h : Cpu::asmhelp()) {
                console->AddLog("%s", h.c_str());
            }
            return;
        }
        uint16_t prev = state->addr;
        auto err = state->cpu.Assemble(line, &state->addr);
        if (err == Cpu::AsmError::None) {
            const char *comment = strchr(cmdline, ';');
            if (prev != state->addr) {
                std::string instruction = state->cpu.Disassemble(&prev);
                console->AddLog("#{8f8}%-40s %s",
                    instruction.c_str(), comment ? comment : "");
            } else if (comment) {
                console->AddLog("#{8f8}%s", comment);
            }
        } else if (err == Cpu::AsmError::Meta) {
            console->AddLog("#{88f}%04x: %s", prev, cmdline);
            if (prev != state->addr) {
                console->AddLog("#{88f}%04x:", state->addr);
            }
        } else if (err == Cpu::AsmError::End) {
            const auto& errors = state->cpu.ApplyFixups();
            for(const auto& e : errors) {
                console->AddLog("#{f88}%s", e.c_str());
            }
            console->AddLog("#{88f}Leaving assembler mode");
            delete state;
            console->PopLineCallback();
            return;
        } else {
            console->AddLog("#{f88}%s: '%s'", errors[int(err)], cmdline);
        }
    });
}



void Z2Edit::InsertPrg(DebugConsole* console, int argc, char **argv) {
    int bank = bank_;
    if (argc < 2) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        console->AddLog("[error] %s <bank>", argv[0]);
        return;
    }

    bank = strtoul(argv[1], 0, ibase_);
    cartridge_.InsertPrg(bank, nullptr);
    console->AddLog("#{0f0}Added PRG bank %d", bank);
}

void Z2Edit::CopyPrg(DebugConsole* console, int argc, char **argv) {
    uint8_t src, dst;
    if (argc < 3) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        console->AddLog("[error] %s <src> <dst>", argv[0]);
        return;
    }

    src = strtoul(argv[1], 0, ibase_);
    dst = strtoul(argv[2], 0, ibase_);
    for(int i=0; i<16384; i++) {
        mapper_->WritePrgBank(dst, i, mapper_->ReadPrgBank(src, i));
    }
    console->AddLog("#{0f0}Copied PRG bank %d to %d", src, dst);
}

void Z2Edit::InsertChr(DebugConsole* console, int argc, char **argv) {
    int bank = bank_;
    if (argc < 2) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        console->AddLog("[error] %s <bank>", argv[0]);
        return;
    }

    bank = strtoul(argv[1], 0, ibase_);
    cartridge_.InsertChr(bank, nullptr);
    console->AddLog("#{0f0}Added CHR bank %d", bank);
}

void Z2Edit::CopyChr(DebugConsole* console, int argc, char **argv) {
    uint8_t src, dst;
    if (argc < 3) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        console->AddLog("[error] %s <src> <dst>", argv[0]);
        return;
    }

    src = strtoul(argv[1], 0, ibase_);
    dst = strtoul(argv[2], 0, ibase_);
    for(int i=0; i<4096; i++) {
        mapper_->WriteChrBank(dst, i, mapper_->ReadChrBank(src, i));
    }
    console->AddLog("#{0f0}Copied CHR bank %d to %d", src, dst);
}

bool Z2Edit::ParseChr(const std::string& a, int* bank, uint8_t* addr) {
    *bank = chrbank_;
    const char* s = a.c_str();
    const char* t = strchr(s, ':');
    if (t) {
        *bank = strtoul(s, 0, ibase_);
        s = t+1;
    }
    *addr = strtoul(s, 0, ibase_);
    return true;
}

void Z2Edit::CharClear(DebugConsole* console, int argc, char **argv) {
    int bank = chrbank_;
    uint8_t chr;
    bool with_id = false;
    if (argc < 2) {
        console->AddLog("[error] Usage: %s [char{,char,char...}] [with_id]", argv[0]);
        return;
    }
    if (!strcmp(argv[2], "true")) {
        with_id = true;
    }
    ChrUtil util(mapper_.get());
    for(const auto& s : absl::StrSplit(argv[1], ',')) {
        ParseChr(std::string(s), &bank, &chr);
        util.Clear(bank, chr, with_id);
    }
}

void Z2Edit::CharCopy(DebugConsole* console, int argc, char **argv) {
    int sbank, dbank;
    uint8_t src, dst;
    if (argc < 3) {
        console->AddLog("[error] Usage: %s [dstchar] [srcchar]", argv[0]);
        return;
    }
    ParseChr(argv[1], &dbank, &dst);
    ParseChr(argv[2], &sbank, &src);
    ChrUtil util(mapper_.get());
    if (!strcmp(argv[0], "charcopy")) {
        util.Copy(dbank, dst, sbank, src);
    } else if (!strcmp(argv[0], "charswap")) {
        util.Swap(dbank, dst, sbank, src);
    } else {
        console->AddLog("[error] Usage: %s [dstchar] [srcchar]", argv[0]);
    }
}

void Z2Edit::SetVar(DebugConsole* console, int argc, char **argv) {
    if (argc < 3) {
        console->AddLog("[error] Usage: %s [var] [number]", argv[0]);
        console->AddLog("Current 'ibase': %d "
                        "(zero means autodetect with C prefixes)", ibase_);
        console->AddLog("Current 'bank': %d", bank_);
        console->AddLog("Current 'text' encoding: %d", text_encoding_);
        console->AddLog("Current 'emulator' command: %s", FLAGS_emulator.c_str());
        return;
    }
    for(int i=1; i<argc; i++) {
        if (!strcmp(argv[i], "ibase")) {
            ibase_ = strtoul(argv[++i], 0, 0);
        } else if (!strcmp(argv[i], "bank")) {
            bank_ = strtoul(argv[++i], 0, ibase_);
        } else if (!strcmp(argv[i], "chrbank")) {
            chrbank_ = strtoul(argv[++i], 0, ibase_);
        } else if (!strcmp(argv[i], "text")) {
            text_encoding_ = strtoul(argv[++i], 0, ibase_);
        } else if (!strcmp(argv[i], "emulator")) {
            FLAGS_emulator = argv[++i];
        } else if (!strcmp(argv[i], "mapper")) {
            uint8_t m = strtoul(argv[++i], 0, 0);
            cartridge_.set_mapper(m);
        } else {
            console->AddLog("[error] Unknown var '%s'", argv[1]);
        }
    }
}

void Z2Edit::Source(DebugConsole* console, int argc, char **argv) {
    if (argc != 2) {
        console->AddLog("[error] Usage: %s [file]", argv[0]);
        return;
    }
    Source(argv[1], console);
}

void Z2Edit::Source(const std::string& filename, DebugConsole* console) {
    if (console == nullptr) console = &console_;
    char buf[4096];
    char *p;
    FILE *fp = fopen(filename.c_str(), "r");;
    if (!fp) {
        console->AddLog("[error] Couldn't read %s", filename.c_str());
        return;
    }
    while((p = fgets(buf, sizeof(buf), fp)) != nullptr) {
        char *end = p + strlen(p);
        while(end > p && isspace(end[-1])) {
            *--end = '\0';
        }
        console->ExecCommand(p);
    }
    fclose(fp);
}

void Z2Edit::RestoreBank(DebugConsole* console, int argc, char **argv) {
    bool move = FLAGS_move_from_keepout;
    if (argc < 4) {
        console->AddLog("[error] %s: Wrong number of arguments.", argv[0]);
        console->AddLog("[error] %s <nesfile> <frombank> <tobank> [move=<0,1>]",  argv[0]);
        return;
    }

    const char *nesfile = argv[1];
    uint32_t from = strtoul(argv[2], 0, ibase_);
    uint32_t to = strtoul(argv[3], 0, ibase_);
    if (argc == 5) {
        if (!strcmp(argv[4], "move=0")) move = false;
        if (!strcmp(argv[4], "move=1")) move = true;
    }

    Cartridge kart;
    if (!kart.IsNESFile(nesfile)) {
        console->AddLog("[error] %s is not a NES file", nesfile);
        return;
    }
    kart.LoadFile(nesfile);
    if (from >= kart.prglen()) {
        console->AddLog("[error] %s only has %u banks", nesfile, kart.prglen());
        return;
    }
    if (to >= cartridge_.prglen()) {
        console->AddLog("[error] Current image only has %u banks",
                        cartridge_.prglen());
        return;
    }
    for(int i=0; i<16384; i++) {
        uint8_t data = kart.ReadPrg(from*16384 + i);
        mapper_->WritePrgBank(to, i, data);
    }
    LoadPostProcess(move);
}

void Z2Edit::DumpTownText(DebugConsole* console, int argc, char **argv) {
    if (argc != 3) {
        console->AddLog("[error] Usage: %s [towncode] [enemyid]", argv[0]);
        return;
    }
    uint8_t towncode = strtoul(argv[1], 0, ibase_);
    uint8_t enemyid = strtoul(argv[2], 0, ibase_);

    if (enemyid < 10) {
        console->AddLog("[error] Enemy IDs less than 10 do not have text");
        return;
    }
    const auto& text_table = ConfigLoader<RomInfo>::GetConfig().text_table();
    enemyid -= 10;

    char text[256];
    uint8_t world = towncode >> 2;
    uint8_t index = enemyid * 4 + (towncode & 3);
    Address ptable = mapper_->ReadAddr(text_table.pointer(), world * 2);
    int len = (index < 64) ? 2 : 1;

    for(int i=0; i<len; i++) {
        const auto& region = text_table.index(world * 2 + i);
        if (index >= region.length())
            continue;

        int offset = mapper_->Read(region, index);
        Address str = mapper_->ReadAddr(ptable, offset * 2);

        for(int j=0, ch=0; j<254; j++) {
            ch = mapper_->Read(str, j);
            if (ch == 255) {
                text[j] = 0;
                break;
            }
            ch = TextEncoding::FromZelda2(ch);
            if (ch == 0) ch = '_';
            text[j] = ch;
        }
        console->AddLog("%d (@%04x): %s", i, str.address(), text);
    }
}

void Z2Edit::ConnTable(DebugConsole* console, int argc, char **argv) {
    if (argc != 3) {
        console->AddLog("[error] Usage: %s [overworld] [subworld]", argv[0]);
        return;
    }
    int overworld = strtol(argv[1], 0, ibase_);
    int subworld = strtol(argv[2], 0, ibase_);
    OverworldConnectorList clist;
    if (!clist.Init(mapper_.get(), overworld, subworld)) {
        console->AddLog("[error] Overworld %d-%d not known.", overworld, subworld);
        return;
    }
    for(const auto& s : clist.Print()) {
        console->AddLog("%s", s.c_str());
    }
}

void Z2Edit::SendMessage(DebugConsole* console, int argc, char **argv) {
    if (argc != 3) {
        console->AddLog("[error] Usage: %s [message] [argument]", argv[0]);
        return;
    }
    const char* message = argv[1];
    int argument = strtol(argv[2], 0, ibase_);
    ProcessMessage(message, reinterpret_cast<void*>(static_cast<intptr_t>(argument)));
}

void Z2Edit::SpawnEmulator() {
    std::string romtmp = os::TempFilename(FLAGS_romtmp);
    cartridge_.SaveFile(romtmp);
    os::System(absl::StrCat(FLAGS_emulator, " ", romtmp), true);
}

void Z2Edit::SpawnEmulator(
        uint8_t bank,
        uint8_t region,
        uint8_t world,
        uint8_t town_code,
        uint8_t palace_code,
        uint8_t location_id,
        uint8_t room,
        uint8_t page,
        uint8_t prev_region) {

    uint8_t facing = (page < 3) ? 0 : 1;
    LOGF(INFO, "StartEmulator:");
    LOGF(INFO, "  bank: %d", bank);
    LOGF(INFO, "  region: %d", region);
    LOGF(INFO, "  world: %d", world);
    LOGF(INFO, "  town_code: %d", town_code);
    LOGF(INFO, "  palace_code: %d", palace_code);
    LOGF(INFO, "  location_id: %d", location_id);
    LOGF(INFO, "  room: %d", room);
    LOGF(INFO, "  page: %d", page);

    uint8_t inject[] = {
        //0xa9, bank,             // LDA #bank
        //0x8d, 0x69, 0x07,       // STA $0769  ; CurrentPrgBank - value doesn't need to be pre-set for Z2R
        0xa9, region,           // LDA #region
        0x8d, 0x06, 0x07,       // STA $0706  ; RegionNumber   - Z2edit doesn't adapt to palaces shuffled across continents so this breaks
        0xa9, world,            // LDA #world
        0x8d, 0x07, 0x07,       // STA $0707  ; WorldNumber
        0xa9, town_code,        // LDA #town_code
        0x8d, 0x6b, 0x05,       // STA $056b  ; TownNumber
        0xa9, palace_code,      // LDA #palace_code
        0x8d, 0x6c, 0x05,       // STA $056c  ; PalaceRegionIndex
        0xa9, location_id,      // LDA #location_id
        0x8d, 0x48, 0x07,       // STA $0748  ; LocationNumber  - Z2edit just puts 1 here - affects where you end up when you leave
        0xa9, room,             // LDA #room
        0x8d, 0x61, 0x05,       // STA $0561  ; MapNumber
        0x8d, 0x20, 0x01,       // STA $0120  ; [Z2R] temp_room_code
        0xa9, page,             // LDA #page
        0x8d, 0x5c, 0x07,       // STA $075c  ; EntryPage
        0xa9, facing,           // LDA #facing
        0x8d, 0x01, 0x07,       // STA $0701  ; EntryFacing
        //0xa9, prev_region,      // LDA #prev_region
        //0x8d, 0x0a, 0x07,       // STA $070a  ; value unused in Z2R
        0x60,                   // RTS
    };
    uint16_t addr = 0xaa3f & 0x3FFF;

    std::string romtmp = os::TempFilename(FLAGS_romtmp);
    Cartridge temp(cartridge_);
    for(size_t i=0; i < sizeof(inject); i++) {
        temp.WritePrg(addr + i, inject[i]);
    }
    temp.SaveFile(romtmp);
    os::System(absl::StrCat(FLAGS_emulator, " ", romtmp), true);
}

void Z2Edit::ProcessEvent(SDL_Event* event) {
    editor_->ProcessEvent(event);
}

void Z2Edit::ProcessMessage(const std::string& msg, const void* extra) {
    if (msg == "commit") {
        project_.Commit(static_cast<const char*>(extra));
        // Refresh this here for convenience: the table is very small, but
        // commites of the overworld can re-write it.
        tile_transform_->Refresh();
    } else if (msg == "loadpostprocess") {
        LoadPostProcess(reinterpret_cast<intptr_t>(extra));
    } else if (msg == "repack") {
        simplemap_->Refresh();
        editor_->Refresh();
    } else if (msg == "emulate_at") {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(extra);
        SpawnEmulator(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8]);
    } else {
        console_.AddLog("[error] Unknown message %s(%p)", msg.c_str(), extra);
    }
}

// Open a widget (and raise it if it is already visible behind other widgets).
static void ShowWidget(ImWindowBase* w) {
    w->set_visible(true);
    w->set_focus_next(true);
}

// Draw a widget, first honoring any pending bring-to-front request.
static void DrawWidget(ImWindowBase* w) {
    if (w->consume_focus_next())
        ImGui::SetNextWindowFocus();
    w->Draw();
}

void Z2Edit::Draw() {
    SetTitle(project_.name());
    ImGui::SetNextWindowSize(ImVec2(500,300), ImGuiCond_FirstUseEver);
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Emulate", "Ctrl+E")) {
                SpawnEmulator();
            }
#ifdef HAVE_NFD
            if (ImGui::MenuItem("Load & Run a Script")) {
                char *filename = nullptr;
                char cmd[] = "source";
                auto result = NFD_OpenDialog(nullptr, nullptr, &filename);
                if (result == NFD_OKAY) {
                    char *argv[] = {cmd, filename};
                    Source(&console_, 2, argv);
                    console_.visible() = true;
                }
                free(filename);
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                char *filename = nullptr;
                auto result = NFD_OpenDialog("z2prj;nes", nullptr, &filename);
                if (result == NFD_OKAY) {
                    project_.Load(filename);
                    save_filename_.assign(filename);
                }
                free(filename);
            }

            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                if (save_filename_.empty())
                    goto save_as;
                project_.Save(save_filename_);
            }
            if (ImGui::MenuItem("Save As")) {
save_as:
                char *filename = nullptr;
                auto result = NFD_SaveDialog("z2prj", nullptr, &filename);
                if (result == NFD_OKAY) {
                    std::string savefile = filename;
                    if (absl::EndsWith(savefile, ".z2prj")) {
                        save_filename_.assign(savefile);
                        project_.Save(save_filename_);
                    } else {
                        ErrorDialog::Spawn("Bad File Extension",
                            ErrorDialog::OK | ErrorDialog::CANCEL,
                            "Project files should have the extension .z2prj\n"
                            "If you want to save a .nes file, use File | Export\n\n"
                            "Press 'OK' to save anyway.\n")->set_result_cb(
                                [=](int result) {
                                    if (result == ErrorDialog::OK) {
                                        save_filename_.assign(savefile);
                                        project_.Save(save_filename_);
                                    }
                                });
                    }
                }
                free(filename);
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Import ROM")) {
                char *filename = nullptr;
                auto result = NFD_OpenDialog("nes", nullptr, &filename);
                if (result == NFD_OKAY) {
                    project_.ImportRom(filename);
                }
                free(filename);
            }
            if (ImGui::MenuItem("Export ROM")) {
                if (export_filename_.empty())
                    goto export_as;
                project_.ExportRom(export_filename_);
            }
            if (ImGui::MenuItem("Export ROM As")) {
export_as:
                char *filename = nullptr;
                auto result = NFD_SaveDialog("nes", nullptr, &filename);
                if (result == NFD_OKAY) {
                    export_filename_.assign(filename);
                    project_.ExportRom(export_filename_);
                }
                free(filename);
            }
            if (ImGui::MenuItem("Export IPS Patch")) {
                char *filename = nullptr;
                auto result = NFD_SaveDialog("ips", nullptr, &filename);
                if (result == NFD_OKAY) {
                    (void)project_.ExportIps(filename);
                }
                free(filename);
            }
#endif
            ImGui::Separator();
            if (!FLAGS_config.empty()) {
                if (ImGui::MenuItem("Reload Config")) {
                    auto* config = ConfigLoader<z2util::RomInfo>::Get();
                    config->Reload();
                }
            }
            if (ImGui::MenuItem("Quit")) {
                running_ = false;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Debug Console", nullptr, console_.visible()))
                ShowWidget(&console_);
            if (ImGui::MenuItem("Drops", nullptr, drops_->visible()))
                ShowWidget(drops_.get());
            if (ImGui::MenuItem("Enemy Attributes", nullptr,
                                enemy_editor_->visible()))
                ShowWidget(enemy_editor_.get());
            if (ImGui::MenuItem("Experience Table", nullptr,
                                experience_table_->visible()))
                ShowWidget(experience_table_.get());
            if (ImGui::MenuItem("Item Effects", nullptr,
                                item_effects_->visible()))
                ShowWidget(item_effects_.get());
            if (ImGui::MenuItem("Overworld Editor", nullptr, editor_->visible()))
                ShowWidget(editor_.get());
            if (ImGui::MenuItem("Sideview Editor", nullptr,
                                simplemap_->visible()))
                ShowWidget(simplemap_.get());
            if (ImGui::MenuItem("Palace Graphics", nullptr,
                                palace_gfx_->visible()))
                ShowWidget(palace_gfx_.get());
            if (ImGui::MenuItem("Palette Editor", nullptr,
                                palette_editor_->visible()))
                ShowWidget(palette_editor_.get());
            if (ImGui::MenuItem("Start Values", nullptr,
                                start_values_->visible()))
                ShowWidget(start_values_.get());
            if (ImGui::MenuItem("Text Table", nullptr, text_table_->visible()))
                ShowWidget(text_table_.get());
            if (ImGui::MenuItem("Tile Transforms", nullptr,
                                tile_transform_->visible()))
                ShowWidget(tile_transform_.get());
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("History", nullptr, project_.visible()))
                ShowWidget(&project_);
            if (ImGui::MenuItem("Hardware Palette", nullptr, hwpal_->visible()))
                ShowWidget(hwpal_);
            if (ImGui::MenuItem("CHR Viewer", nullptr, chrview_->visible()))
                ShowWidget(chrview_.get());
            if (ImGui::MenuItem("Object Table", nullptr,
                                object_table_->visible()))
                ShowWidget(object_table_.get());
            if (ImGui::MenuItem("Rom Memory", nullptr, rom_memory_->visible()))
                ShowWidget(rom_memory_.get());
            ImGui::Separator();
            ImGui::Checkbox("Remember Window Positions",
                            &remember_window_positions_);
            ImGui::GetIO().IniFilename = remember_window_positions_
                                             ? ini_path_.c_str()
                                             : nullptr;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Online Help")) {
                Help("root");
            }
            if (ImGui::MenuItem("About")) {
                ErrorDialog::Spawn("About Z2Edit",
                    "Z2Edit Zelda II ROM Editor\n\n",
#ifdef BUILD_GIT_VERSION
                    "Version: ", BUILD_GIT_VERSION
#else
                    "Version: Unknown"
#warning "Built without version stamp"
#endif
                    );

            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (loaded_) {
        DrawWidget(start_values_.get());
        DrawWidget(text_table_.get());
        DrawWidget(tile_transform_.get());
        DrawWidget(item_effects_.get());
        DrawWidget(palace_gfx_.get());
        DrawWidget(palette_editor_.get());
        DrawWidget(rom_memory_.get());
        DrawWidget(hwpal_);
        DrawWidget(chrview_.get());
        DrawWidget(drops_.get());
        DrawWidget(simplemap_.get());
        DrawWidget(editor_.get());
        DrawWidget(object_table_.get());
        DrawWidget(enemy_editor_.get());
        DrawWidget(experience_table_.get());
        DrawWidget(&project_);
    }
}

void Z2Edit::AfterDraw() {
    if (!loaded_) {
        // Let the first frame reach the screen before the native picker blocks.
        char *filename = nullptr;
        auto result = NFD_OpenDialog("z2prj,nes", nullptr, &filename);
        if (result == NFD_OKAY) {
            project_.Load(filename, false);
            if (absl::EndsWith(filename, ".z2prj")) {
                save_filename_.assign(filename);
            }
        }
        free(filename);
    }
}

void Z2Edit::Help(const std::string& topickey) {
    const auto& ri = ConfigLoader<RomInfo>::GetConfig();
    const auto& it = ri.help().url().find(topickey);
    if (it == ri.help().url().end()) {
        ZLOG(ERROR, "No help for topickey=", topickey);
        if (topickey != "root") {
            Help("root");
        }
        return;
    }
    Browser::Open(it->second);
}

}  // namespace z2util
