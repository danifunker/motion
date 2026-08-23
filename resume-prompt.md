# Resume prompt — motion (SGI IRIS 3130 emulator)

Paste the section below into a new session. Everything after the horizontal rule is the prompt.

---

We're working on `motion`, an SGI IRIS 3130 emulator at `/home/dani/repos/motion`. Continue from where
the last session stopped. The last session's work is **uncommitted** — read the working tree, not just
`git log`. Note `CLAUDE.md` bars AI-generated code from the repo; the owner has asked for code changes
directly in-session, so make them but leave everything uncommitted for review.

## Where we are

The IRIX kernel **boots**. It loads from disk, relocates its page map, calibrates its delay loop,
initialises the keyboard, probes the graphics boards, brings up its own DSD disk driver, reads
sectors, probes for the boards that aren't emulated, and then parks in `_swtch` on `stop #$2000`
waiting for an interrupt.

**The next thing it needs is interrupts.** Nothing in the emulator asserts IRQ to the CPU — the DUART
even says so in a comment. Without a periodic clock tick the scheduler has nothing to wake it, so the
kernel idles forever. That means: a real interrupt path from devices to Moira, the IP2's interrupt
vector PROM (MAME `cpu_map`, `0xfffffff0..0xffffffff`, vector = `BIT(offset,1,3) << 6 | m_lint`), and
the DUART timer wired to it.

## Build and run

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j$(nproc)
cd build/output/RelWithDebInfo && DISPLAY=:1 ./motion +set skipLauncher 1 +set startPaused 0
```

* **RelWithDebInfo on purpose.** `Debug` defines `DEBUG`, which enables `MOTION_ASSERT`; the cylinder
  assert in `CHSToLinear` fires on this disk image (the PROM programs a synthetic 2000-cylinder
  geometry smaller than the 60 MB image). It also turns on ASan and `-O0`.
* Disk image is staged at `build/output/RelWithDebInfo/profile/3130.img`.
* Log is `build/output/RelWithDebInfo/motion.log`. A healthy 25-second run is **~100 lines** and ends
  with the kernel initialising disk units 0 and 1 at 987 cylinders / 7 heads / 17 sectors. Hundreds of
  thousands of lines means something regressed.
* `+set logCpuTrace 1` turns on the CPU bring-up instrumentation (see below). Off by default.
* Kill with `pkill -x motion`. It segfaults on shutdown — pre-existing, the author's v0.2.0 TODO calls
  the shutdown path "ShitDown".

## Facts that were expensive to derive — don't rediscover these

**Disk image byte order.** Use `~/3130.img`, NOT `~/3130-swab16.img`. The raw dump is stored with each
16-bit pair swapped, which is correct: the IP2 crosses the Multibus byte lanes, so **Multibus byte N is
the byte the 68020 wrote at N ^ 1**.

**32-bit control-block fields are stored ROL16.** PROM routine `0x3000da16` is literally `ROL.L #16`.
The DSD's `Write16` little-endian split already reproduces this.

**The Multibus slave map is the whole ballgame.** The IP2 does not put its RAM on the backplane
directly. Sheet 14 (MAP.ADDRESS.GENERATION) sits four AM2148 1Kx4 SRAMs in between: 256 usable entries
of 16 bits, low 14 bits a 4KB frame number. Multibus `0x000000-0x0FFFFF` is a window onto system RAM
through that map; Multibus `0x100000-0x1FFFFF` **is** the map, one entry per 4KB block. The CPU reaches
both through segment 4. See `Multibus::DecodeSlave`.
  * PROM `0x30000770` fills all 256 entries with `[0x33000010] + i` (`= 0xf00 + i`, i.e. the top
    megabyte of RAM) before booting anything.
  * PROM `0x30005cfe` then *slides* the window over low RAM while loading the kernel — that's why the
    three ~124 KB text chunks all DMA to the same Multibus addresses.
  * The kernel re-does this for itself: `_init_mbmap` (`0x20036e6e`) invalidates all 256 entries, then
    `0x20032584` / `0x20036fb2` / `0x200370a2` point individual entries at its own buffers.

**Segments 4 and 5 do not go through the page map.** MAME's `mem_map` sends segment 3 to `sys_map` and
4/5 straight to the bus. Running segment 4 through the map with a base of zero is what was silently
eating the PROM's map programming.

**The DSD CIB pointer names the CIB's byte 4, not its base.** SGI hands the controller `lea 4(cib)`.
This used to be handled by rounding block pointers down to 16 bytes, which is the same thing only
while the block is paragraph-aligned — the PROM's are, the kernel's are not (CCB at multibus `0x1cde`,
CIB at `0x1cee`), and `dsdinit` then printed `dsd0: ccb timeout during init`. See
`DSD5217_CIB_PTR_BIAS`. The CCB and IOPB pointers are used as they stand.

