#ifndef FLOKSC_H
#define FLOKSC_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "frnl_semantic.h"
#ifdef __cplusplus
extern "C" {
#endif
/* User-owned bounded arrays; no allocation, resident loop, OS or GUI dependencies. */
#define FC_NAME 32
#define FC_TEXT 64
#define FC_MAX_PORTS 8
typedef enum { FC_OK,FC_INVALID,FC_CAPACITY,FC_STALE,FC_DUPLICATE,FC_CYCLE,FC_MISMATCH,FC_DISABLED,FC_FAILED,FC_OVERFLOW,FC_NOT_FOUND } fc_result;
typedef struct { uint32_t index,generation; } fc_handle;
typedef enum { FC_PANEL=1,FC_BUTTON,FC_INPUT,FC_MENU,FC_TILE,FC_SELECT,FC_TAB,FC_DIALOG,FC_LABEL } fc_kind;
typedef enum { FC_IDLE,FC_ACTIVE,FC_OPEN,FC_CLOSED } fc_state;
typedef struct {double x,y,w,h,font_size;uint32_t foreground,background;} fc_style;
typedef struct {bool live,enabled,selected;uint32_t generation;fc_handle parent;fc_kind kind;fc_state state;
 char id[FC_NAME],label[FC_TEXT],value[FC_TEXT];fc_style style;} fc_component;
typedef fc_result (*fc_render_fn)(void *,fc_handle,const fc_component *);
typedef fc_result (*fc_event_fn)(void *,fc_handle,const char *);
typedef struct {fc_component *entries;size_t capacity;fc_render_fn renderer;void *render_data;fc_event_fn event;void *event_data;} fc_ui;
fc_handle fc_invalid(void);
fc_result fc_init(fc_ui *ui,fc_component *storage,size_t capacity);
const fc_component *fc_get(const fc_ui *ui,fc_handle handle);
fc_result fc_add(fc_ui *ui,fc_kind kind,const char *id,fc_handle parent,fc_handle *handle);
fc_result fc_reparent(fc_ui *ui,fc_handle child,fc_handle parent);
fc_result fc_destroy(fc_ui *ui,fc_handle handle);
fc_result fc_label_set(fc_ui *ui,fc_handle handle,const char *text);
fc_result fc_value_set(fc_ui *ui,fc_handle handle,const char *text);
fc_result fc_style_set(fc_ui *ui,fc_handle handle,fc_style style);
fc_result fc_state_set(fc_ui *ui,fc_handle handle,fc_state state);
fc_result fc_enable(fc_ui *ui,fc_handle handle,bool enabled);
fc_result fc_select(fc_ui *ui,fc_handle handle,bool selected);
fc_result fc_activate(fc_ui *ui,fc_handle handle,const char *event);
fc_result fc_walk(const fc_ui *ui,fc_handle root,fc_render_fn callback,void *data);
fc_result fc_render(const fc_ui *ui);
void fc_renderer_bind(fc_ui *ui,fc_render_fn callback,void *data);
void fc_event_bind(fc_ui *ui,fc_event_fn callback,void *data);
fc_result fc_frnl_register(frnl_runtime *rt,const fc_ui *ui,fc_handle handle,const char *initial_state);
fc_result fc_frnl_event(frnl_runtime *rt,const fc_ui *ui,fc_handle handle,const char *event,const void *payload);
typedef enum {FC_PORT_IN=1,FC_PORT_OUT=2} fc_port_direction;
typedef enum {FC_SIGNAL_ANY,FC_SIGNAL_CONTROL,FC_SIGNAL_MIDI,FC_SIGNAL_AUDIO} fc_signal;
typedef struct {char id[FC_NAME];fc_port_direction direction;fc_signal signal;} fc_port;
typedef struct {uint32_t id;char label[FC_TEXT];double x,y,width,height;bool selected;fc_port ports[FC_MAX_PORTS];size_t ports_count;} fc_node;
typedef struct {uint32_t source,target;uint8_t output,input;} fc_edge;
typedef struct {fc_node *nodes;fc_edge *edges;uint8_t *scratch;size_t node_count,node_cap,edge_count,edge_cap;
 double zoom,pan_x,pan_y,min_zoom,max_zoom;} fc_graph;
fc_result fc_graph_init(fc_graph *g,fc_node *nodes,size_t node_cap,fc_edge *edges,size_t edge_cap,uint8_t *scratch);
fc_result fc_node_add(fc_graph *g,uint32_t id,const char *name,double x,double y);
fc_result fc_node_delete(fc_graph *g,uint32_t id);
fc_result fc_port_add(fc_graph *g,uint32_t id,const char *name,fc_port_direction direction,fc_signal signal);
fc_result fc_edge_add(fc_graph *g,uint32_t source,uint8_t output,uint32_t target,uint8_t input);
fc_result fc_node_move(fc_graph *g,uint32_t id,double x,double y);
fc_result fc_node_select(fc_graph *g,uint32_t id,bool selected);
fc_result fc_screen(const fc_graph *g,double x,double y,double *screen_x,double *screen_y);
fc_result fc_canvas(const fc_graph *g,double x,double y,double *canvas_x,double *canvas_y);
fc_result fc_zoom_at(fc_graph *g,double screen_x,double screen_y,double zoom);
typedef struct {uint64_t id;uint8_t pitch;double start,length;bool selected;} fc_note;
typedef struct {fc_note *notes;size_t count,capacity;uint64_t next_id;double length,loop_start,loop_end,playhead;} fc_timeline;
fc_result fc_timeline_init(fc_timeline *t,fc_note *storage,size_t capacity,double beats);
fc_result fc_note_add(fc_timeline *t,int pitch,double start,double duration,uint64_t *id);
fc_result fc_note_move(fc_timeline *t,uint64_t id,int pitch,double start);
fc_result fc_note_delete(fc_timeline *t,uint64_t id);
fc_result fc_loop_set(fc_timeline *t,double start,double end);
fc_result fc_seek(fc_timeline *t,double beat);
fc_result fc_pitch_y(int pitch,double height,double *y);
fc_result fc_y_pitch(double y,double height,int *pitch);
fc_result fc_x_beat(double x,double width,double beats,double *beat);
fc_result fc_beat_x(double beat,double width,double beats,double *x);
#ifdef __cplusplus
}
#endif
#endif
