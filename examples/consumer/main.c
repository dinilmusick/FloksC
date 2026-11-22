#include <floksc/floksc.h>
#include <floksc/authoring.h>
#include <assert.h>
#include <string.h>
static frnl_status on_click(void *ctx,const void *payload){
 int *count=(int *)ctx;(void)payload;(*count)++;return FRNL_OK;
}
int main(void) {
 fc_component storage[2];fc_ui ui;fc_handle button;
 frnl_component components[2];frnl_state_spec states[1];
 frnl_event_def events[1];frnl_function functions[1];int clicks=0;
 frnl_storage memory={0};frnl_runtime rt;
 memory.components=components;memory.component_capacity=2;
 memory.states=states;memory.state_capacity=1;memory.max_depth=2;
 memory.events=events;memory.event_capacity=1;
 memory.functions=functions;memory.function_capacity=1;
 assert(frnl_init(&rt,&memory)==FRNL_OK);
 assert(frnl_define_state(&rt,(frnl_state_spec){"button","idle",0,0})==FRNL_OK);
 assert(fc_init(&ui,storage,2)==FC_OK);
 assert(fc_add(&ui,FC_BUTTON,"myButton",fc_invalid(),&button)==FC_OK);
 assert(fc_frnl_register(&rt,&ui,button,"idle")==FC_OK);
 assert(rt.components==1);
 assert(frnl_event_define(&rt,(frnl_event_def){"myButton","idle","click","handler"})==FRNL_OK);
 assert(frnl_function_add(&rt,(frnl_function){"myButton","handler",on_click,&clicks})==FRNL_OK);
 assert(fc_frnl_event(&rt,&ui,button,"click",NULL)==FC_OK&&clicks==1);
 assert(fc_frnl_event(&rt,&ui,button,"unknown",NULL)==FC_FAILED);
 assert(fc_activate(&ui,button,"click")==FC_OK);
 frnl_cleanup(&rt);
 /* Verify newly installed authoring symbols and snapshot roundtrip without
    referencing source-tree headers or relying on the host GUI. */
 {
  fc_author_node nodes[3],decoded_nodes[3];
  fc_author_field fields[3],decoded_fields[3];
  fc_author_operation operations[1],decoded_ops[1];
  uint8_t scratch[3],decoded_scratch[3],wire[1024];
  fc_author_session author;
  size_t size=0;
  fc_author_value value={0};
  value.kind=FC_AUTHOR_STRING;
  strcpy(value.text,"Hello");
  assert(fc_author_init(&author,"external",FC_AUTHOR_SERVICE,
   nodes,3,fields,3,operations,1,scratch)==FC_OK);
  assert(fc_author_node_add(&author,"page",FC_AUTHOR_PAGE,"","Home",NULL)==FC_OK);
  assert(fc_author_node_add(&author,"button",FC_AUTHOR_COMPONENT,
   "page","Submit","floksButton")==FC_OK);
  assert(fc_author_property(&author,"button","label",value)==FC_OK);
  assert(fc_author_bind(&author,"button","label","service","api","title","text")==FC_OK);
  assert(fc_author_save(&author,NULL,0,&size)==FC_OK);
  assert(size>0 && size<sizeof(wire));
  assert(fc_author_save(&author,wire,sizeof(wire),&size)==FC_OK);
  assert(fc_author_node_remove(&author,"page")==FC_OK);
  assert(fc_author_load(&author,wire,size,decoded_nodes,decoded_fields,
   decoded_ops,decoded_scratch)==FC_OK);
  assert(author.node_count==2 && author.field_count==2);
  assert(fc_author_find(&author,"button")!=NULL);
  assert(fc_author_validate(&author)==FC_OK);
 }
 return 0;
}