**The DUART counter/timer gates everything after `_main`.** `_calibuzz`/`_buzztest` program ACR `0xbb`
(counter, X1/16 = 230400 Hz), preload `0xffff`, start via a read of register `0x0e`, spin, stop via
`0x0f`, and divide to get `_millibuzz`. Without a counter that counts, `_msdelay` never returns.

**Kernel layout on disk.** a.out header at file offset `0x1e800`; text `0x534e4`, data `0x12cd0`, bss
`0x2f014`; entry at header `+0x1c` = `0x20000400`. Text starts at file `0x1e820` and loads at virtual
`0x20000000`. `start` sets `VBR = 0x20000000`, copies 256 PTEs from index `0x3E00` down to `0x3400`
(`KSTARTPG`), sets `osBase = 0x34`, zeroes bss, then calls `_mlsetup`, `_main`, `_sureg`. It maps one
stack page via a single PTE at `0x3b00dffc` (index `0x37ff`) and sets `a7 = 0x20400000`.

**The kernel has a full symbol table** — 2541 symbols, a.out `nlist` at file offset
`0x1e820 + text + data`, 12 bytes each, string table after. Parsing it turns every address into a name
and is by far the highest-leverage thing to do first. Scratch scripts from last session:
`kdis.py` (disassemble kernel), `pdis.py` (disassemble PROM), `ksyms.py`, `a2s.py` (address → symbol).
Recreate with `pip download capstone` and unzip the wheel; it has m68k support.

## References available

* `~/IRIS3130.zip` — IP2/BP3/UC4/IM1 schematics (scans, no text layer) and assorted firmware.
  **No GF2 schematic.** IP2 sheet index: 8 MOUSE/PARITY, 14 MAP.ADDRESS.GENERATION, 15 PROCESSOR.MAP,
  16 PROTECTION/LIMIT, 18 MULTIBUS.MAP.AND.CONTROL.
* **MAME is the best reference for the IP2** — `https://raw.githubusercontent.com/mamedev/mame/master/src/mame/sgi/ip2.cpp`.
  Complete register map, MMU, protection model, and the slave map. Prefer it over the scans.
* DSD 5217 manuals on bitsavers under `pdf/dsd/5215_5217/`.
* `dsd5217-analysis.md` in the repo root — writeup of the disk controller work. Note its §2.3
  (paragraph masking) is superseded, see above.

## Uncommitted changes from last session

* `component/multibus/multibus.{cpp,hpp}` — implemented the slave map (`DecodeSlave`, `slaveMap`), and
  deleted the kludge that mapped the top megabyte of RAM onto this component. Bus-master helpers
  (`ReadMB*`/`WriteMB*`) widened from 20 to 24 bits and rebased onto segment 4. Unmapped-access
  warnings rate limited.
* `component/ip2/ip2_mmu.{cpp,hpp}` — segments 4/5 no longer go through the page map. Page-table
  register window case range was inclusive of one entry past the array, so the kernel's PTE-clearing
  loop wrote out of bounds.
* `component/ip2/ip2_duart.{cpp,hpp}` — implemented the counter/timer (`UpdateCounter`,
  `GetCounterTickNs`); computed from elapsed time rather than ticked.
* `component/storage/dsd5217.{cpp,hpp}` — `DSD5217_CIB_PTR_BIAS` replaces the paragraph mask.
* `component/memory.{cpp,hpp}` — reads/writes wrapped with `addr %= GetRamCapacity()`, which also made
  the bounds check below it dead code. Absent RAM now reads zero and swallows writes, like MAME.
* `component/addrspace.{cpp,hpp}` — rate-limited unmapped logging; an `unmappedHook` for the CPU trace.
* `component/cpu/{cpu.hpp,mc68020.hpp,mc68020_core.cpp}` — `GetProgramCounter()`; `Tick` now catches
  exceptions escaping Moira (`processException` rethrows anything that isn't an address/bus fault, and
  a `DoubleFault` escapes too because it's thrown as a pointer and caught by reference — that can
  terminate the process); and the `logCpuTrace` instrumentation: a PC ring, a control-flow-edge dump
  on unmapped access, a one-shot kernel-entry map dump, and a periodic PC sample.
* `component/ip2/{prom.cpp,prom_sram.hpp}` — mapping ends are inclusive in `AddrSpace::GetMapping`, so
  `start + size` was one byte too long. Same fix in `memory.cpp` and both DUART mappings.

## Known gaps beyond this

No interrupts at all (see above — this is the blocker). No GF2 (no 3D), no Ethernet, no Interphase SMD
or Storager, no FPA, no DSD write path or tape/floppy. `AddrSpace::GetMapping` linear-scans an
`unordered_map` on every access. `Memory::Start` still writes a fake reset vector into RAM at 0 that
nothing needs any more. One unexplained `SIGABRT` was seen once, early in the graphics probe, and did
not recur in ~20 runs afterwards — the `Tick` catch should now contain that class of failure.
