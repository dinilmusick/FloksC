# FloksC Wave 3 source-pinned parity

TypeScript source: dinilmusick/Floks@7ab84526d7d9820a14de47b0ab33b568b8367460
Native semantic source: dinilmusick/FrnlC@1684071ce39ba539cab221810bdf0a8f12329ede
Native utility source: dinilmusick/GutC@0e26ac721c7a020078cfaf52e6fce259fe03301e

| Canonical source | FloksC adaptation | Coverage |
|---|---|---|
| authoring/types.ts, authoringModel.ts, authoringOperations.ts | `fc_author_session`, node add/update/subtree-delete, properties, state, slots, event mappings, external bindings, operations, metadata | Functional bounded native authoring session; immutable TypeScript copy-on-write intentionally maps to explicit in-place C11 mutation |
| canvas-ui/tile-menu/types.ts | FC_TILE/FC_MENU, explicit actions | Core control state; no badges |
| contracts/src/graph.ts | fc_graph ports, edges, viewport, cycle rejection | Functional bounded graph core; no predictive routing |
| MusicalTimeline.tsx | fc_timeline, loop and playhead | Functional data layer; host handles drag/render |
| MidiTimeline.tsx | fc_note, MIDI pitch/time transforms | Functional data layer; host handles canvas |
| FrnlC semantic runtime | fc_frnl_register and fc_frnl_event | Explicit bridge; host registers states first |
| Theming and layout | host-defined fc_theme table, fc_layout grid/row/column and fc_style | Functional basic theme selection and geometry; not full TypeScript theme packs |
| Authoring persistence | Versioned `fc_author_save/load` FCAS v1 plus `fc_ui_save/load`, `fc_graph_save/load`, `fc_timeline_save/load` | Explicit bounded binary snapshots; external text/JSON authoring schema translation stays a host adapter |

Strict C11, no implicit allocation, OS renderer, resident timer or static mutable globals. This first implementation is NOT a full Floks TypeScript feature parity certificate. Verification requires GitHub exact-head CI and external consumer, not merely compilation.

**FrnlC borrowed lifetime constraint:** `frnl_add_component` stores pointers to FloksC component IDs. When a component has been registered in the host FrnlC runtime, clean up that FrnlC runtime before destroying or reusing its FloksC storage. FrnlC 0.2 does not provide per-component unregister; the adapter does not fabricate one. Subsequent re-registration requires a fresh host runtime.
