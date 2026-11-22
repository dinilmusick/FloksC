#include "floksc/floksc.h"
#include <math.h>
#include <string.h>
static bool valid_number(double n){return isfinite(n)!=0;}
static bool valid_string(const char *s,size_t cap,bool empty){
 size_t i;
 for(i=0;i<cap;i++)if(s[i]==0)return empty || i>0;
 return false;
}
static size_t lookup(const fc_graph *g,uint32_t id){
 size_t i;
 for(i=0;i<g->node_count;i++)if(g->nodes[i].id==id)return i;
 return g->node_count;
}
static bool compatible(const fc_port *a,const fc_port *b){
 return a->direction==FC_PORT_OUT && b->direction==FC_PORT_IN &&
 (a->signal==FC_SIGNAL_ANY || b->signal==FC_SIGNAL_ANY || a->signal==b->signal);
}
fc_result fc_graph_validate(const fc_graph *g){
 size_t i,j,k,start,round;
 if(!g || !g->nodes || !g->edges || !g->scratch ||
 !g->node_cap || !g->edge_cap || g->node_count>g->node_cap ||
 g->edge_count>g->edge_cap || !valid_number(g->zoom) ||
 !valid_number(g->min_zoom) || !valid_number(g->max_zoom) ||
 !valid_number(g->pan_x) || !valid_number(g->pan_y) ||
 g->min_zoom<=0 || g->max_zoom<g->min_zoom ||
 g->zoom<g->min_zoom || g->zoom>g->max_zoom)return FC_INVALID;
 for(i=0;i<g->node_count;i++){
  const fc_node *n=&g->nodes[i];
  if(!n->id || n->ports_count>FC_MAX_PORTS || !valid_string(n->label,FC_TEXT,false) ||
   !valid_number(n->x)||!valid_number(n->y)||
   !valid_number(n->width)||!valid_number(n->height)||
   n->width<=0||n->height<=0)return FC_INVALID;
  for(j=0;j<i;j++)if(g->nodes[j].id==n->id)return FC_DUPLICATE;
  for(j=0;j<n->ports_count;j++){
   const fc_port *p=&n->ports[j];
   if(!valid_string(p->id,FC_NAME,false) ||
    (p->direction!=FC_PORT_IN && p->direction!=FC_PORT_OUT) ||
    p->signal<FC_SIGNAL_ANY || p->signal>FC_SIGNAL_AUDIO)return FC_INVALID;
   for(k=0;k<j;k++)if(strcmp(n->ports[k].id,p->id)==0)return FC_DUPLICATE;
  }
 }
 for(i=0;i<g->edge_count;i++){
  const fc_edge *e=&g->edges[i];
  size_t s=lookup(g,e->source),t=lookup(g,e->target);
  if(s==g->node_count||t==g->node_count)return FC_NOT_FOUND;
  if(s==t)return FC_CYCLE;
  if(e->output>=g->nodes[s].ports_count || e->input>=g->nodes[t].ports_count)
   return FC_INVALID;
  if(!compatible(&g->nodes[s].ports[e->output],&g->nodes[t].ports[e->input]))
   return FC_MISMATCH;
  for(j=0;j<i;j++){
   const fc_edge *prev=&g->edges[j];
   if(prev->source==e->source && prev->target==e->target &&
    prev->output==e->output && prev->input==e->input)return FC_DUPLICATE;
  }
 }
 /* Iterate reachability in caller-owned scratch, never recurse or allocate. */
 for(start=0;start<g->node_count;start++){
  memset(g->scratch,0,g->node_cap);
  g->scratch[start]=1;
  for(round=0;round<g->node_count;round++){
   bool changed=false;
   for(i=0;i<g->edge_count;i++){
    size_t s=lookup(g,g->edges[i].source),t=lookup(g,g->edges[i].target);
    if(g->scratch[s]){
     if(t==start)return FC_CYCLE;
     if(!g->scratch[t]){g->scratch[t]=1;changed=true;}
    }
   }
   if(!changed)break;
  }
 }
 return FC_OK;
}
