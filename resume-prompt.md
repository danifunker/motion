# Resume prompt — motion (SGI IRIS 3130 emulator)

Paste the section below into a new session. Everything after the horizontal rule is the prompt.

---

We're working on `motion`, an SGI IRIS 3130 emulator at `/home/dani/repos/motion`. Continue from where
the last session stopped. **Nothing is committed** — read the working tree, don't assume `git log` is
current. Note `CLAUDE.md` bars AI-generated code from the repo; the owner has asked for code changes
directly in-session, so make them but leave everything uncommitted for review.

## Goal

Get the IRIX kernel to actually run. It currently loads in full and starts executing, then corrupts a
pointer and the machine resets on a ~22 second loop.

## Build and run

```bash
# submodules are already populated (SDL3, imgui, --depth 1)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j$(nproc)
cd build/output/RelWithDebInfo && DISPLAY=:1 ./motion +set skipLauncher 1 +set startPaused 0
```

* **RelWithDebInfo on purpose.** `Debug` defines `DEBUG`, which enables `MOTION_ASSERT`; the cylinder
  assert in `CHSToLinear` fires on this disk image (the PROM programs a synthetic 2000-cylinder
  geometry that is smaller than the 60 MB image). It also turns on ASan and `-O0`.
* Disk image is already staged at `build/output/RelWithDebInfo/profile/3130.img`.
* Log is `build/output/RelWithDebInfo/motion.log`. A healthy 25-second run is ~300 lines. If you see
  hundreds of thousands, something regressed.
* Kill with `pkill -x motion`. It segfaults on shutdown — that's a known pre-existing issue (the
  author's own v0.2.0 TODO calls the shutdown path "ShitDown"), not your change.

## Facts that were expensive to derive — don't rediscover these

**Disk image byte order.** Use `~/3130.img`, NOT `~/3130-swab16.img`. The raw dump is stored with each
16-bit pair swapped, which is correct: the IP2 crosses the Multibus byte lanes, so **Multibus byte N is
the byte the 68020 wrote at N ^ 1**. `strings` on the swab16 file reads normally and on the raw file
reads as "rPai m1V07" — the raw one is the one that comes out right through the swap.

**32-bit control-block fields are stored ROL16.** PROM routine `0x3000da16` is literally `ROL.L #16`.
The DSD's `Write16` little-endian split already reproduces this, so the controller sees correct values.

**Control block pointers are masked to 16 bytes** (`& 0xFFFFF0`). SGI's driver hands the controller a
CIB pointer 4 bytes into its own struct and relies on the hardware rounding it down.

