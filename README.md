# lameo

Uhh, I don't know. I want it to at least run on both Windows and Linux, though.

This is a solo project. You can look at it, otherwise suggest things through
issues and/or pull requests even if you're a Schwungus dev.

This requires FMOD, just get it and move everything from
`C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\api\core`
to `include/fmod/win` for Windows or blahblahblah for Linux. CMake should do
the rest.