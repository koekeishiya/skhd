**skhd** is a simple hotkey daemon for macOS. It is a stripped version of [**khd**](https://github.com/koekeishiya/khd)
(although mostly rewritten from scratch), that sacrifices the more advanced features in favour of increased responsiveness and performance.

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

#### Install

The first time **skhd** is ran, it will request access to the accessibility API.

After access has been granted, the application must be restarted.

*Secure Keyboard Entry* must be disabled for **skhd** to receive key-events.

**Source**:

Requires xcode-8 command-line tools,

      make install      # release version
      make              # debug version

#### Usage

```
-v | --version: Print version number to stdout
    skhd -v

-c | --config: Specify location of config file
    skhd -c ~/.skhdrc

```

#### Configuration

**skhd** will load the configuration file `$HOME/.skhdrc`, unless otherwise specified.

See [sample config](https://github.com/koekeishiya/skhd/blob/master/examples/skhdrc) for syntax information.
