#include "floksc/serialization.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
int main(void){
 fc_node nodes[3];fc_edge edges[4];uint8_t scratch[3];fc_graph g;size_t len=0;
 uint8_t buffer[2048];
 assert(fc_graph_init(&g,nodes,3,edges,4,scratch)==FC_OK);
 assert(fc_node_add(&g,1,"one",0,0)==FC_OK);
 assert(fc_node_add(&g,2,"two",1,1)==FC_OK);
 assert(fc_node_add(&g,3,"three",2,2)==FC_OK);
 assert(fc_port_add(&g,1,"out",FC_PORT_OUT,FC_SIGNAL_MIDI)==FC_OK);
 assert(fc_port_add(&g,1,"in",FC_PORT_IN,FC_SIGNAL_MIDI)==FC_OK);
 assert(fc_port_add(&g,2,"in",FC_PORT_IN,FC_SIGNAL_MIDI)==FC_OK);
 assert(fc_port_add(&g,2,"out",FC_PORT_OUT,FC_SIGNAL_MIDI)==FC_OK);
 assert(fc_port_add(&g,3,"in",FC_PORT_IN,FC_SIGNAL_MIDI)==FC_OK);
 assert(fc_port_add(&g,3,"out",FC_PORT_OUT,FC_SIGNAL_MIDI)==FC_OK);
 assert(fc_edge_add(&g,1,0,2,0)==FC_OK);
 assert(fc_edge_add(&g,2,1,3,0)==FC_OK);
 assert(fc_graph_validate(&g)==FC_OK);
 assert(fc_graph_save(&g,buffer,sizeof(buffer),&len)==FC_OK);
 g.edges[2]=(fc_edge){3,1,1,1};g.edge_count=3;
 assert(fc_graph_validate(&g)==FC_CYCLE);
 assert(fc_graph_save(&g,buffer,sizeof(buffer),&len)==FC_CYCLE);
 g.edge_count=2;
 nodes[1].id=1;
 assert(fc_graph_validate(&g)==FC_DUPLICATE);
 nodes[1].id=2;
 assert(fc_graph_validate(&g)==FC_OK);
 g.edges[0].output=255;
 assert(fc_graph_validate(&g)==FC_INVALID);
 g.edges[0].output=0;
 g.zoom=NAN;
 assert(fc_graph_validate(&g)==FC_INVALID);
 assert(fc_zoom_at(&g,1,1,1)==FC_INVALID);
 g.zoom=1;
 assert(fc_graph_validate(&g)==FC_OK);
 fc_note notes[2];fc_timeline t;uint64_t a,b;
 assert(fc_timeline_init(&t,notes,2,16)==FC_OK);
 assert(fc_note_add(&t,60,1,1,&a)==FC_OK);
 assert(fc_note_add(&t,61,2,1,&b)==FC_OK);
 assert(fc_timeline_save(&t,buffer,sizeof(buffer),&len)==FC_OK);
 t.notes[1].id=a;
 assert(fc_timeline_save(&t,buffer,sizeof(buffer),&len)==FC_DUPLICATE);
 t.notes[1].id=b;
 t.length=NAN;
 assert(fc_loop_set(&t,0,1)==FC_INVALID);
 assert(fc_seek(&t,1)==FC_INVALID);
 t.length=16;
 assert(fc_loop_set(&t,0,8)==FC_OK);
 assert(fc_seek(&t,2)==FC_OK);
 puts("adversarial graph/timeline fixtures passed");
 return 0;
}
