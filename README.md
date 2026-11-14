# Z2edit for Zelda II Randomizer v5.0

This is a modified z2edit to:
- View palaces in Z2R v5.0+ where the sideview data has been moved to extended PRG banks.
- **Edit** palace rooms easier, working directly with PalaceRooms.json
  - Note: I say edit. For creating rooms from scratch, the original Z2Edit is still preferable.
  - Sideview data can be copied into a field, and **hit Enter** to re-create that room. When you are satisfied with the changes, press **Commit to ROM** to update the copy-paste-field, then copy that hex string back to PalaceRooms.json.
  - The same works for the enemy tab in the Sideview Editor.

Notes on using emulate:
- **File > Emulate** launches a temporary copy of the current ROM. The room
  and connector **Emulate** buttons set a starting location on a best-effort
  basis. Start a new game from the title screen to apply it.
- Shuffling palace index/region ("Allow Palaces to Swap Continents") will break things. Don't do that if you want to edit the palaces.
- Your best bet is to start with Vanilla palaces with Full length as a base Z2R ROM.

See [BUILD-WINDOWS.md](BUILD-WINDOWS.md) for build instructions.

Original Z2edit repository is at: https://github.com/cfrantz/z2edit
