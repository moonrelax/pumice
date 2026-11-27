# pumice

An experimental Nintendo Switch research emulator written in C.

pumice is a from-scratch HLE emulator targeting the Switch's Horizon OS
environment. It loads NSO/NRO binaries, resolves SDK symbol imports, and stubs
enough of the system services (sm, fs, nvdrv, vi, hid, audio, account, …) to
boot homebrew and watch how far commercial titles get. GPU commands are
translated to Vulkan via a Maxwell shader recompiler. It is a learning and
reverse-engineering project, not a playable emulator.

## Status

| area | state |
| --- | --- |
| NSO/NRO loader, MOD0 parser | not started |
| Horizon kernel (svc, threads, memory, IPC) | not started |
| Service manager (sm:) | not started |
| nvdrv / GPU command submission | not started |
| Vulkan GPU — Maxwell shader translator | not started |
| vi (display / compositor) | not started |
| hid (controller input) | not started |
| audio (audout / audren) | not started |
| filesystem (fsp-srv) | not started |

## Build

Requires a C compiler (gcc or clang), Make, and the Vulkan SDK.

```bash
make                         # builds build/pumice
make tests                   # unit tests
```

## Usage

```bash
pumice <game.nsp|game.nro>
```

Missing HLE stubs are logged at runtime so you can see what needs implementing
next.

## Layout

```
src/core     NSO/NRO loader, kernel svc emulation, IPC, threads
src/gpu      Maxwell command processor, shader translator, Vulkan renderer
src/hle      high-level emulation of Horizon OS services
tests        unit tests + homebrew samples
tools        helper scripts
```

## Legal

pumice contains no Nintendo code, firmware, keys or game data, and does not
decrypt anything. You need your own legally obtained, already-decrypted files.
Nintendo Switch is a trademark of Nintendo; this project is not affiliated
with it.

## License

Source-available — view and study only. See [LICENSE](LICENSE).
