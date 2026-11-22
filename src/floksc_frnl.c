#include "floksc/floksc.h"
#include <limits.h>
static const char *type_name(fc_kind type){
 switch(type){
 case FC_PANEL:return "panel";case FC_BUTTON:return "button";case FC_INPUT:return "input";
 case FC_MENU:return "menu";case FC_TILE:return "tile";case FC_SELECT:return "selection";
 case FC_TAB:return "tabs";case FC_DIALOG:return "dialog";case FC_LABEL:return "label";
 default:return NULL;
 }
}
fc_result fc_frnl_register(frnl_runtime *rt,const fc_ui *ui,fc_handle h,const char *state){
 const fc_component *c=fc_get(ui,h),*p;
 const char *type,*parent=NULL;
 if(!rt||!c||!state||!state[0])return FC_INVALID;
 type=type_name(c->kind);if(!type)return FC_INVALID;
 if(c->parent.index!=UINT32_MAX){
  p=fc_get(ui,c->parent);if(!p)return FC_STALE;
  parent=p->id;
 }
 /* Host must define FrnlC state type before registering components. */
 return frnl_add_component(rt,c->id,parent,type,state)==FRNL_OK?FC_OK:FC_FAILED;
}
fc_result fc_frnl_event(frnl_runtime *rt,const fc_ui *ui,fc_handle h,const char *event,const void *payload){
 const fc_component *c=fc_get(ui,h);
 if(!rt||!c||!event||!event[0])return FC_INVALID;
 return frnl_emit(rt,c->id,event,payload)==FRNL_OK?FC_OK:FC_FAILED;
}
