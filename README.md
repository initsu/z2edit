# Z2edit for Zelda II Randomizer v5.0

This is a modified z2edit to:
- View palaces in Z2R v5.0+ where the sideview data has been moved to extended PRG banks.
- **Edit** palace rooms easier, working directly with PalaceRooms.json
  - Note: I say edit. For creating rooms from scratch, the original Z2Edit is still preferable.
  - Sideview data can be copied into a field, and **hit Enter** to re-create that room. When you are satisfied with the changes, press **Commit to ROM** to update the copy-paste-field, then copy that hex string back to PalaceRooms.json.
  - The same works for the enemy tab in the Sideview Editor.

Notes on using emulate:
- All the proper values for locations etc. will absolutely not be set.
- Shuffling palace index/region ("Allow Palaces to Swap Continents") will break things.
- Emulating Palace 1 rooms (**with the above flag off**) seems to work. End of list.

Original Z2edit repository is at: https://github.com/cfrantz/z2edit
