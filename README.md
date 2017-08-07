**skhd** is a simple hotkey daemon for macOS. It is a stripped version of [**khd**](https://github.com/koekeishiya/khd)
(although mostly rewritten from scratch), that sacrifices the more advanced features in favour of increased responsiveness and performance.

**skhd** is able to hotload its config file, meaning that hotkeys can be edited and updated live while **skhd** is running.

**skhd** has an improved parser that is able to accurately identify both the line and character position of any symbol that does not
follow the correct grammar rules for defining a hotkey. If a syntax error is detected during parsing, the parser will print the error and abort.
**skhd** will still continue to run and when the syntax error has been fixed, it will automatically reload and reparse the config file.

feature comparison between **skhd** and **khd**

| feature                    | skhd | khd |
|:--------------------------:|:----:|:---:|
| hotload config file        | [x]  | [ ] |
| require unix domain socket | [ ]  | [x] |
| hotkey passthrough         | [x]  | [x] |
| modal hotkey-system        | [ ]  | [x] |
| app specific hotkey        | [ ]  | [x] |
| modifier only hotkey       | [ ]  | [x] |
| caps-lock as hotkey        | [ ]  | [x] |
| mouse-buttons as hotkey    | [ ]  | [x] |
| emit keypress              | [ ]  | [x] |
| autowrite text             | [ ]  | [x] |

### Install

The first time **skhd** is ran, it will request access to the accessibility API.

After access has been granted, the application must be restarted.

*Secure Keyboard Entry* must be disabled for **skhd** to receive key-events.

**Homebrew**:

      brew install koekeishiya/formulae/skhd
      brew services start skhd

      stdout -> /tmp/skhd.out
      stderr -> /tmp/skhd.err

**Source**:

Requires xcode-8 command-line tools,

      make install      # release version
      make              # debug version

### Usage

```
-v | --version: Print version number to stdout
    skhd -v

-c | --config: Specify location of config file
    skhd -c ~/.skhdrc

```

### Configuration

**skhd** will load the configuration file `$HOME/.skhdrc`, unless otherwise specified.

A list of all built-in modifier and literal keywords can be found [here](https://github.com/koekeishiya/skhd/issues/1)

A hotkey is written according to the following rules:
```
hotkey   = <keysym> ':' <command> |
           <keysym> '->' ':' <command>

keysym   = <mod> '-' <key> | <key>

mod      = 'built-in mod keyword' | <mod> '+' <mod>

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
