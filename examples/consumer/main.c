#include <floksc/floksc.h>
#include <assert.h>
int main(void) {
 fc_component storage[2];fc_ui ui;fc_handle button;
 frnl_component components[2];frnl_state_spec states[1];
 frnl_storage memory={0};frnl_runtime rt;
 memory.components=components;memory.component_capacity=2;
 memory.states=states;memory.state_capacity=1;memory.max_depth=2;
 assert(frnl_init(&rt,&memory)==FRNL_OK);
 assert(frnl_define_state(&rt,(frnl_state_spec){"button","idle",0,0})==FRNL_OK);
 assert(fc_init(&ui,storage,2)==FC_OK);
 assert(fc_add(&ui,FC_BUTTON,"myButton",fc_invalid(),&button)==FC_OK);
 assert(fc_frnl_register(&rt,&ui,button,"idle")==FC_OK);
 assert(rt.components==1);
 assert(fc_activate(&ui,button,"click")==FC_OK);
 frnl_cleanup(&rt);
 return 0;
}
