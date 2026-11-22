# FloksC Wave 3 source-pinned parity

TypeScript source: dinilmusick/Floks@7ab84526d7d9820a14de47b0ab33b568b8367460
Native semantic source: dinilmusick/FrnlC@1684071ce39ba539cab221810bdf0a8f12329ede
Native utility source: dinilmusick/GutC@0e26ac721c7a020078cfaf52e6fce259fe03301e

| Canonical source | FloksC adaptation | Coverage |
|---|---|---|
| authoring/authoringModel.ts and authoringOperations.ts | fc_add, fc_reparent, fc_destroy, fc_walk | Basic hierarchy only; no authoring sessions or bindings yet |
| canvas-ui/tile-menu/types.ts | FC_TILE/FC_MENU, explicit actions | Core control state; no badges |
| contracts/src/graph.ts | fc_graph ports, edges, viewport, cycle rejection | Functional bounded graph core; no predictive routing |
| MusicalTimeline.tsx | fc_timeline, loop and playhead | Functional data layer; host handles drag/render |
| MidiTimeline.tsx | fc_note, MIDI pitch/time transforms | Functional data layer; host handles canvas |
| FrnlC semantic runtime | fc_frnl_register and fc_frnl_event | Explicit bridge; host registers states first |
| Theming | fc_style descriptors | Basic style only, not full theme pack resolution |
| Authoring persistence | Not yet implemented | Outstanding serialization/configuration gap |

Strict C11, no implicit allocation, OS renderer, resident timer or static mutable globals. This first implementation is NOT a full Floks TypeScript feature parity certificate. Verification requires GitHub exact-head CI and external consumer, not merely compilation.
