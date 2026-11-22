#include "floksc/serialization.h"
#include <string.h>
#include <float.h>
#include <math.h>
#include <limits.h>

typedef struct {uint8_t *out;const uint8_t *in;size_t capacity,pos;fc_result err;} stream;
static bool binary64(void){return sizeof(double)==8&&DBL_MANT_DIG==53&&DBL_MAX_EXP==1024&&FLT_RADIX==2;}
static void write_bytes(stream *s,const void *data,size_t n){
 if(s->err!=FC_OK&&s->err!=FC_CAPACITY)return;
 if(n>SIZE_MAX-s->pos){s->err=FC_OVERFLOW;return;}
 if(s->err==FC_OK&&s->out){
  if(s->pos<=s->capacity&&n<=s->capacity-s->pos)memcpy(s->out+s->pos,data,n);
  else s->err=FC_CAPACITY;
 }
 s->pos+=n;
}
static void read_bytes(stream *s,void *out,size_t n){
 if(s->err!=FC_OK)return;
 if(s->pos>s->capacity||n>s->capacity-s->pos){s->err=FC_INVALID;return;}
 memcpy(out,s->in+s->pos,n);s->pos+=n;
}
static void w8(stream *s,uint8_t x){write_bytes(s,&x,1);}
static uint8_t r8(stream *s){uint8_t x=0;read_bytes(s,&x,1);return x;}
static void w32(stream *s,uint32_t x){size_t i;for(i=0;i<4;i++)w8(s,(uint8_t)(x>>(i*8)));}
static uint32_t r32(stream *s){size_t i;uint32_t x=0;for(i=0;i<4;i++)x|=((uint32_t)r8(s))<<(i*8);return x;}
static void w64(stream *s,uint64_t x){size_t i;for(i=0;i<8;i++)w8(s,(uint8_t)(x>>(i*8)));}
static uint64_t r64(stream *s){size_t i;uint64_t x=0;for(i=0;i<8;i++)x|=((uint64_t)r8(s))<<(i*8);return x;}
static void wd(stream *s,double x){
 uint64_t bits;if(!isfinite(x)){s->err=FC_INVALID;return;}
 memcpy(&bits,&x,sizeof(bits));w64(s,bits);
}
static double rd(stream *s){
 uint64_t bits=r64(s);double value=0;
 memcpy(&value,&bits,sizeof(value));if(!isfinite(value))s->err=FC_INVALID;
 return value;
}
static size_t slen(const char *s,size_t max){
 size_t i;if(!s)return SIZE_MAX;
 for(i=0;i<max;i++)if(!s[i])return i;
 return SIZE_MAX;
}
static void ws(stream *s,const char *str,size_t cap){
 size_t n=slen(str,cap);
 if(n==SIZE_MAX||n>UINT8_MAX){s->err=FC_INVALID;return;}
 w8(s,(uint8_t)n);write_bytes(s,str,n);
}
static void rs(stream *s,char *out,size_t cap){
 size_t n=r8(s),i;if(n>=cap){s->err=FC_INVALID;return;}
 memset(out,0,cap);read_bytes(s,out,n);
 if(s->err!=FC_OK)return;
 for(i=0;i<n;i++)if(!out[i]){s->err=FC_INVALID;return;}
 out[n]=0;
}
static void magic_write(stream *s,const char tag[4]){write_bytes(s,tag,4);}
static void magic_read(stream *s,const char tag[4]){
 char found[4]={0};read_bytes(s,found,4);
 if(s->err==FC_OK&&memcmp(found,tag,4)!=0)s->err=FC_INVALID;
}
static fc_result start_save(stream *s,uint8_t *buffer,size_t capacity,size_t *written){
 if(written)*written=0;
 if(!s||!written)return FC_INVALID;
 s->out=buffer;s->in=NULL;s->capacity=capacity;s->pos=0;s->err=FC_OK;
 if(!binary64())return FC_MISMATCH;
 return FC_OK;
}
static fc_result finish_save(stream *s,size_t *written){
 *written=s->pos;return s->err;
}
static fc_result start_load(stream *s,const uint8_t *data,size_t length){
 if(!s||!data||!length)return FC_INVALID;
 if(!binary64())return FC_MISMATCH;
 s->out=NULL;s->in=data;s->capacity=length;s->pos=0;s->err=FC_OK;return FC_OK;
}
static fc_result finish_load(const stream *s){return s->err!=FC_OK?s->err:(s->pos==s->capacity?FC_OK:FC_INVALID);}
static bool parent_missing(fc_handle p){return p.index==UINT32_MAX&&p.generation==0;}
static bool component_valid(const fc_ui *ui){
 size_t i,j,k;
 if(!ui||!ui->entries||!ui->capacity||ui->capacity>=UINT32_MAX)return false;
 for(i=0;i<ui->capacity;i++){
  const fc_component *c=&ui->entries[i];
  if(!c->live)continue;
  if(!c->generation||c->kind<FC_PANEL||c->kind>FC_LABEL||c->state>FC_CLOSED||
     (c->kind!=FC_DIALOG&&c->state>=FC_OPEN)||
     slen(c->id,FC_NAME)==SIZE_MAX||!c->id[0]||
     slen(c->label,FC_TEXT)==SIZE_MAX||slen(c->value,FC_TEXT)==SIZE_MAX||
     !isfinite(c->style.x)||!isfinite(c->style.y)||!isfinite(c->style.w)||
     !isfinite(c->style.h)||!isfinite(c->style.font_size)||
     c->style.w<0||c->style.h<0||c->style.font_size<=0)return false;
  for(j=0;j<i;j++)if(ui->entries[j].live&&strcmp(ui->entries[j].id,c->id)==0)return false;
  k=i;
  for(j=0;j<ui->capacity;j++){
   const fc_component *p=&ui->entries[k];
   if(parent_missing(p->parent))break;
   if(p->parent.index>=ui->capacity)return false;
   k=p->parent.index;
   if(!ui->entries[k].live||ui->entries[k].generation!=p->parent.generation||k==i)return false;
  }
  if(j==ui->capacity)return false;
 }
 return true;
}
fc_result fc_ui_save(const fc_ui *ui,uint8_t *out,size_t cap,size_t *written){
 size_t i;stream s;fc_result r=start_save(&s,out,cap,written);
 if(r!=FC_OK)return r;
 if(!component_valid(ui))return FC_INVALID;
 magic_write(&s,"FCU1");w32(&s,(uint32_t)ui->capacity);
 for(i=0;i<ui->capacity;i++){
  const fc_component *c=&ui->entries[i];
  w32(&s,c->generation);w8(&s,(uint8_t)c->live);
  if(!c->live)continue;
  w8(&s,(uint8_t)c->kind);w8(&s,(uint8_t)c->state);
  w8(&s,(uint8_t)c->enabled);w8(&s,(uint8_t)c->selected);
  w32(&s,c->parent.index);w32(&s,c->parent.generation);
  ws(&s,c->id,FC_NAME);ws(&s,c->label,FC_TEXT);ws(&s,c->value,FC_TEXT);
  wd(&s,c->style.x);wd(&s,c->style.y);wd(&s,c->style.w);wd(&s,c->style.h);
  wd(&s,c->style.font_size);w32(&s,c->style.foreground);w32(&s,c->style.background);
 }
 return finish_save(&s,written);
}
fc_result fc_ui_load(fc_ui *ui,const uint8_t *data,size_t length,fc_component *scratch,size_t count){
 stream s;size_t i;fc_ui staged;fc_result r;
 if(!ui||!ui->entries||!scratch||scratch==ui->entries||count<ui->capacity)return FC_INVALID;
 r=start_load(&s,data,length);if(r!=FC_OK)return r;
 memset(scratch,0,ui->capacity*sizeof(*scratch));
 magic_read(&s,"FCU1");if(r32(&s)!=ui->capacity)return FC_INVALID;
 for(i=0;i<ui->capacity&&s.err==FC_OK;i++){
  fc_component *c=&scratch[i];
  c->generation=r32(&s);uint8_t live=r8(&s);
  if(live>1){s.err=FC_INVALID;break;}
  c->live=live!=0;if(!c->live)continue;
  c->kind=(fc_kind)r8(&s);c->state=(fc_state)r8(&s);
  uint8_t enabled=r8(&s),selected=r8(&s);
  if(enabled>1||selected>1){s.err=FC_INVALID;break;}
  c->enabled=enabled!=0;c->selected=selected!=0;
  c->parent.index=r32(&s);c->parent.generation=r32(&s);
  rs(&s,c->id,FC_NAME);rs(&s,c->label,FC_TEXT);rs(&s,c->value,FC_TEXT);
  c->style.x=rd(&s);c->style.y=rd(&s);c->style.w=rd(&s);
  c->style.h=rd(&s);c->style.font_size=rd(&s);
  c->style.foreground=r32(&s);c->style.background=r32(&s);
 }
 r=finish_load(&s);if(r!=FC_OK)return r;
 staged=*ui;staged.entries=scratch;
 if(!component_valid(&staged))return FC_INVALID;
 memcpy(ui->entries,scratch,ui->capacity*sizeof(*scratch));return FC_OK;
}
fc_result fc_graph_save(const fc_graph *g,uint8_t *out,size_t cap,size_t *written){
 stream s;size_t i,j;fc_result r=start_save(&s,out,cap,written);
 if(r!=FC_OK)return r;
 if(!g||!g->nodes||!g->edges||g->node_count>g->node_cap||g->edge_count>g->edge_cap||
 g->node_count>UINT32_MAX||g->edge_count>UINT32_MAX||!isfinite(g->zoom)||
 g->zoom<=0||!isfinite(g->pan_x)||!isfinite(g->pan_y)||!isfinite(g->min_zoom)||!isfinite(g->max_zoom)||
 g->min_zoom<=0||g->max_zoom<g->min_zoom)return FC_INVALID;
 r=fc_graph_validate(g);if(r!=FC_OK)return r;
 magic_write(&s,"FCG1");w32(&s,(uint32_t)g->node_count);w32(&s,(uint32_t)g->edge_count);
 wd(&s,g->pan_x);wd(&s,g->pan_y);wd(&s,g->zoom);wd(&s,g->min_zoom);wd(&s,g->max_zoom);
 for(i=0;i<g->node_count;i++){
  const fc_node *n=&g->nodes[i];
  if(!n->id||n->ports_count>FC_MAX_PORTS||n->width<=0||n->height<=0)return FC_INVALID;
  w32(&s,n->id);ws(&s,n->label,FC_TEXT);
  wd(&s,n->x);wd(&s,n->y);wd(&s,n->width);wd(&s,n->height);w8(&s,n->selected?1:0);
  w8(&s,(uint8_t)n->ports_count);
  for(j=0;j<n->ports_count;j++){
   ws(&s,n->ports[j].id,FC_NAME);w8(&s,(uint8_t)n->ports[j].direction);
   w8(&s,(uint8_t)n->ports[j].signal);
  }
 }
 for(i=0;i<g->edge_count;i++){
  const fc_edge *e=&g->edges[i];w32(&s,e->source);w32(&s,e->target);
  w8(&s,e->output);w8(&s,e->input);
 }
 return finish_save(&s,written);
}
fc_result fc_graph_load(fc_graph *g,const uint8_t *data,size_t length,
 fc_node *ns,fc_edge *es,uint8_t *walk){
 stream s;size_t i,j,n,e;fc_graph tmp;fc_result r;
 if(!g||!g->nodes||!g->edges||!g->scratch||!ns||!es||!walk||
 ns==g->nodes||es==g->edges||walk==g->scratch)return FC_INVALID;
 r=start_load(&s,data,length);if(r!=FC_OK)return r;
 magic_read(&s,"FCG1");n=r32(&s);e=r32(&s);
 if(s.err!=FC_OK)return FC_INVALID;
 if(n>g->node_cap||e>g->edge_cap)return FC_CAPACITY;
 r=fc_graph_init(&tmp,ns,g->node_cap,es,g->edge_cap,walk);if(r!=FC_OK)return r;
 tmp.pan_x=rd(&s);tmp.pan_y=rd(&s);tmp.zoom=rd(&s);
 tmp.min_zoom=rd(&s);tmp.max_zoom=rd(&s);
 if(!isfinite(tmp.pan_x)||!isfinite(tmp.pan_y)||tmp.min_zoom<=0||
 tmp.max_zoom<tmp.min_zoom||tmp.zoom<tmp.min_zoom||tmp.zoom>tmp.max_zoom)return FC_INVALID;
 for(i=0;i<n&&s.err==FC_OK;i++){
  uint32_t id=r32(&s);char name[FC_TEXT];rs(&s,name,FC_TEXT);
  double x=rd(&s),y=rd(&s),w=rd(&s),h=rd(&s);
  uint8_t selected=r8(&s),pc=r8(&s);
  if(s.err!=FC_OK||selected>1||pc>FC_MAX_PORTS||w<=0||h<=0)return FC_INVALID;
  if((r=fc_node_add(&tmp,id,name,x,y))!=FC_OK)return FC_INVALID;
  tmp.nodes[tmp.node_count-1].width=w;tmp.nodes[tmp.node_count-1].height=h;
  tmp.nodes[tmp.node_count-1].selected=selected!=0;
  for(j=0;j<pc;j++){
   char port[FC_NAME];rs(&s,port,FC_NAME);
   fc_port_direction d=(fc_port_direction)r8(&s);fc_signal sig=(fc_signal)r8(&s);
   if(s.err!=FC_OK||(r=fc_port_add(&tmp,id,port,d,sig))!=FC_OK)return FC_INVALID;
  }
 }
 for(i=0;i<e&&s.err==FC_OK;i++){
  uint32_t from=r32(&s),to=r32(&s);uint8_t a=r8(&s),b=r8(&s);
  if(s.err!=FC_OK||(r=fc_edge_add(&tmp,from,a,to,b))!=FC_OK)return FC_INVALID;
 }
 if((r=finish_load(&s))!=FC_OK)return r;
 memcpy(g->nodes,ns,g->node_cap*sizeof(*ns));
 memcpy(g->edges,es,g->edge_cap*sizeof(*es));
 g->node_count=tmp.node_count;g->edge_count=tmp.edge_count;
 g->pan_x=tmp.pan_x;g->pan_y=tmp.pan_y;g->zoom=tmp.zoom;
 g->min_zoom=tmp.min_zoom;g->max_zoom=tmp.max_zoom;return FC_OK;
}
fc_result fc_timeline_save(const fc_timeline *t,uint8_t *out,size_t cap,size_t *written){
 size_t i;stream s;fc_result r=start_save(&s,out,cap,written);
 if(r!=FC_OK)return r;
 if(!t||!t->notes||t->count>t->capacity||t->count>UINT32_MAX||
 !isfinite(t->length)||t->length<=0||!isfinite(t->loop_start)||
 !isfinite(t->loop_end)||!isfinite(t->playhead)||t->loop_start<0||
 t->loop_end>t->length||t->loop_start>=t->loop_end||
 t->playhead<0||t->playhead>t->length)return FC_INVALID;
 magic_write(&s,"FCT1");w32(&s,(uint32_t)t->count);
 wd(&s,t->length);wd(&s,t->loop_start);wd(&s,t->loop_end);wd(&s,t->playhead);w64(&s,t->next_id);
 for(i=0;i<t->count;i++){
  const fc_note *n=&t->notes[i];
  if(!n->id||n->id>=t->next_id||n->pitch>127||!isfinite(n->start)||
  !isfinite(n->length)||n->start<0||n->start>=t->length||
  n->length<=0||n->length>t->length-n->start)return FC_INVALID;
  size_t j;for(j=0;j<i;j++)if(t->notes[j].id==n->id)return FC_DUPLICATE;
  w64(&s,n->id);w8(&s,n->pitch);wd(&s,n->start);wd(&s,n->length);w8(&s,n->selected?1:0);
 }
 return finish_save(&s,written);
}
fc_result fc_timeline_load(fc_timeline *t,const uint8_t *data,size_t length,fc_note *scratch,size_t scratch_count){
 stream s;size_t i,j,n;fc_result r;double beats,lo,hi,head;uint64_t next;
 if(!t||!t->notes||!scratch||scratch==t->notes||scratch_count<t->capacity)return FC_INVALID;
 r=start_load(&s,data,length);if(r!=FC_OK)return r;
 magic_read(&s,"FCT1");n=r32(&s);if(s.err!=FC_OK)return FC_INVALID;
 if(n>t->capacity)return FC_CAPACITY;
 beats=rd(&s);lo=rd(&s);hi=rd(&s);head=rd(&s);next=r64(&s);
 if(s.err!=FC_OK||!isfinite(beats)||beats<=0||lo<0||lo>=hi||hi>beats||
 head<0||head>beats||!next)return FC_INVALID;
 memset(scratch,0,t->capacity*sizeof(*scratch));
 for(i=0;i<n&&s.err==FC_OK;i++){
  fc_note *item=&scratch[i];item->id=r64(&s);item->pitch=r8(&s);
  item->start=rd(&s);item->length=rd(&s);uint8_t selected=r8(&s);
  item->selected=selected!=0;
  if(s.err!=FC_OK||selected>1||!item->id||item->id>=next||item->pitch>127||
  item->start<0||item->start>=beats||item->length<=0||
  item->length>beats-item->start)return FC_INVALID;
  for(j=0;j<i;j++)if(scratch[j].id==item->id)return FC_INVALID;
 }
 if((r=finish_load(&s))!=FC_OK)return r;
 memcpy(t->notes,scratch,t->capacity*sizeof(*scratch));
 t->count=n;t->next_id=next;t->length=beats;t->loop_start=lo;
 t->loop_end=hi;t->playhead=head;return FC_OK;
}
