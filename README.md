# Sarangi (सारंगी) – free VST3 instrument

A physically modelled Nepali sarangi for FL Studio (or any VST3 host).
No samples: the sound is synthesised from a model of a bowed string, a
skin-covered body, and a set of sympathetic strings.

## Controls

| Knob | What it does |
|------|--------------|
| **BOW** | Bow pressure. Low = soft and airy, high = gritty, raspy and nasal |
| **MEEND** | Glide between notes. Overlap notes in the Piano Roll to slide |
| **GAMAK** | Vibrato that blooms in after each note starts |
| **TARAF** | Sympathetic strings that ring along when you play notes of the scale |
| **SPACE** | Room reverb |
| **VOLUME** | Output level |
| **Sa =** | Tonic the sympathetic strings are tuned to. Set it to your song's key |

The big text at the top shows the note you're playing in sargam
(Sa Re Ga ma Pa Dha Ni) relative to the Sa you picked.

It's monophonic, like the real instrument. Hold one note and press another to
slide (meend) to it; release to slide back.

## Getting the .vst3 file (no coding needed)

1. Make a free account at github.com and create a new **public** repository.
2. Click **Add file → Upload files** and drag in *everything* in this folder,
   including the hidden `.github` folder. (If your file browser hides it,
   create the file `.github/workflows/build.yml` with **Add file → Create new
   file** and paste its contents in.)
3. Open the **Actions** tab. The "Build Sarangi plugin" job starts by itself
   (click "Run workflow" if it doesn't). It takes about 5–10 minutes.
4. When it shows a green tick, open the run and download
   **Sarangi-Windows-VST3** at the bottom of the page. Unzip it.

## Installing in FL Studio (Windows)

1. Copy the `Sarangi.vst3` folder to `C:\Program Files\Common Files\VST3\`
2. In FL Studio: **Options → Manage plugins → Find installed plugins**.
3. Star *Sarangi* in the list, then add it from the Channel Rack (**+**).

## Building it yourself instead

Install Visual Studio 2022 Community (with "Desktop development with C++")
and CMake, then in this folder run:

```
cmake -S . -B build
cmake --build build --config Release
```

The plugin appears in `build/Sarangi_artefacts/Release/VST3/`.

## Playing tips

- Range: the real sarangi sits roughly from G3 to G5. It sounds best there.
- Use velocity: harder notes bow faster and louder.
- For meend, let notes overlap slightly in the Piano Roll; MEEND sets how slow
  the slide is.
- Turn TARAF up and play slow notes in your Sa scale to hear the shimmer.

`tools/render_demo.cpp` renders a demo WAV straight from the sound engine,
which is handy for tweaking the sound without opening a DAW.
