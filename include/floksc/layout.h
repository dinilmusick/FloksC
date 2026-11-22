#ifndef FLOKSC_LAYOUT_H
#define FLOKSC_LAYOUT_H
#include "floksc.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Explicit, caller-owned style packs. No global theme registry or font provider. */
#define FC_THEME_KIND_COUNT ((size_t)FC_LABEL+1u)
typedef struct {char name[FC_NAME];bool present[FC_THEME_KIND_COUNT];fc_style styles[FC_THEME_KIND_COUNT];} fc_theme;
typedef struct {fc_theme *items;size_t count,capacity;} fc_themes;
fc_result fc_themes_init(fc_themes *themes,fc_theme *memory,size_t capacity);
fc_result fc_theme_add(fc_themes *themes,const char *name,size_t *index);
fc_result fc_theme_style(fc_themes *themes,size_t index,fc_kind kind,fc_style style);
fc_result fc_theme_apply(fc_ui *ui,fc_handle handle,const fc_theme *theme);
fc_result fc_theme_apply_tree(fc_ui *ui,fc_handle root,const fc_theme *theme);
typedef enum {FC_LAYOUT_ROW=1,FC_LAYOUT_COLUMN,FC_LAYOUT_GRID} fc_layout_kind;
typedef struct {fc_layout_kind kind;double x,y,width,height,padding,gap;size_t columns;} fc_layout;
/* Repositions direct children only; does not dispatch renderer callbacks.
   All bounds checked before modification, so invalid layouts are non-mutating. */
fc_result fc_layout_apply(fc_ui *ui,fc_handle parent,fc_layout config);
#ifdef __cplusplus
}
#endif
#endif
