### Volume-Setter

Command line tool for Windows to set the audio volumes of running programs to
values defined in a config file.

I run the same programs at different volumes depending on the situation, with
e.g. game volume set lower when I'm on voice or watching a stream than when
I'm playing by myself. I wrote this program to stop having to mess around with
the Windows volume mixer and just press a button to set everything's volume to
what I wanted for the situation.

Usage is very simple; define a config file containing volume 'profiles' which
define volume levels for different applications, then switch to a profile
(e.g. the profile "foo") by calling `volume-setter foo` in a terminal.
The application will automatically find the config file
``%LOCALAPPDATA%\volume-setter\config.toml``, but if you want to put one
somewhere else then you can pass the path to it with the `--config` option.

#### Example Config

```toml
# Each section heading defines a 'profile' which is a collection of volume
# settings for different programs. Switching to a profile with
# `volume-setter default` would change the volumes of all running applications
# that match a control in the profile.
[default]
# A 'control' defines the relative volume of some set of running applications.
# Each must have a `suffix` string and a `volume` number. The `volume` number
# is a relative volume given between 0.0 and 1.0. The `suffix` provides a very
# simple string matching system; the end of the path to the executable file
# producing audio will be matched against the suffix, and if it matches, the
# volume will be changed. Later controls take priority over earlier ones.
# Some suffixes are special and are documented below.
controls = [
    # The special suffix `:device` matches the audio device through which the
    # audio is currently being outputted, such as speakers or headphones.
    # Other volumes are relative to this, so an application with volume `0.1`
    # would be reported by the volume mixer as having a volume of
    # `0.26 * 0.1 = 0.026`, display as `3`.
    { suffix = ":device", volume = 0.26 },
    # The special suffix `:system` matches the system sounds, such as
    # notification or error sounds from Windows.
    { suffix = ":system", volume = 0.1 },
    # Other suffixes are matched against the end of the path to the process
    # outputting audio. Backslashes should be escaped with ``\\`` as shown.
    { suffix = "\\steam.exe", volume = 0.3 },
    { suffix = "\\chrome.exe", volume = 0.9 },
]
```

#### Installing

You should be able to just download and run the executable from the Releases
page, no installer is required.

If building from source, download CMake and vcpkg then simply build and install
the CMake project as usual; the vcpkg integration should take care of
downloading the dependencies.
