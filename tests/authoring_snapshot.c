#include "floksc/authoring.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static fc_author_value str(const char *s){
 fc_author_value v={0};v.kind=FC_AUTHOR_STRING;
 memcpy(v.text,s,strlen(s)+1);return v;
}
int main(void){
 fc_author_node nodes[4],scratch_nodes[4];
 fc_author_field fields[5],scratch_fields[5];
 fc_author_operation ops[2],scratch_ops[2];
 uint8_t scratch[4],scratch_walk[4],bytes[2048];
 fc_author_session s;size_t needed=0,size=0;fc_author_operation op={0};
 assert(fc_author_init(&s,"S",FC_AUTHOR_APPLICATION,nodes,4,fields,5,ops,2,scratch)==FC_OK);
 assert(fc_author_node_add(&s,"page",FC_AUTHOR_PAGE,"",NULL,NULL)==FC_OK);
 assert(fc_author_node_add(&s,"button",FC_AUTHOR_COMPONENT,"page","Submit","floksButton")==FC_OK);
 assert(fc_author_property(&s,"button","label",str("save"))==FC_OK);
 assert(fc_author_state_property(&s,"button","active","enabled",str("true"))==FC_OK);
 assert(fc_author_event(&s,"button","click","onSave")==FC_OK);
 assert(fc_author_slot(&s,"page","main","button")==FC_OK);
 assert(fc_author_bind(&s,"button","text","service","api","title","string")==FC_OK);
 strcpy(op.id,"save");strcpy(op.name,"Save");
 assert(fc_author_operation_set(&s,op)==FC_OK);
 assert(fc_author_save(&s,NULL,0,&needed)==FC_OK&&needed>20&&needed<sizeof(bytes));
 size=needed;
 assert(fc_author_save(&s,bytes,size-1,&needed)==FC_CAPACITY&&needed==size);
 assert(fc_author_save(&s,bytes,size,&needed)==FC_OK&&needed==size);
 uint8_t previous=bytes[0];bytes[0]='X';
 assert(fc_author_load(&s,bytes,size,scratch_nodes,scratch_fields,scratch_ops,scratch_walk)==FC_INVALID);
 assert(s.node_count==2&&s.field_count==5);
 bytes[0]=previous;
 assert(fc_author_load(&s,bytes,size-1,scratch_nodes,scratch_fields,scratch_ops,scratch_walk)==FC_INVALID);
 assert(s.node_count==2&&s.field_count==5);
 assert(fc_author_node_remove(&s,"page")==FC_OK&&s.node_count==0);
 assert(fc_author_load(&s,bytes,size,scratch_nodes,scratch_fields,scratch_ops,scratch_walk)==FC_OK);
 assert(s.node_count==2&&s.field_count==5&&s.operation_count==1);
 assert(fc_author_validate(&s)==FC_OK);
 assert(fc_author_find(&s,"button")!=NULL);
 assert(strcmp(fc_author_field_find(&s,FC_AUTHOR_BINDING,"button","text","")->target_api,"api")==0);
 size_t roundtrip=0;
 assert(fc_author_save(&s,NULL,0,&roundtrip)==FC_OK&&roundtrip==size);
 uint8_t copy[2048];
 assert(fc_author_save(&s,copy,sizeof(copy),&roundtrip)==FC_OK);
 assert(memcmp(copy,bytes,size)==0);
 bytes[size-1]=255;
 assert(fc_author_load(&s,bytes,size,scratch_nodes,scratch_fields,scratch_ops,scratch_walk)!=FC_OK);
 assert(s.node_count==2&&s.operation_count==1);
 puts("authoring snapshot fixtures passed");
 return 0;
}
