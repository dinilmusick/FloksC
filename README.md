# FloksC

Native C11 UI component authoring foundation for ULTRST, with generation-tagged handles, bounded component hierarchy, host events/render callbacks, typed graph editing and MIDI piano-roll time primitives.

The host supplies all storage, FrnlC runtime and rasterization. No implicit heap allocation, GUI framework or OS event loop is required. `FrnlC` 0.2 is the installed build dependency.

Build: `cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/frnlc/install`; `cmake --build build`; `ctest --test-dir build --output-on-failure`. See `examples/consumer` for a separate installed consumer, and `docs/PARITY.md` for pinned sources and remaining gaps.

Bounded persistence: include `<floksc/serialization.h>` and use the distinct UI, graph and timeline save/load functions; callers supply buffers and decode scratch arrays. Snapshots are little-endian, versioned and require binary64 floating point; unsupported hosts return `FC_MISMATCH`. Invalid, truncated and cyclic data are rejected without committing target state.
