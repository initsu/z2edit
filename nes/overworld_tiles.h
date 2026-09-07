#ifndef Z2HD_NES_OVERWORLD_TILES_H_
#define Z2HD_NES_OVERWORLD_TILES_H_

#include "nes/mapper.h"
#include "proto/rominfo.pb.h"

namespace z2util {

// Detects which "overworld tiles" layout the loaded ROM uses and adjusts the
// editor configuration so that overworld tile/palette data is read from the
// addresses the ROM actually uses.  Randomizer/hack ROMs relocate this data,
// and without this the map editor would read tiles from vanilla addresses.
//
// Detection only; never writes to the ROM itself.
class OverworldTiles {
  public:
    OverworldTiles() : mapper_(nullptr) {}

    void set_mapper(Mapper* m) { mapper_ = m; }
    // Re-run ROM detection and update config.  Call after loading a ROM and
    // before refreshing the map/palette editors.
    void Refresh();

  private:
    int EnabledIndex() const;
    bool MemcmpHack(const PokeData& data) const;

    Mapper* mapper_;
};

}  // namespace z2util
#endif // Z2HD_NES_OVERWORLD_TILES_H_