#include "floksc/authoring.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static fc_author_value str(const char *s){
 fc_author_value v={0};
 v.kind=FC_AUTHOR_STRING;
 assert(strlen(s)<FC_TEXT);
 memcpy(v.text,s,strlen(s)+1);
 return v;
}
static fc_author_value number(double x){
 fc_author_value v={0};v.kind=FC_AUTHOR_NUMBER;v.number=x;return v;
}
int main(void){
 fc_author_session s;
 fc_author_node nodes[5];
 fc_author_field fields[14];
 fc_author_operation ops[2];
 uint8_t scratch[5];
 fc_author_asset asset={0};
 fc_author_operation op={0};
 const fc_author_field *f;
 assert(fc_author_init(&s,"session1",FC_AUTHOR_APPLICATION,nodes,5,fields,14,ops,2,scratch)==FC_OK);
 assert(fc_author_node_add(&s,"page",FC_AUTHOR_PAGE,"",NULL,NULL)==FC_OK);
 assert(fc_author_node_add(&s,"section",FC_AUTHOR_SECTION,"page","Header",NULL)==FC_OK);
 assert(fc_author_node_add(&s,"component",FC_AUTHOR_COMPONENT,"section",NULL,"floksButton")==FC_OK);
 assert(fc_author_node_add(&s,"other",FC_AUTHOR_COMPONENT,"page","Card","floksTile")==FC_OK);
 assert(fc_author_node_add(&s,"component",FC_AUTHOR_COMPONENT,"page",NULL,NULL)==FC_DUPLICATE);
 assert(fc_author_node_add(&s,"orphan",FC_AUTHOR_COMPONENT,"missing",NULL,NULL)==FC_NOT_FOUND);
 assert(fc_author_node_update(&s,"page","component",NULL)==FC_CYCLE);
 assert(fc_author_node_update(&s,"section","section",NULL)==FC_CYCLE);
 assert(fc_author_node_update(&s,"missing",NULL,"Missing")==FC_NOT_FOUND);
 assert(fc_author_node_update(&s,"section",NULL,"Toolbar")==FC_OK);
 assert(strcmp(fc_author_find(&s,"section")->name,"Toolbar")==0);
 assert(fc_author_property(&s,"component","size",number(12))==FC_OK);
 assert(fc_author_property(&s,"component","size",number(15))==FC_OK);
 f=fc_author_field_find(&s,FC_AUTHOR_PROPERTY,"component","size","");
 assert(f&&f->value.number==15);
 assert(fc_author_property(&s,"component","bad",number(NAN))==FC_INVALID);
 assert(fc_author_state_property(&s,"component","hover","opacity",number(.7))==FC_OK);
 assert(fc_author_state_property(&s,"component","","opacity",number(.7))==FC_INVALID);
 assert(fc_author_event(&s,"component","click","onClick")==FC_OK);
 assert(fc_author_bind(&s,"component","label","query","api1","title","string")==FC_OK);
 assert(fc_author_bind(&s,"component","label","query","api2","title","string")==FC_OK);
 f=fc_author_field_find(&s,FC_AUTHOR_BINDING,"component","label","");
 assert(f && strcmp(f->target_api,"api2")==0);
 assert(fc_author_metadata(&s,"owner","author")==FC_OK);
 assert(fc_author_slot(&s,"section","default","component")==FC_OK);
 assert(fc_author_slot(&s,"page","default","component")==FC_INVALID);
 assert(fc_author_slot(&s,"section","default","component")==FC_OK);
 assert(fc_author_unslot(&s,"section","default","component")==FC_OK);
 assert(fc_author_unslot(&s,"section","default","component")==FC_NOT_FOUND);
 assert(fc_author_slot(&s,"section","default","component")==FC_OK);
 assert(fc_author_validate(&s)==FC_OK);
 assert(fc_author_node_update(&s,"component","page",NULL)==FC_OK);
 assert(fc_author_field_find(&s,FC_AUTHOR_SLOT,"section","default","component")==NULL);
 assert(fc_author_slot(&s,"page","default","component")==FC_OK);
 strcpy(asset.package_id,"floks");
 strcpy(asset.package_version,"2.0.1");
 strcpy(asset.export_path,"ui/button");
 strcpy(asset.asset_id,"button");
 strcpy(asset.asset_version,"1");
 assert(fc_author_asset_set(&s,"component",asset)==FC_OK);
 strcpy(op.id,"save");strcpy(op.name,"Save");
 strcpy(op.inputs[0].name,"text");strcpy(op.inputs[0].type,"string");
 op.input_count=1;
 strcpy(op.outputs[0].name,"ok");strcpy(op.outputs[0].type,"boolean");
 op.output_count=1;
 assert(fc_author_operation_set(&s,op)==FC_OK);
 assert(fc_author_operation_set(&s,op)==FC_OK);
 assert(s.operation_count==1);
 assert(fc_author_validate(&s)==FC_OK);
 assert(fc_author_unbind(&s,"component","label")==FC_OK);
 assert(fc_author_unbind(&s,"component","label")==FC_NOT_FOUND);
 assert(fc_author_node_remove(&s,"section")==FC_OK);
 assert(fc_author_find(&s,"section")==NULL);
 assert(fc_author_find(&s,"component")!=NULL);
 assert(fc_author_validate(&s)==FC_OK);
 assert(fc_author_node_remove(&s,"page")==FC_OK);
 assert(s.node_count==0);
 assert(s.field_count==1); /* metadata survives subtree deletion */
 assert(fc_author_validate(&s)==FC_OK);
 assert(fc_author_operation_remove(&s,"save")==FC_OK&&s.operation_count==0);
 assert(fc_author_operation_remove(&s,"save")==FC_NOT_FOUND);
 assert(fc_author_node_add(&s,"root",FC_AUTHOR_PAGE,"",NULL,NULL)==FC_OK);
 assert(fc_author_property(&s,"root","a",str("hello"))==FC_OK);
 assert(fc_author_property(&s,"root","b",str("hello"))==FC_OK);
 assert(fc_author_validate(&s)==FC_OK);
 /* Tampering with the host-owned buffer is detected. */
 strcpy(s.nodes[0].parent_id,"root");
 assert(fc_author_validate(&s)==FC_CYCLE);
 s.nodes[0].parent_id[0]=0;
 assert(fc_author_validate(&s)==FC_OK);
 {
  fc_author_session tiny;fc_author_node ns[1];fc_author_field fs[1];fc_author_operation os[1];uint8_t ss[1];
  assert(fc_author_init(&tiny,"tiny",FC_AUTHOR_SERVICE,ns,1,fs,1,os,1,ss)==FC_OK);
  assert(fc_author_node_add(&tiny,"a",FC_AUTHOR_PAGE,"",NULL,NULL)==FC_OK);
  assert(fc_author_node_add(&tiny,"b",FC_AUTHOR_PAGE,"",NULL,NULL)==FC_CAPACITY);
  assert(fc_author_property(&tiny,"a","x",str("1"))==FC_OK);
  assert(fc_author_property(&tiny,"a","y",str("2"))==FC_CAPACITY);
  assert(fc_author_property(&tiny,"a","x",str("3"))==FC_OK);
  assert(fc_author_validate(&tiny)==FC_OK);
 }
 puts("authoring fixtures passed");
 return 0;
}
