# lameo

Uhh, I don't know. I want it to at least run on both Windows and Linux, though.

This is a solo project. You can look at it, otherwise suggest things through
issues and/or pull requests, even if you're a Schwungus dev.

## External Dependencies

### [FMOD Engine 2.03.08](https://www.fmod.com/download#fmodengine)

Move everything from `C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\api\core`
to `include/fmod/windows` on Windows. Similar process on Linux.

### [Steamworks SDK 1.62](https://partner.steamgames.com/dashboard)

Place the SDK's zip and a `steam_appid.txt` with the application ID of your
choice in `include/steam`.
