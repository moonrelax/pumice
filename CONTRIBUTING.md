# Contributing to pumice

Thanks for your interest. Please read and follow these rules before opening a
PR.

## Ground rules

- Open an issue first to discuss what you want to change.
- One PR = one feature or fix. Keep it small and focused.
- Do not submit code copied from other emulators or proprietary sources.
- All contributions fall under the project's source-available license.

## Commit messages

Use this format:

```
area: short summary in lowercase

Optional body explaining the why, not the what.
```

Examples:

```
hle/nvdrv: add ioctl handler for NVGPU_GPU_IOCTL_GET_CHARACTERISTICS
gpu: implement Maxwell texture descriptor parsing
core/loader: handle MOD0 offset in NSO header
```

Areas: `core`, `gpu`, `hle`, `hle/kernel`, `hle/nvdrv`, `hle/vi`,
`hle/hid`, `hle/audio`, `hle/fs`, `tests`, `tools`, `docs`.

Rules:
- Lowercase, no period at the end
- Imperative mood ("add", "fix", "remove", not "added", "fixes", "removed")
- Summary under 72 characters
- Body wrapped at 72 characters

## Code style

- C11, no C++ or compiler extensions unless necessary
- 4-space indentation, no tabs
- `snake_case` for functions and variables
- Keep functions short — if it scrolls, split it
- No trailing whitespace

## Pull requests

1. Fork the repo and create your branch from `main`
2. Make sure your code compiles with no warnings (`-Wall -Wextra`)
3. Test with at least one homebrew sample from `tests/`
4. Describe what your PR does and why in the description

## Questions

Open an issue. There is no Discord or chat yet.
