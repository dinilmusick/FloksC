#ifndef FLOKSC_SERIALIZATION_H
#define FLOKSC_SERIALIZATION_H
#include "floksc.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Versioned, deterministic little-endian snapshots. IEEE binary64 doubles required.
   Decode is transactional: caller supplies disjoint scratch arrays; target changes
   only after full validation. No allocation, I/O, locale, globals or OS dependencies.
   On insufficient output buffer, returns FC_CAPACITY and reports required bytes. */
fc_result fc_ui_save(const fc_ui *ui,uint8_t *out,size_t capacity,size_t *written);
fc_result fc_ui_load(fc_ui *ui,const uint8_t *data,size_t length,fc_component *scratch,size_t scratch_count);
fc_result fc_graph_save(const fc_graph *graph,uint8_t *out,size_t capacity,size_t *written);
fc_result fc_graph_load(fc_graph *graph,const uint8_t *data,size_t length,
 fc_node *scratch_nodes,fc_edge *scratch_edges,uint8_t *scratch_walk);
fc_result fc_timeline_save(const fc_timeline *timeline,uint8_t *out,size_t capacity,size_t *written);
fc_result fc_timeline_load(fc_timeline *timeline,const uint8_t *data,size_t length,
 fc_note *scratch,size_t scratch_count);
#ifdef __cplusplus
}
#endif
#endif
