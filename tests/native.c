#include "floksc/floksc.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static fc_result render(void *data,fc_handle h,const fc_component *c){
 int *count=(int *)data;(void)h;assert(c->live);(*count)++;return FC_OK;
}
static fc_result activate(void *data,fc_handle h,const char *event){
 int *count=(int *)data;(void)h;assert(strcmp(event,"click")==0);(*count)++;return FC_OK;
}
int main(void){
 fc_component memory[5];fc_ui ui;fc_handle a,b,c,tab1,tab2,tmp;
 fc_graph graph;fc_node nodes[3];fc_edge edges[3];uint8_t scratch[3];
 fc_note notes[2];fc_timeline timeline;uint64_t note_id,note2;double x,y;
 int count=0,actions=0,pitch=0;
 assert(fc_init(&ui,memory,5)==FC_OK);
 assert(fc_add(&ui,FC_PANEL,"root",fc_invalid(),&a)==FC_OK);
 assert(fc_add(&ui,FC_BUTTON,"child",a,&b)==FC_OK);
 assert(fc_add(&ui,FC_INPUT,"grand",b,&c)==FC_OK);
 assert(fc_reparent(&ui,a,c)==FC_CYCLE);
 assert(fc_add(&ui,FC_PANEL,"root",fc_invalid(),&tmp)==FC_DUPLICATE);
 assert(fc_value_set(&ui,c,"hi")==FC_OK);
 assert(fc_value_set(&ui,a,"bad")==FC_MISMATCH);
 fc_renderer_bind(&ui,render,&count);
 assert(fc_render(&ui)==FC_OK&&count==3);
 assert(fc_destroy(&ui,b)==FC_OK);
 assert(fc_get(&ui,c)==NULL&&fc_get(&ui,b)==NULL);
 assert(fc_add(&ui,FC_TAB,"one",a,&tab1)==FC_OK);
 assert(fc_add(&ui,FC_TAB,"two",a,&tab2)==FC_OK);
 assert(fc_select(&ui,tab1,true)==FC_OK);
 assert(fc_select(&ui,tab2,true)==FC_OK);
 assert(!fc_get(&ui,tab1)->selected&&fc_get(&ui,tab2)->selected);
 assert(fc_add(&ui,FC_BUTTON,"button",a,&b)==FC_OK);
 fc_event_bind(&ui,activate,&actions);
 assert(fc_activate(&ui,b,"click")==FC_OK&&actions==1);
 assert(fc_enable(&ui,b,false)==FC_OK);
 assert(fc_activate(&ui,b,"click")==FC_DISABLED);
 assert(fc_state_set(&ui,b,FC_OPEN)==FC_MISMATCH);
 assert(fc_style_set(&ui,b,(fc_style){0,0,-1,1,12,0,0})==FC_INVALID);
 assert(fc_destroy(&ui,a)==FC_OK&&fc_get(&ui,b)==NULL);
 assert(fc_graph_init(&graph,nodes,3,edges,3,scratch)==FC_OK);
 assert(fc_node_add(&graph,1,"A",0,0)==FC_OK);
 assert(fc_node_add(&graph,2,"B",5,0)==FC_OK);
 assert(fc_node_add(&graph,3,"C",10,0)==FC_OK);
 assert(fc_port_add(&graph,1,"out",FC_PORT_OUT,FC_SIGNAL_MIDI)==FC_OK);
 assert(fc_port_add(&graph,2,"in",FC_PORT_IN,FC_SIGNAL_MIDI)==FC_OK);
 assert(fc_port_add(&graph,2,"out",FC_PORT_OUT,FC_SIGNAL_MIDI)==FC_OK);
 assert(fc_port_add(&graph,3,"in",FC_PORT_IN,FC_SIGNAL_MIDI)==FC_OK);
 assert(fc_port_add(&graph,3,"out",FC_PORT_OUT,FC_SIGNAL_MIDI)==FC_OK);
 assert(fc_port_add(&graph,1,"in",FC_PORT_IN,FC_SIGNAL_MIDI)==FC_OK);
 assert(fc_edge_add(&graph,1,0,2,0)==FC_OK);
 assert(fc_edge_add(&graph,2,1,3,0)==FC_OK);
 assert(fc_edge_add(&graph,3,1,1,1)==FC_CYCLE);
 assert(fc_edge_add(&graph,1,0,2,0)==FC_DUPLICATE);
 assert(fc_edge_add(&graph,1,1,2,0)==FC_MISMATCH);
 assert(fc_zoom_at(&graph,100,100,2)==FC_OK);
 assert(fc_screen(&graph,5,5,&x,&y)==FC_OK);
 assert(fc_canvas(&graph,x,y,&x,&y)==FC_OK&&fabs(x-5)<1e-8&&fabs(y-5)<1e-8);
 assert(fc_node_delete(&graph,2)==FC_OK&&graph.edge_count==0);
 assert(fc_timeline_init(&timeline,notes,2,16)==FC_OK);
 assert(fc_note_add(&timeline,60,1,2,&note_id)==FC_OK);
 assert(fc_note_add(&timeline,127,3,1,&note2)==FC_OK);
 assert(fc_note_add(&timeline,128,1,1,&note2)==FC_INVALID);
 assert(fc_note_move(&timeline,note_id,61,2)==FC_OK);
 assert(fc_loop_set(&timeline,4,12)==FC_OK);
 assert(fc_seek(&timeline,8)==FC_OK);
 assert(fc_loop_set(&timeline,12,4)==FC_INVALID);
 assert(fc_y_pitch(0,768,&pitch)==FC_OK&&pitch==127);
 assert(fc_pitch_y(127,768,&y)==FC_OK&&y==0);
 assert(fc_x_beat(50,100,16,&x)==FC_OK&&x==8);
 assert(fc_beat_x(8,100,16,&x)==FC_OK&&x==50);
 assert(fc_note_delete(&timeline,note_id)==FC_OK);
 assert(fc_note_delete(&timeline,note_id)==FC_NOT_FOUND);
 return 0;
}
