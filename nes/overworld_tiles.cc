#include "nes/overworld_tiles.h"

#include "nes/mapper.h"
#include "proto/rominfo.pb.h"
#include "util/config.h"
#include "util/logging.h"

namespace z2util {

void OverworldTiles::Refresh() {
    int index = EnabledIndex();
    if (index < 0) {
        LOGF(INFO, "No known overworld tile layout detected in ROM.");
        return;
    }
    LOGF(INFO, "Overworld tile layout %d detected in ROM.", index);

    auto* ri = ConfigLoader<RomInfo>::MutableConfig();
    auto* p = ri->mutable_palettes(0);
    if (index == 1) {
        p->mutable_palette(0)->set_hidden(true);
        p->mutable_palette(1)->set_hidden(true);
        p->mutable_palette(2)->set_hidden(false);
        p->mutable_palette(3)->set_hidden(false);
        p->mutable_palette(4)->set_hidden(false);
        p->mutable_palette(5)->set_hidden(false);
        p->mutable_palette(6)->set_hidden(false);
        p->mutable_palette(7)->set_hidden(false);
        ri->mutable_map(0)->mutable_objtable(0)->set_address(0xbb80);
        ri->mutable_map(1)->mutable_objtable(0)->set_address(0xbc00);
        ri->mutable_map(2)->mutable_objtable(0)->set_address(0xbc80);
        ri->mutable_map(3)->mutable_objtable(0)->set_address(0xbc00);
        *ri->mutable_map(0)->mutable_palette() = p->palette(2).address();
        *ri->mutable_map(1)->mutable_palette() = p->palette(4).address();
        *ri->mutable_map(2)->mutable_palette() = p->palette(6).address();
        *ri->mutable_map(3)->mutable_palette() = p->palette(4).address();
        ri->mutable_misc()->mutable_overworld_tile_palettes()->set_address(0x87e3);
    } else if (index == 2) {
        p->mutable_palette(0)->set_hidden(false);
        p->mutable_palette(1)->set_hidden(false);
        p->mutable_palette(2)->set_hidden(true);
        p->mutable_palette(3)->set_hidden(true);
        p->mutable_palette(4)->set_hidden(true);
        p->mutable_palette(5)->set_hidden(true);
        p->mutable_palette(6)->set_hidden(true);
        p->mutable_palette(7)->set_hidden(true);
        ri->mutable_map(0)->mutable_objtable(0)->set_address(0xb000);
        ri->mutable_map(1)->mutable_objtable(0)->set_address(0xb000);
        ri->mutable_map(2)->mutable_objtable(0)->set_address(0xb000);
        ri->mutable_map(3)->mutable_objtable(0)->set_address(0xb000);
        *ri->mutable_map(0)->mutable_palette() = p->palette(0).address();
        *ri->mutable_map(1)->mutable_palette() = p->palette(0).address();
        *ri->mutable_map(2)->mutable_palette() = p->palette(0).address();
        *ri->mutable_map(3)->mutable_palette() = p->palette(0).address();
        ri->mutable_misc()->mutable_overworld_tile_palettes()->set_address(0xb100);
    } else {
        p->mutable_palette(0)->set_hidden(false);
        p->mutable_palette(1)->set_hidden(false);
        p->mutable_palette(2)->set_hidden(true);
        p->mutable_palette(3)->set_hidden(true);
        p->mutable_palette(4)->set_hidden(true);
        p->mutable_palette(5)->set_hidden(true);
        p->mutable_palette(6)->set_hidden(true);
        p->mutable_palette(7)->set_hidden(true);
        ri->mutable_map(0)->mutable_objtable(0)->set_address(0x87a3);
        ri->mutable_map(1)->mutable_objtable(0)->set_address(0x87a3);
        ri->mutable_map(2)->mutable_objtable(0)->set_address(0x87a3);
        ri->mutable_map(3)->mutable_objtable(0)->set_address(0x87a3);
        *ri->mutable_map(0)->mutable_palette() = p->palette(0).address();
        *ri->mutable_map(1)->mutable_palette() = p->palette(0).address();
        *ri->mutable_map(2)->mutable_palette() = p->palette(0).address();
        *ri->mutable_map(3)->mutable_palette() = p->palette(0).address();
        ri->mutable_misc()->mutable_overworld_tile_palettes()->set_address(0x87e3);
    }
}

int OverworldTiles::EnabledIndex() const {
    const auto& ri = ConfigLoader<RomInfo>::GetConfig();
    int index = 0;
    for(const auto& hack : ri.overworld_tiles()) {
        if (hack.hack_size() > 0 && MemcmpHack(hack.hack(0))) {
            return index;
        }
        index++;
    }
    return -1;
}

bool OverworldTiles::MemcmpHack(const PokeData& data) const {
    for(int i=0; i<data.data_size(); i++) {
        if (mapper_->Read(data.address(), i) != data.data(i)) {
            return false;
        }
    }
    return true;
}

}  // namespace z2util