# Apple GPU monitoring on J716c Linux

Local patch on upstream btop v1.4.8. Recognizes the `aurora`, `asahi`, and
`apple-gpu` platform drivers without assuming a DRM card number.

The GPU panel reads the driver's `gpu_memory` ledger. **Used** is
`user_committed + kernel_resident`, matching the driver's allocation budget;
**Limit** is the device-wide allocation ceiling. This is shared system RAM,
not dedicated VRAM, and it excludes imported buffers accounted by their
exporter. A zero ceiling means unlimited and is not plotted as a percentage.

On kernels exposing qualified `agx_stats`, utilization is
`util_device_percent`, accepted only when `utilisation_status` is `valid`.
Missing, stale, malformed and out-of-range readings display as unavailable,
not zero. Unqualified legacy counters are not interpreted. Clocks, power,
temperature and bandwidth utilization remain unsupported.

## Build on framework

```sh
cmake -S . -B build-arm -G Ninja \
  -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
  -DCMAKE_EXE_LINKER_FLAGS='-static-libstdc++ -static-libgcc' \
  -DCMAKE_BUILD_TYPE=Release -DBTOP_GPU=ON -DBTOP_LTO=OFF \
  -DBUILD_TESTING=OFF
cmake --build build-arm -j 6
```

## Validation (2026-11-07)

- ARM64 and native builds passed.
- Standalone parser tests passed under ASan/UBSan, including malformed,
  missing, duplicate, negative, overflowing and stale readings.
- Native UI tested with an isolated mount namespace and synthetic sysfs:
  150x48 full layout and 80x24 GPU-only layout, unavailable -> 73% -> stale
  -> valid 0%. Both passed without showing stale utilization as zero.
- Live J716c test on `7.3.0-j716c-on1012g19+`: clean exit, real memory readings,
  49 valid utilization observations and transitions back to unavailable.
  Captured GPU panel showed 46% and 5.47 GiB allocated / 84.3 GiB limit.
- Evidence and UI harness are retained under `validation/` (not committed).

Standalone parser test:

```sh
g++ -std=c++23 -Wall -Wextra -Werror -fsanitize=address,undefined \
  tests/asahi_memory.cpp -o /tmp/btop-asahi-memory-test
/tmp/btop-asahi-memory-test
```

The Mac's patched binary is `/usr/local/bin/btop`; the distribution binary
remains `/usr/bin/btop`. Press **5** to toggle the GPU panel. The original
configuration is backed up beside `~/.config/btop/btop.conf` with a
`.pre-j716c-` timestamp suffix. No kernel or boot settings were changed.