**Kernel layout on disk.** a.out-ish header at file offset `0x1e800`, entry at header `+0x1c`. Text
starts at file `0x1e820` and loads at virtual `0x20000000`. The first 1 KB is the 68000 exception
vector table; `0x20000400` (vector 1's value) is the entry point the PROM jumps to. Kernel startup sets
`VBR = 0x20000000`, `osBase = 0x3400`, and points its stack at `0x20400000` having mapped only ONE page
for it (a single PTE written to `0x3b00dffc`, index `0x37ff`).

## References available

* `~/IRIS3130.zip` — IP2/BP3/UC4/IM1 schematics (scans, no text layer), GF2/DC4/UC4/BP3/FP1 firmware,
  EXOS-201 Ethernet firmware, Storager 3030 ROMs, a second IP2 PROM revision. **No GF2 schematic.**
  IP2 schematic sheet index: 8 MOUSE/PARITY, 14 MAP.ADDRESS.GENERATION, 15 PROCESSOR.MAP,
  16 PROTECTION/LIMIT, 18 MULTIBUS.MAP.AND.CONTROL. Render with
  `pdftoppm -r 200 -png -f <n> -l <n> IP2_Schematic.pdf out` and crop — they're big.
* **MAME is the best reference for the IP2** — `https://raw.githubusercontent.com/mamedev/mame/master/src/mame/sgi/ip2.cpp`.
  It has the complete register map, the MMU, and the protection model. It corrected a schematic
  misreading last session; prefer it over squinting at scans.
* DSD 5217 manuals: `bitsavers.trailing-edge.com/pdf/dsd/5215_5217/040040-01_5215_Users_Guide_198404.pdf`
  and `.../040069-01_5217_Users_Guide_Addendu_198404.pdf`.
* `dsd5217-analysis.md` in the repo root — full writeup of the disk controller work.

## PROM disassembly (very useful, recreate as needed)

`pip download capstone` into a venv or unzip the wheel; it has m68k support. PROM loads at `0x30000000`:

```python
import sys, capstone
d = open('roms/iris3130/ip2/ip2_prom_3.0.10.bin','rb').read()
md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN | capstone.CS_MODE_M68K_020)
start, n = int(sys.argv[1],16), int(sys.argv[2],16)
for i in md.disasm(d[start:start+n], 0x30000000+start):
    print("%08x  %-20s %s %s" % (i.address, i.bytes.hex(), i.mnemonic, i.op_str))
```

For the **kernel**, read from `~/3130.img`, un-swap 16-bit pairs, text file offset `0x1e820` maps to
virtual `0x20000000`. Useful PROM addresses: `0x3000c8e2` DSD init, `0x3000d6ae` mdstrategy,
`0x3000d5c6` issue-command, `0x3000c2d8` md devsw strategy, `0x30011ce8` device switch table.

## Uncommitted changes already made

* `component/storage/dsd5217.{cpp,hpp}` — rewritten. The controller is a bus master and now decodes only
  its I/O port, chaining WUB→CCB→CIB→IOPB out of Multibus RAM. Previously it claimed a phantom 256-byte
  memory window that sat inside the PROM's DMA buffer and ate multi-sector transfers. **Verified working**:
  reads are contiguous and complete, full 417 KB kernel loads.
* `component/ip2/ip2_mmu.{cpp,hpp}` — corrected against MAME: 13-bit frame mask (was 14), 14-bit page
  number (was relying on uint16_t = 16), 12-bit page offset (was 13), 16384 entries, full MAME
  protection checks re-enabled, out-of-range index faults instead of reading OOB.
* `component/cpu/mc68020_moira_bridge.hpp`, `component/addrspace.{cpp,hpp}`, `component/cpu/mc68020_core.cpp`
  — 68020 bus errors. MMU records a fault, the bridge throws Moira's `BusError` with a stack frame.
  Faults are gated off while the CPU is in reset. Device-space holes (>= 0x30000000) and unmapped
  Multibus fault; absent RAM reads zero (MAME's model — the PROM's memory sizing depends on it).
* `component/multibus/multibus.cpp` — unmapped Multibus accesses fault (this is how `gl2_probe` detects a GF2).
* `component/ip2/prom.hpp`, `prom_sram.hpp`, `base/machine/machine_iris3130.cpp` — PROM and PROM SRAM are
  now early-start and added before the CPU. The reset vector and reset SP live in them and the CPU was
  resetting before they were mapped.
* `component/ip2/ip2_mouse.hpp` (new) — button register `0x30800000`, quadrature `0x31000000`. The PROM
  reads the first during boot phase 0 for the board revision.
* `component/ip2/ip2_dip_switches.hpp` — `switchState` was an uninitialised member; now `AUTOBOOT |
  BOOT_DEVICE_MD`. Clear autoboot if you want to stop at the PROM monitor.
* `coherent/coherent_core.cpp` — `Coherent::Init` used `Cvar::Set` for `startPaused`, clobbering the
  command line so the documented `+set startPaused 0` did nothing. Now `Cvar::Get`.

## The open problem

After the PROM jumps to `0x20000400` the kernel runs briefly, then a `bcopy` at `0x200533bc` is entered
with a source pointer of `0x33000800` and a nonsense length. `0x33000800` is the kernel's own initial
SSP (vector 0) — it's being read back off a stack that has gone somewhere wrong. It's now stopped at
the first bad fetch by the bus error rather than walking 341 KB through nothing.

Two concrete leads, neither chased yet:

1. **`Memory::Read8/16/32` do `addr %= GetRamCapacity()`** — they *wrap* rather than returning zero,
   which also makes the `>= capacity` check immediately below them dead code. High addresses alias onto
   low RAM. MAME returns zero. This could plausibly be corrupting things.
2. **Segment 3 bypasses translation entirely** in `IP2MMU::Translate` ("these don't seem to use virtual
   memory, so just ignore them"). MAME does not do this. Compare against MAME's `mem_map`/`sys_map`.

A PC ring-buffer in `MC68020::Tick` plus a dump hook in `AddrSpace`'s unmapped branch (logging
`getPC/getSP/getA(0)/getA(1)/getD(0)` and the top of stack) is what localised this last time — about 40
lines, worth redoing.

## Known gaps beyond this

No GF2 (geometry engine — no 3D at all), no Ethernet, no Interphase SMD or Storager, no FPA, no DSD
write path or tape/floppy, no DUART counters (the scheduler clock), and the unmapped-access warning
needs rate-limiting.
