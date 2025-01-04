# Title Music Engine

The title music engine is different from the engine that plays music
during the rest of the game.  In particular, it has a wider range of
notes that can be represented.  If you want to look into the actual
code, the title screen music engine main loop is at `$8000` (vesus
`$9000` for the game music loop).

The metadata for the songs is all the same as in the other areas,
although the title music is actually broken up into several songs
itself.  These song boundaries control the timing of the title scroll.

The following songs exist in the title song table:

  1. Intro
  2. Start
  3. Build up
  4. Main
  5. Breakdown

The start of song 2 triggers the title to scroll into view.  The start
of song 4 triggers a countdown until the story scroll begins.  After
song 5 finishes, the engine loops back to song 2.

Within the note data, a special format is used.  Rather than encoding
the duration and pitch together in a single byte, the title music has
"pitch" bytes and "duration" bytes.  Any byte with the highest bit set
(i.e. anything >= `$80`) is interpretted as a duration change, which
sets the duration of all future notes.  Anything else is a pitch value
which indicates a note of the current duration should be played.  The
duration values appear to key into an lookup table, but I am not sure
where it is.  My notes have the following durations for the table:

  * `$80` - 6 ticks (sixteenth note)
  * `$82` - 12 ticks (eighth note)
  * `$83` - 24 ticks (quarter note)
  * `$84` - 36 ticks (dotted quarter note)
  * `$85` - 48 ticks (half note)
  * `$86` - 96 ticks (whole note)
  * `$8A` - 60 ticks

The pitch values are much more straightforward.  As far as I can tell,
`$4C` is A4 and every semitone away from that adds or subtracts 2 from
the value.  Thus, C5 (three semitones above A4) is `$52`.  The value
`$02` represents a rest.

By way of an example, let's look at the main section of the vanilla
title music.  The title metadata starts at bank 6 `$84DA`:

    08 11 14 16 19 1E 1E 1E
             ^ Main section

The main section is the fourth song, starting at `$84DA + $16 = $84F0`:

    3F 3F 00

This means there is a single phrase for the main section, which is
repeated twice.  The phrase data is at `$84DA + $3F = $8519`.

    00 3D 86 3A 1A 5F

From this, we see the melody note data is located at `$863D`:

    83 Quarter notes
    02 Rest
    48 Ab4
    82 Eighth note
    46 G4
    83 Quarter notes
    3E Eb4
    34 Bb3
    84 Dotted quarter notes
    2E G3
    83 Quarter notes
    30 Ab3
    34 Bb3
    3A Db4
    38 C4
    34 Bb3
    30 Ab3
    82 Eighth notes
    34 Bb3
    83 Quarter notes
    30 Ab3
    85 Half notes
    2E G3
    82 Quarter notes
    02 Rest
    00 End of Data
