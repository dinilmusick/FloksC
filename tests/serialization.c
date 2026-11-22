#include "floksc/serialization.h"
#include <assert.h>
#include <string.h>
#include <stdint.h>
int main(void){
 uint8_t buf[2048];size_t len=0;fc_component mem[3],scratch[3];fc_ui ui;
 fc_handle root,child;
 assert(fc_init(&ui,mem,3)==FC_OK);
 assert(fc_add(&ui,FC_PANEL,"root",fc_invalid(),&root)==FC_OK);
 assert(fc_add(&ui,FC_INPUT,"child",root,&child)==FC_OK);
 assert(fc_value_set(&ui,child,"alpha")==FC_OK);
 assert(fc_ui_save(&ui,NULL,0,&len)==FC_OK&&len>0);
 size_t required=len;
 assert(fc_ui_save(&ui,buf,len-1,&len)==FC_CAPACITY&&len==required);
 assert(fc_ui_save(&ui,buf,sizeof(buf),&len)==FC_OK);
 assert(fc_destroy(&ui,root)==FC_OK);
 assert(fc_ui_load(&ui,buf,len,scratch,3)==FC_OK);
 assert(fc_get(&ui,child)!=NULL&&strcmp(fc_get(&ui,child)->value,"alpha")==0);
 uint8_t prev=buf[0];buf[0]='X';
 assert(fc_ui_load(&ui,buf,len,scratch,3)==FC_INVALID&&fc_get(&ui,child)!=NULL);
 buf[0]=prev;
 assert(fc_ui_load(&ui,buf,len-1,scratch,3)==FC_INVALID);
 assert(fc_ui_load(&ui,buf,len,scratch,2)==FC_INVALID);
 fc_node nodes[3],nodes2[3];fc_edge edges[3],edges2[3];
 uint8_t visited[3],visited2[3];fc_graph g;
 assert(fc_graph_init(&g,nodes,3,edges,3,visited)==FC_OK);
 assert(fc_node_add(&g,1,"n1",0,0)==FC_OK);
 assert(fc_node_add(&g,2,"n2",10,0)==FC_OK);
 assert(fc_port_add(&g,1,"out",FC_PORT_OUT,FC_SIGNAL_MIDI)==FC_OK);
 assert(fc_port_add(&g,2,"in",FC_PORT_IN,FC_SIGNAL_MIDI)==FC_OK);
 assert(fc_edge_add(&g,1,0,2,0)==FC_OK);
 assert(fc_graph_save(&g,buf,sizeof(buf),&len)==FC_OK);
 assert(fc_node_delete(&g,1)==FC_OK);
 assert(fc_graph_load(&g,buf,len,nodes2,edges2,visited2)==FC_OK);
 assert(g.node_count==2&&g.edge_count==1&&g.nodes[0].id==1);
 buf[0]='X';
 assert(fc_graph_load(&g,buf,len,nodes2,edges2,visited2)==FC_INVALID&&g.edge_count==1);
 fc_note notes[3],note2[3];fc_timeline t;uint64_t id;
 assert(fc_timeline_init(&t,notes,3,16)==FC_OK);
 assert(fc_note_add(&t,60,1,2,&id)==FC_OK);
 assert(fc_loop_set(&t,2,10)==FC_OK);
 assert(fc_seek(&t,5)==FC_OK);
 assert(fc_timeline_save(&t,buf,sizeof(buf),&len)==FC_OK);
 assert(fc_note_delete(&t,id)==FC_OK);
 assert(fc_timeline_load(&t,buf,len,note2,3)==FC_OK);
 assert(t.count==1&&t.notes[0].id==id&&t.loop_start==2&&t.playhead==5);
 buf[len-1]=255;
 assert(fc_timeline_load(&t,buf,len,note2,3)==FC_INVALID&&t.count==1);
 return 0;
}
