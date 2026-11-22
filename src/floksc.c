#include "floksc/floksc.h"
#include <math.h>
#include <string.h>
#include <limits.h>
static bool finite(double x){return isfinite(x)!=0;}
static bool valid_text(const char *s,size_t max,bool empty){
 size_t i;if(!s)return false;
 for(i=0;i<max;i++){if(!s[i])return i>0||empty;}
 return false;
}
static bool valid_id(const char *s){
 size_t i;
 if(!valid_text(s,FC_NAME,false))return false;
 for(i=0;s[i];i++){if((unsigned char)s[i]<33||s[i]=='/'||s[i]=='\\')return false;}
 return true;
}
fc_handle fc_invalid(void){fc_handle h={UINT32_MAX,0};return h;}
static bool no_parent(fc_handle h){return h.index==UINT32_MAX&&h.generation==0;}
static fc_handle at(const fc_ui *ui,size_t i){fc_handle h={(uint32_t)i,ui->entries[i].generation};return h;}
const fc_component *fc_get(const fc_ui *ui,fc_handle h){
 const fc_component *c;
 if(!ui||!ui->entries||h.index>=ui->capacity||!h.generation)return NULL;
 c=&ui->entries[h.index];return c->live&&c->generation==h.generation?c:NULL;
}
static fc_component *mut(fc_ui *ui,fc_handle h){return (fc_component *)fc_get(ui,h);}
fc_result fc_init(fc_ui *ui,fc_component *mem,size_t n){
 if(!ui||!mem||!n||n>=UINT32_MAX||n>SIZE_MAX/sizeof(*mem))return FC_INVALID;
 memset(ui,0,sizeof(*ui));memset(mem,0,n*sizeof(*mem));ui->entries=mem;ui->capacity=n;return FC_OK;
}
fc_result fc_add(fc_ui *ui,fc_kind kind,const char *id,fc_handle parent,fc_handle *out){
 size_t i;fc_component *c;uint32_t generation;
 if(out)*out=fc_invalid();
 if(!ui||!ui->entries||!out||kind<FC_PANEL||kind>FC_LABEL||!valid_id(id))return FC_INVALID;
 if(!no_parent(parent)&&!fc_get(ui,parent))return FC_STALE;
 for(i=0;i<ui->capacity;i++){if(ui->entries[i].live&&strcmp(ui->entries[i].id,id)==0)return FC_DUPLICATE;}
 for(i=0;i<ui->capacity;i++){if(!ui->entries[i].live&&ui->entries[i].generation!=UINT32_MAX)break;}
 if(i==ui->capacity)return FC_CAPACITY;
 c=&ui->entries[i];generation=c->generation?c->generation:1;
 memset(c,0,sizeof(*c));c->generation=generation;c->live=true;c->enabled=true;
 c->kind=kind;c->state=kind==FC_DIALOG?FC_CLOSED:FC_IDLE;c->parent=parent;
 c->style=(fc_style){0,0,100,32,12,0,0};memcpy(c->id,id,strlen(id)+1);
 *out=at(ui,i);return FC_OK;
}
static size_t depth(const fc_ui *ui,fc_handle root,fc_handle h){
 size_t i;for(i=0;i<=ui->capacity;i++){
  const fc_component *c=fc_get(ui,h);
  if(!c)return SIZE_MAX;
  if(h.index==root.index&&h.generation==root.generation)return i;
  if(no_parent(c->parent))return SIZE_MAX;
  h=c->parent;
 }
 return SIZE_MAX;
}
fc_result fc_reparent(fc_ui *ui,fc_handle child,fc_handle parent){
 fc_component *c=mut(ui,child);
 if(!c)return FC_STALE;
 if(!no_parent(parent)&&!fc_get(ui,parent))return FC_STALE;
 if(!no_parent(parent)&&depth(ui,child,parent)!=SIZE_MAX)return FC_CYCLE;
 c->parent=parent;return FC_OK;
}
static void dead(fc_component *c){
 uint32_t next=c->generation==UINT32_MAX?UINT32_MAX:c->generation+1;
 memset(c,0,sizeof(*c));c->generation=next;
}
fc_result fc_destroy(fc_ui *ui,fc_handle root){
 size_t d,i;if(!fc_get(ui,root))return FC_STALE;
 for(d=ui->capacity;d>0;d--){
  for(i=0;i<ui->capacity;i++){if(ui->entries[i].live&&depth(ui,root,at(ui,i))==d)dead(&ui->entries[i]);}
 }
 dead(&ui->entries[root.index]);return FC_OK;
}
fc_result fc_label_set(fc_ui *ui,fc_handle h,const char *s){
 fc_component *c=mut(ui,h);if(!c)return FC_STALE;
 if(!valid_text(s,FC_TEXT,true))return FC_INVALID;
 memcpy(c->label,s,strlen(s)+1);return FC_OK;
}
fc_result fc_value_set(fc_ui *ui,fc_handle h,const char *s){
 fc_component *c=mut(ui,h);if(!c)return FC_STALE;
 if(c->kind!=FC_INPUT&&c->kind!=FC_LABEL)return FC_MISMATCH;
 if(!valid_text(s,FC_TEXT,true))return FC_INVALID;
 memcpy(c->value,s,strlen(s)+1);return FC_OK;
}
fc_result fc_style_set(fc_ui *ui,fc_handle h,fc_style s){
 fc_component *c=mut(ui,h);if(!c)return FC_STALE;
 if(!finite(s.x)||!finite(s.y)||!finite(s.w)||!finite(s.h)||!finite(s.font_size)||
 s.w<0||s.h<0||s.font_size<=0)return FC_INVALID;
 c->style=s;return FC_OK;
}
fc_result fc_state_set(fc_ui *ui,fc_handle h,fc_state s){
 fc_component *c=mut(ui,h);if(!c)return FC_STALE;
 if(s>FC_CLOSED)return FC_INVALID;
 if(c->kind!=FC_DIALOG&&(s==FC_OPEN||s==FC_CLOSED))return FC_MISMATCH;
 c->state=s;return FC_OK;
}
fc_result fc_enable(fc_ui *ui,fc_handle h,bool enabled){
 fc_component *c=mut(ui,h);if(!c)return FC_STALE;
 c->enabled=enabled;return FC_OK;
}
fc_result fc_select(fc_ui *ui,fc_handle h,bool selected){
 fc_component *c=mut(ui,h);size_t i;if(!c)return FC_STALE;
 if(c->kind!=FC_TILE&&c->kind!=FC_TAB&&c->kind!=FC_SELECT)return FC_MISMATCH;
 if(selected&&(c->kind==FC_TAB||c->kind==FC_SELECT)){
  for(i=0;i<ui->capacity;i++){
   fc_component *x=&ui->entries[i];
   if(x->live&&x->kind==c->kind&&x->parent.index==c->parent.index&&x->parent.generation==c->parent.generation)x->selected=false;
  }
 }
 c->selected=selected;return FC_OK;
}
fc_result fc_activate(fc_ui *ui,fc_handle h,const char *event){
 fc_component *c=mut(ui,h);if(!c)return FC_STALE;
 if(!c->enabled)return FC_DISABLED;
 if(!valid_id(event))return FC_INVALID;
 if(ui->event&&ui->event(ui->event_data,h,event)!=FC_OK)return FC_FAILED;
 if(c->kind==FC_BUTTON)c->state=FC_ACTIVE;
 if(c->kind==FC_DIALOG)c->state=c->state==FC_OPEN?FC_CLOSED:FC_OPEN;
 return FC_OK;
}
fc_result fc_walk(const fc_ui *ui,fc_handle root,fc_render_fn callback,void *data){
 size_t level,i;
 if(!ui||!ui->entries||!callback)return FC_INVALID;
 if(!no_parent(root)&&!fc_get(ui,root))return FC_STALE;
 for(level=0;level<ui->capacity;level++){
  for(i=0;i<ui->capacity;i++){
   size_t d=0;fc_handle h;const fc_component *c;
   if(!ui->entries[i].live)continue;
   h=at(ui,i);
   if(no_parent(root)){
    c=fc_get(ui,h);
    while(c&&!no_parent(c->parent)&&d<ui->capacity){h=c->parent;c=fc_get(ui,h);d++;}
    if(!c)return FC_STALE;
    if(d>=ui->capacity)return FC_CYCLE;
   }else d=depth(ui,root,h);
   if(d==level&&callback(data,at(ui,i),&ui->entries[i])!=FC_OK)return FC_FAILED;
  }
 }
 return FC_OK;
}
void fc_renderer_bind(fc_ui *ui,fc_render_fn fn,void *data){if(ui){ui->renderer=fn;ui->render_data=data;}}
void fc_event_bind(fc_ui *ui,fc_event_fn fn,void *data){if(ui){ui->event=fn;ui->event_data=data;}}
fc_result fc_render(const fc_ui *ui){return ui?fc_walk(ui,fc_invalid(),ui->renderer,ui->render_data):FC_INVALID;}
static fc_node *find(fc_graph *g,uint32_t id){
 size_t i;if(!g||!id)return NULL;
 for(i=0;i<g->node_count;i++){if(g->nodes[i].id==id)return &g->nodes[i];}
 return NULL;
}
fc_result fc_graph_init(fc_graph *g,fc_node *n,size_t nc,fc_edge *e,size_t ec,uint8_t *scratch){
 if(!g||!n||!e||!scratch||!nc||!ec||nc>SIZE_MAX/sizeof(*n)||ec>SIZE_MAX/sizeof(*e))return FC_INVALID;
 memset(g,0,sizeof(*g));memset(n,0,nc*sizeof(*n));memset(e,0,ec*sizeof(*e));memset(scratch,0,nc);
 g->nodes=n;g->edges=e;g->scratch=scratch;g->node_cap=nc;g->edge_cap=ec;
 g->zoom=1;g->min_zoom=.25;g->max_zoom=2.5;return FC_OK;
}
fc_result fc_node_add(fc_graph *g,uint32_t id,const char *name,double x,double y){
 fc_node *n;
 if(!g||!id||!valid_text(name,FC_TEXT,false)||!finite(x)||!finite(y))return FC_INVALID;
 if(find(g,id))return FC_DUPLICATE;
 if(g->node_count>=g->node_cap)return FC_CAPACITY;
 n=&g->nodes[g->node_count++];memset(n,0,sizeof(*n));n->id=id;n->x=x;n->y=y;n->width=150;n->height=100;
 memcpy(n->label,name,strlen(name)+1);return FC_OK;
}
fc_result fc_node_delete(fc_graph *g,uint32_t id){
 size_t i,j;if(!find(g,id))return FC_NOT_FOUND;
 for(i=0;i<g->edge_count;){
  if(g->edges[i].source==id||g->edges[i].target==id){
   for(j=i+1;j<g->edge_count;j++){g->edges[j-1]=g->edges[j];}
   g->edge_count--;
  }else i++;
 }
 for(i=0;i<g->node_count;i++){if(g->nodes[i].id==id)break;}
 for(j=i+1;j<g->node_count;j++){g->nodes[j-1]=g->nodes[j];}
 g->node_count--;memset(&g->nodes[g->node_count],0,sizeof(*g->nodes));return FC_OK;
}
fc_result fc_port_add(fc_graph *g,uint32_t id,const char *name,fc_port_direction dir,fc_signal sig){
 fc_node *n=find(g,id);fc_port *p;size_t i;
 if(!n)return FC_NOT_FOUND;
 if(!valid_id(name)||(dir!=FC_PORT_IN&&dir!=FC_PORT_OUT)||sig>FC_SIGNAL_AUDIO)return FC_INVALID;
 for(i=0;i<n->ports_count;i++){if(strcmp(n->ports[i].id,name)==0)return FC_DUPLICATE;}
 if(n->ports_count==FC_MAX_PORTS)return FC_CAPACITY;
 p=&n->ports[n->ports_count++];memset(p,0,sizeof(*p));
 memcpy(p->id,name,strlen(name)+1);p->direction=dir;p->signal=sig;return FC_OK;
}
static bool reachable(fc_graph *g,uint32_t from,uint32_t to){
 size_t i,j,round;memset(g->scratch,0,g->node_cap);
 for(i=0;i<g->node_count;i++){if(g->nodes[i].id==from)g->scratch[i]=1;}
 for(round=0;round<g->node_count;round++){
  bool changed=false;
  for(i=0;i<g->edge_count;i++){
   size_t s=g->node_count,t=g->node_count;
   for(j=0;j<g->node_count;j++){
    if(g->nodes[j].id==g->edges[i].source)s=j;
    if(g->nodes[j].id==g->edges[i].target)t=j;
   }
   if(s<g->node_count&&t<g->node_count&&g->scratch[s]&&!g->scratch[t]){
    g->scratch[t]=1;changed=true;
   }
  }
  if(!changed)break;
 }
 for(i=0;i<g->node_count;i++){if(g->nodes[i].id==to&&g->scratch[i])return true;}
 return false;
}
fc_result fc_edge_add(fc_graph *g,uint32_t source,uint8_t output,uint32_t target,uint8_t input){
 fc_node *a=find(g,source),*b=find(g,target);size_t i;
 if(!a||!b)return FC_NOT_FOUND;
 if(source==target)return FC_CYCLE;
 if(output>=a->ports_count||input>=b->ports_count)return FC_INVALID;
 if(a->ports[output].direction!=FC_PORT_OUT||b->ports[input].direction!=FC_PORT_IN)return FC_MISMATCH;
 if(a->ports[output].signal!=FC_SIGNAL_ANY&&b->ports[input].signal!=FC_SIGNAL_ANY&&
 a->ports[output].signal!=b->ports[input].signal)return FC_MISMATCH;
 for(i=0;i<g->edge_count;i++){
  fc_edge *e=&g->edges[i];
  if(e->source==source&&e->target==target&&e->output==output&&e->input==input)return FC_DUPLICATE;
 }
 if(g->edge_count==g->edge_cap)return FC_CAPACITY;
 if(reachable(g,target,source))return FC_CYCLE;
 g->edges[g->edge_count++]=(fc_edge){source,target,output,input};return FC_OK;
}
fc_result fc_node_move(fc_graph *g,uint32_t id,double x,double y){
 fc_node *n=find(g,id);if(!n)return FC_NOT_FOUND;
 if(!finite(x)||!finite(y))return FC_INVALID;
 n->x=x;n->y=y;return FC_OK;
}
fc_result fc_node_select(fc_graph *g,uint32_t id,bool selected){
 fc_node *n=find(g,id);if(!n)return FC_NOT_FOUND;
 n->selected=selected;return FC_OK;
}
fc_result fc_screen(const fc_graph *g,double x,double y,double *sx,double *sy){
 double a,b;
 if(!g||!sx||!sy||!finite(x)||!finite(y))return FC_INVALID;
 a=x*g->zoom+g->pan_x;b=y*g->zoom+g->pan_y;
 if(!finite(a)||!finite(b))return FC_OVERFLOW;
 *sx=a;*sy=b;return FC_OK;
}
fc_result fc_canvas(const fc_graph *g,double sx,double sy,double *x,double *y){
 double a,b;
 if(!g||!x||!y||!finite(sx)||!finite(sy)||g->zoom<=0)return FC_INVALID;
 a=(sx-g->pan_x)/g->zoom;b=(sy-g->pan_y)/g->zoom;
 if(!finite(a)||!finite(b))return FC_OVERFLOW;
 *x=a;*y=b;return FC_OK;
}
fc_result fc_zoom_at(fc_graph *g,double sx,double sy,double zoom){
 double x,y,px,py;
 if(!g||!finite(zoom)||zoom<g->min_zoom||zoom>g->max_zoom)return FC_INVALID;
 if(fc_canvas(g,sx,sy,&x,&y)!=FC_OK)return FC_INVALID;
 px=sx-x*zoom;py=sy-y*zoom;
 if(!finite(px)||!finite(py))return FC_OVERFLOW;
 g->pan_x=px;g->pan_y=py;g->zoom=zoom;return FC_OK;
}
fc_result fc_timeline_init(fc_timeline *t,fc_note *mem,size_t cap,double length){
 if(!t||!mem||!cap||cap>SIZE_MAX/sizeof(*mem)||!finite(length)||length<=0)return FC_INVALID;
 memset(t,0,sizeof(*t));memset(mem,0,cap*sizeof(*mem));
 t->notes=mem;t->capacity=cap;t->length=length;t->loop_end=length;t->next_id=1;return FC_OK;
}
fc_result fc_note_add(fc_timeline *t,int pitch,double start,double duration,uint64_t *id){
 if(id)*id=0;
 if(!t||!id||pitch<0||pitch>127||!finite(start)||!finite(duration)||start<0||
 duration<=0||start>=t->length||duration>t->length-start)return FC_INVALID;
 if(t->count==t->capacity)return FC_CAPACITY;
 if(t->next_id==UINT64_MAX)return FC_OVERFLOW;
 *id=t->next_id++;t->notes[t->count++]=(fc_note){*id,(uint8_t)pitch,start,duration,false};return FC_OK;
}
fc_result fc_note_move(fc_timeline *t,uint64_t id,int pitch,double start){
 size_t i;if(!t||!id)return FC_INVALID;
 for(i=0;i<t->count;i++){if(t->notes[i].id==id){
  if(pitch<0||pitch>127||!finite(start)||start<0||start>=t->length||
   t->notes[i].length>t->length-start)return FC_INVALID;
  t->notes[i].pitch=(uint8_t)pitch;t->notes[i].start=start;return FC_OK;
 }}
 return FC_NOT_FOUND;
}
fc_result fc_note_delete(fc_timeline *t,uint64_t id){
 size_t i,j;if(!t||!id)return FC_INVALID;
 for(i=0;i<t->count;i++){if(t->notes[i].id==id){
  for(j=i+1;j<t->count;j++){t->notes[j-1]=t->notes[j];}
  t->count--;memset(&t->notes[t->count],0,sizeof(*t->notes));return FC_OK;
 }}
 return FC_NOT_FOUND;
}
fc_result fc_loop_set(fc_timeline *t,double start,double end){
 if(!t||!finite(start)||!finite(end)||start<0||end>t->length||start>=end)return FC_INVALID;
 t->loop_start=start;t->loop_end=end;return FC_OK;
}
fc_result fc_seek(fc_timeline *t,double beat){
 if(!t||!finite(beat)||beat<0||beat>t->length)return FC_INVALID;
 t->playhead=beat;return FC_OK;
}
fc_result fc_pitch_y(int pitch,double height,double *y){
 if(!y||pitch<0||pitch>127||!finite(height)||height<=0)return FC_INVALID;
 *y=(127-pitch)/127.0*height;return FC_OK;
}
fc_result fc_y_pitch(double y,double height,int *pitch){
 if(!pitch||!finite(y)||!finite(height)||height<=0)return FC_INVALID;
 *pitch=(int)floor(127-fmax(0,fmin(1,y/height))*127+0.5);return FC_OK;
}
fc_result fc_x_beat(double x,double width,double beats,double *beat){
 if(!beat||!finite(x)||!finite(width)||!finite(beats)||width<=0||beats<=0)return FC_INVALID;
 *beat=fmax(0,fmin(beats,x/width*beats));return FC_OK;
}
fc_result fc_beat_x(double beat,double width,double beats,double *x){
 if(!x||!finite(beat)||!finite(width)||!finite(beats)||width<=0||beats<=0||beat<0||beat>beats)return FC_INVALID;
 *x=beat/beats*width;return FC_OK;
}
