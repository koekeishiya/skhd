[![Build Status](https://travis-ci.org/koekeishiya/skhd.svg?branch=master)](https://travis-ci.org/koekeishiya/skhd)

**skhd** is a simple hotkey daemon for macOS. It is a stripped version of [**khd** (*no longer maintained*)](https://github.com/koekeishiya/khd)
(although rewritten from scratch), that sacrifices the more advanced features in favour of increased responsiveness and performance.
**skhd** is able to hotload its config file, meaning that hotkeys can be edited and updated live while **skhd** is running.

feature comparison between **skhd** and **khd**

| feature                    | skhd | khd |
|:--------------------------:|:----:|:---:|
| hotload config file        | [x]  | [ ] |
| hotkey passthrough         | [x]  | [x] |
| modal hotkey-system        | [x]  | [x] |
| use media-keys as hotkey   | [x]  | [ ] |
| application specific hotkey| [ ]  | [x] |
| modifier only hotkey       | [ ]  | [x] |
| caps-lock as hotkey        | [ ]  | [x] |
| mouse-buttons as hotkey    | [ ]  | [x] |

### Install

The first time **skhd** is ran, it will request access to the accessibility API.

After access has been granted, the application must be restarted.

*Secure Keyboard Entry* must be disabled for **skhd** to receive key-events.

**Homebrew**:

Requires xcode-8 command-line tools.

      brew install koekeishiya/formulae/skhd
      brew services start skhd

      stdout -> /tmp/skhd.out
      stderr -> /tmp/skhd.err

**Source**:

Requires xcode-8 command-line tools.

      git clone https://github.com/koekeishiya/skhd
      make install      # release version
      make              # debug version

### Usage

```
-v | --version: Print version number to stdout
    skhd -v

-c | --config: Specify location of config file
    skhd -c ~/.skhdrc

-k | --key: Synthesize a keypress (same syntax as when defining a hotkey)
    skhd -k "shift + alt - 7"

-t | --text: Synthesize a line of text
    skhd -t "ello, worldシ"
```

### Configuration

**skhd** will load the configuration file `$HOME/.skhdrc`, unless otherwise specified.

A sample config is available [here](https://github.com/koekeishiya/skhd/blob/master/examples/skhdrc)

A list of all built-in modifier and literal keywords can be found [here](https://github.com/koekeishiya/skhd/issues/1)

A hotkey is written according to the following rules:
```
hotkey   = <mode> '<' <action> | <action>

mode     = 'name of mode' | <mode> ',' <mode>

action   = <keysym> ':' <command> | <keysym> '->' ':' <command>
           <keysym> ';' <mode>    | <keysym> '->' ';' <mode>

keysym   = <mod> '-' <key> | <key>

mod      = 'modifier keyword' | <mod> '+' <mod>

key      = <literal> | <keycode>

literal  = 'single letter or built-in keyword'

keycode  = 'apple keyboard kVK_<Key> values (0x3C)'

->       = keypress is not consumed by skhd

command  = command is executed through '$SHELL -c' and
           follows valid shell syntax. if the $SHELL environment
           variable is not set, it will default to '/bin/bash'.
           when bash is used, the ';' delimeter can be specified
           to chain commands.

           to allow a command to extend into multiple lines,
           prepend '\' at the end of the previous line.

           an EOL character signifies the end of the bind.
```

A mode is declared according to the following rules:
```

mode_decl = '::' <name> '@' ':' <command> | '::' <name> ':' <command> |
            '::' <name> '@'               | '::' <name>

name      = desired name for this mode,

@         = capture keypresses regardless of being bound to an action

command  = command is executed through '$SHELL -c' and
           follows valid shell syntax. if the $SHELL environment
           variable is not set, it will default to '/bin/bash'.
           when bash is used, the ';' delimeter can be specified
           to chain commands.

           to allow a command to extend into multiple lines,
           prepend '\' at the end of the previous line.

           an EOL character signifies the end of the bind.
```
