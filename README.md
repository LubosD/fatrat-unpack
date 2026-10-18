# fatrat-unpack

A FatRat plugin that unpacks the archive(s) a transfer downloaded: RAR (plain and multi-volume),
7z and ZIP (via libarchive), and `.001`/`.002`/… split files. It runs as a transfer action and a
background task, so it works the same from the desktop GUI and from `fatrat-nogui` plus the web
UI: forms replace the old modal dialogs, and the plugin itself needs no GUI.

## Building

```
cmake -B build -DFATRAT_INCLUDE_DIR=/path/to/fatrat/install/include -DWITH_LIBARCHIVE=ON
cmake --build build
```

`FATRAT_INCLUDE_DIR` must point at the `include` directory of a FatRat build new enough to
export the plugin API (`fatrat/plugins/PluginApi.h`; FatRat 1.3.1 and older don't have it). Build
against the headers of the *same* FatRat flavour you'll load this plugin into -- desktop or
headless (`fatrat-nogui`) -- since `Transfer`'s layout differs between the two and a plugin built
for one will call the wrong virtual function in the other.

At runtime, set `PLUGIN_PATH` to the directory holding the built `fatrat-unpack.so` if it isn't
already in FatRat's own plugin directory (reported by the build as "Plugin install directory").

### Options

| Option | Default | |
|---|---|---|
| `WITH_LIBARCHIVE` | OFF | 7z and ZIP support, via libarchive >= 3.2. Without it, only RAR and split files are handled. |
| `WITH_NLS` | OFF | Build and install the Czech, Slovak and Turkish translations. |
| `WITH_TESTS` | OFF | Build `autotests/`, run with `ctest`. |
| `WITH_ZIP` | OFF | Deprecated alias of `WITH_LIBARCHIVE`. |

## Using it

"Unpack archive(s)..." appears on a transfer whose downloaded data is, or contains, something
this plugin recognises. It asks what to do as a sequence of forms -- which archive (if there are
several), a password if one is needed, which files to extract and where -- and reports progress
and a result the same way any other FatRat task does.

## Piping into a command

The plugin can pipe one extracted file straight into a command's stdin (handy for playing a
single video out of an archive without extracting it first) instead of writing it to disk. This
is **disabled by default** and must be turned on in `fatrat.ini`:

```ini
[unpack]
allow_pipe=true
```

Turning it on lets anyone who can use FatRat's web UI run an arbitrary shell command as the user
FatRat runs as. Only enable it on a single-user machine, or behind a web UI you otherwise trust
every user of.

When enabled, the "choose files" form offers a few predefined commands (edit
`data/pipecmds`, installed to `share/fatrat/data/plugins/fatrat-unpack/pipecmds`, one
`Name|command` per line) plus a free-form custom command, run as `sh -c '<command>'`.

## Out of scope

Unpacking automatically when a transfer finishes, and a settings UI for native plugins, are not
implemented by this plugin or by FatRat itself yet.
