#include <floksc/floksc.h>
#include <assert.h>
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
 return 0;
}
