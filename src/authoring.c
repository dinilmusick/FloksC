#include "floksc/authoring.h"
#include <math.h>
#include <string.h>
#include <stdint.h>

static bool bounded(const char *s,size_t capacity,bool empty) {
 size_t i;
 if(!s) return false;
 for(i=0;i<capacity;i++) if(s[i]=='\0') return empty || i>0;
 return false;
}
static bool name_ok(const char *s,bool empty){
 size_t i;
 if(!bounded(s,FC_NAME,empty)) return false;
 for(i=0;s[i];i++) if((unsigned char)s[i]<33 || s[i]=='/' || s[i]=='\\') return false;
 return true;
}
static bool text_ok(const char *s,bool empty){return bounded(s,FC_TEXT,empty);}
static void copy(char *out,const char *text){memcpy(out,text,strlen(text)+1);}
static bool context_ok(fc_author_context ctx){
 return ctx>=FC_AUTHOR_SERVICE && ctx<=FC_AUTHOR_APPLICATION;
}
static bool kind_ok(fc_author_kind kind){
 return kind>=FC_AUTHOR_PAGE && kind<=FC_AUTHOR_COMPONENT;
}
static bool field_kind_ok(fc_author_field_kind kind){
 return kind>=FC_AUTHOR_PROPERTY && kind<=FC_AUTHOR_METADATA;
}
static bool value_ok(fc_author_value v){
 if(v.kind<FC_AUTHOR_STRING || v.kind>FC_AUTHOR_BOOLEAN) return false;
 if(v.kind==FC_AUTHOR_STRING) return text_ok(v.text,true);
 return isfinite(v.number)!=0 || v.kind==FC_AUTHOR_BOOLEAN;
}
static fc_author_node *node_mut(fc_author_session *s,const char *id) {
 size_t i;
 if(!s || !s->nodes || !id || !name_ok(id,false)) return NULL;
 for(i=0;i<s->node_count;i++) if(strcmp(s->nodes[i].id,id)==0) return &s->nodes[i];
 return NULL;
}
const fc_author_node *fc_author_find(const fc_author_session *s,const char *id){
 return node_mut((fc_author_session *)s,id);
}
static bool same(const char *a,const char *b){return strcmp(a,b)==0;}
static bool field_matches(const fc_author_field *f,fc_author_field_kind kind,
 const char *node,const char *key,const char *selector){
 return f->kind==kind && same(f->node_id,node) && same(f->key,key) && same(f->selector,selector);
}
const fc_author_field *fc_author_field_find(const fc_author_session *s,
 fc_author_field_kind kind,const char *node,const char *key,const char *selector){
 size_t i;
 if(!s || !s->fields || !node || !key || !selector) return NULL;
 for(i=0;i<s->field_count;i++) if(field_matches(&s->fields[i],kind,node,key,selector))
  return &s->fields[i];
 return NULL;
}
fc_result fc_author_init(fc_author_session *s,const char *id,fc_author_context context,
 fc_author_node *nodes,size_t node_cap,fc_author_field *fields,size_t field_cap,
 fc_author_operation *ops,size_t operation_cap,uint8_t *scratch){
 if(!s || !name_ok(id,false) || !context_ok(context) || !nodes || !scratch ||
 !node_cap || (field_cap && !fields) || (operation_cap && !ops) ||
 node_cap>SIZE_MAX/sizeof(*nodes) || field_cap>SIZE_MAX/sizeof(*fields) ||
 operation_cap>SIZE_MAX/sizeof(*ops)) return FC_INVALID;
 memset(s,0,sizeof(*s));memset(nodes,0,node_cap*sizeof(*nodes));
 memset(scratch,0,node_cap);
 if(field_cap) memset(fields,0,field_cap*sizeof(*fields));
 if(operation_cap) memset(ops,0,operation_cap*sizeof(*ops));
 copy(s->session_id,id);s->context=context;
 s->nodes=nodes;s->node_cap=node_cap;
 s->fields=fields;s->field_cap=field_cap;
 s->operations=ops;s->operation_cap=operation_cap;
 s->scratch=scratch;
 return FC_OK;
}
fc_result fc_author_node_add(fc_author_session *s,const char *id,fc_author_kind kind,
 const char *parent,const char *name,const char *component_id){
 fc_author_node *n;
 if(!s || !s->nodes || !name_ok(id,false) || !kind_ok(kind) ||
 !name_ok(parent ? parent:"",true) || !text_ok(name?name:"",true) ||
 !name_ok(component_id?component_id:"",true)) return FC_INVALID;
 if(node_mut(s,id)) return FC_DUPLICATE;
 if(parent && parent[0] && !node_mut(s,parent)) return FC_NOT_FOUND;
 if(s->node_count>=s->node_cap) return FC_CAPACITY;
 n=&s->nodes[s->node_count++];
 memset(n,0,sizeof(*n));copy(n->id,id);
 n->kind=kind;
 copy(n->parent_id,parent?parent:"");
 copy(n->component_id,component_id?component_id:"");
 if(name && name[0]) copy(n->name,name);
 else if(kind==FC_AUTHOR_PAGE) copy(n->name,"Page");
 else if(kind==FC_AUTHOR_SECTION) copy(n->name,"Section");
 else if(component_id && component_id[0]) copy(n->name,component_id);
 else copy(n->name,"Component");
 return FC_OK;
}
static bool is_descendant(const fc_author_session *s,const char *ancestor,const char *child) {
 size_t depth=0;
 const fc_author_node *n=fc_author_find(s,child);
 while(n && n->parent_id[0] && depth<s->node_cap) {
  if(same(n->parent_id,ancestor)) return true;
  n=fc_author_find(s,n->parent_id);
  depth++;
 }
 return false;
}
fc_result fc_author_node_update(fc_author_session *s,const char *id,
 const char *new_parent,const char *new_name){
 fc_author_node *n=node_mut(s,id);
 if(!n) return FC_NOT_FOUND;
 if(!new_parent && !new_name) return FC_INVALID;
 if(new_parent && !name_ok(new_parent,true)) return FC_INVALID;
 if(new_name && !text_ok(new_name,false)) return FC_INVALID;
 if(new_parent && new_parent[0]) {
  if(!node_mut(s,new_parent)) return FC_NOT_FOUND;
  if(same(id,new_parent) || is_descendant(s,id,new_parent)) return FC_CYCLE;
  /* Slot membership is tied to the old parent; remove stale slots. */
 }
 if(new_parent){
  size_t i;
  if(!same(n->parent_id,new_parent)){
   for(i=0;i<s->field_count;){
    fc_author_field *f=&s->fields[i];
    if(f->kind==FC_AUTHOR_SLOT && same(f->value.text,id)) {
     size_t j;
     for(j=i+1;j<s->field_count;j++) s->fields[j-1]=s->fields[j];
     s->field_count--;continue;
    }
    i++;
   }
  }
  copy(n->parent_id,new_parent);
 }
 if(new_name) copy(n->name,new_name);
 return FC_OK;
}
fc_result fc_author_asset_set(fc_author_session *s,const char *id,fc_author_asset asset){
 fc_author_node *n=node_mut(s,id);
 if(!n) return FC_NOT_FOUND;
 if(!name_ok(asset.package_id,true) || !name_ok(asset.package_version,true) ||
 !text_ok(asset.export_path,true) || !name_ok(asset.asset_id,true) ||
 !name_ok(asset.asset_version,true)) return FC_INVALID;
 n->asset=asset;return FC_OK;
}
static bool marked(const fc_author_session *s,const char *id) {
 size_t j;
 for(j=0;j<s->node_count;j++) if(s->scratch[j] && same(s->nodes[j].id,id)) return true;
 return false;
}
fc_result fc_author_node_remove(fc_author_session *s,const char *id){
 size_t i,j,round,kept;
 if(!s || !s->scratch || !node_mut(s,id)) return FC_NOT_FOUND;
 memset(s->scratch,0,s->node_cap);
 for(i=0;i<s->node_count;i++) if(same(s->nodes[i].id,id)) s->scratch[i]=1;
 for(round=0;round<s->node_count;round++){
  bool changed=false;
  for(i=0;i<s->node_count;i++){
   if(!s->scratch[i] && s->nodes[i].parent_id[0] &&
       marked(s,s->nodes[i].parent_id)){
    s->scratch[i]=1;changed=true;
   }
  }
  if(!changed) break;
 }
 kept=0;
 for(i=0;i<s->field_count;i++){
  fc_author_field *f=&s->fields[i];
  if((f->node_id[0] && marked(s,f->node_id)) ||
   (f->kind==FC_AUTHOR_SLOT && marked(s,f->value.text))) continue;
  if(kept!=i) s->fields[kept]=*f;
  kept++;
 }
 for(i=kept;i<s->field_count;i++) memset(&s->fields[i],0,sizeof(*s->fields));
 s->field_count=kept;
 kept=0;
 for(i=0;i<s->node_count;i++) if(!s->scratch[i]){
  if(kept!=i) s->nodes[kept]=s->nodes[i];
  kept++;
 }
 for(j=kept;j<s->node_count;j++) memset(&s->nodes[j],0,sizeof(*s->nodes));
 s->node_count=kept;
 return FC_OK;
}
static bool valid_field(const fc_author_session *s,const fc_author_field *f){
 if(!field_kind_ok(f->kind) || !name_ok(f->node_id,true) ||
 !name_ok(f->key,false) || !name_ok(f->selector,true) || !value_ok(f->value))
  return false;
 if(f->kind==FC_AUTHOR_METADATA){
  return !f->node_id[0] && !f->selector[0] && f->value.kind==FC_AUTHOR_STRING;
 }
 if(!fc_author_find(s,f->node_id)) return false;
 if(f->kind==FC_AUTHOR_STATE_PROPERTY) {
  if(!f->selector[0])return false;
 } else if(f->kind!=FC_AUTHOR_SLOT && f->selector[0]) return false;
 if(f->kind==FC_AUTHOR_SLOT){
  const fc_author_node *child=fc_author_find(s,f->value.text);
  if(!child || !same(child->parent_id,f->node_id) ||
    f->value.kind!=FC_AUTHOR_STRING) return false;
 }
 if(f->kind==FC_AUTHOR_EVENT && f->value.kind!=FC_AUTHOR_STRING) return false;
 if(f->kind==FC_AUTHOR_BINDING){
  if(!name_ok(f->target_kind,false) || !name_ok(f->target_api,false) ||
   !name_ok(f->target_field,false) || !name_ok(f->target_type,true))return false;
 }
 return true;
}
static bool field_same_key(const fc_author_field *a,const fc_author_field *b) {
 return field_matches(a,b->kind,b->node_id,b->key,b->selector) &&
 (a->kind!=FC_AUTHOR_SLOT || same(a->value.text,b->value.text));
}
static fc_result field_put(fc_author_session *s,fc_author_field f){
 size_t i;
 if(!s || !s->fields || !valid_field(s,&f))return FC_INVALID;
 for(i=0;i<s->field_count;i++) if(field_same_key(&s->fields[i],&f)){
  s->fields[i]=f;return FC_OK;
 }
 if(s->field_count>=s->field_cap)return FC_CAPACITY;
 s->fields[s->field_count++]=f;return FC_OK;
}
static fc_result field_erase(fc_author_session *s,fc_author_field_kind kind,
 const char *node,const char *key,const char *selector,const char *slot_child){
 size_t i,j;
 if(!s || !s->fields || !node || !key || !selector)return FC_INVALID;
 for(i=0;i<s->field_count;i++){
  fc_author_field *f=&s->fields[i];
  if(field_matches(f,kind,node,key,selector) &&
   (!slot_child || same(slot_child,f->value.text))){
   for(j=i+1;j<s->field_count;j++) s->fields[j-1]=s->fields[j];
   s->field_count--;memset(&s->fields[s->field_count],0,sizeof(*s->fields));
   return FC_OK;
  }
 }
 return FC_NOT_FOUND;
}
static fc_result scalar_put(fc_author_session *s,fc_author_field_kind kind,
 const char *node,const char *key,const char *selector,fc_author_value value){
 fc_author_field f;
 if(!node || !key || !selector)return FC_INVALID;
 memset(&f,0,sizeof(f));
 if(!name_ok(node,true)||!name_ok(key,false)||!name_ok(selector,true))return FC_INVALID;
 f.kind=kind;copy(f.node_id,node);copy(f.key,key);copy(f.selector,selector);
 f.value=value;return field_put(s,f);
}
fc_result fc_author_property(fc_author_session *s,const char *node,const char *key,fc_author_value value){
 return scalar_put(s,FC_AUTHOR_PROPERTY,node,key,"",value);
}
fc_result fc_author_state_property(fc_author_session *s,const char *node,const char *state,
 const char *key,fc_author_value value){
 return scalar_put(s,FC_AUTHOR_STATE_PROPERTY,node,key,state,value);
}
static fc_author_value string_value(const char *s) {
 fc_author_value value;memset(&value,0,sizeof(value));value.kind=FC_AUTHOR_STRING;
 if(s && text_ok(s,true))copy(value.text,s);
 return value;
}
fc_result fc_author_event(fc_author_session *s,const char *node,const char *event,const char *handler){
 if(!name_ok(handler,false))return FC_INVALID;
 return scalar_put(s,FC_AUTHOR_EVENT,node,event,"",string_value(handler));
}
fc_result fc_author_slot(fc_author_session *s,const char *node,const char *slot,const char *child){
 if(!name_ok(child,false))return FC_INVALID;
 return scalar_put(s,FC_AUTHOR_SLOT,node,slot,child,string_value(child));
}
fc_result fc_author_unslot(fc_author_session *s,const char *node,const char *slot,const char *child){
 if(!name_ok(node,false)||!name_ok(slot,false)||!name_ok(child,false))return FC_INVALID;
 return field_erase(s,FC_AUTHOR_SLOT,node,slot,child,child);
}
fc_result fc_author_bind(fc_author_session *s,const char *node,const char *property,
 const char *kind,const char *api,const char *field,const char *type){
 fc_author_field f;
 if(!name_ok(node,false)||!name_ok(property,false)||!name_ok(kind,false)||
 !name_ok(api,false)||!name_ok(field,false)||!name_ok(type?type:"",true))return FC_INVALID;
 memset(&f,0,sizeof(f));
 f.kind=FC_AUTHOR_BINDING;copy(f.node_id,node);copy(f.key,property);
 f.value.kind=FC_AUTHOR_STRING;
 copy(f.target_kind,kind);copy(f.target_api,api);copy(f.target_field,field);
 copy(f.target_type,type?type:"");return field_put(s,f);
}
fc_result fc_author_unbind(fc_author_session *s,const char *node,const char *property){
 if(!name_ok(node,false)||!name_ok(property,false))return FC_INVALID;
 return field_erase(s,FC_AUTHOR_BINDING,node,property,"",NULL);
}
fc_result fc_author_metadata(fc_author_session *s,const char *key,const char *value){
 if(!text_ok(value,true))return FC_INVALID;
 return scalar_put(s,FC_AUTHOR_METADATA,"",key,"",string_value(value));
}
static bool operation_valid(fc_author_operation op){
 size_t i,j;
 if(!name_ok(op.id,false)||!text_ok(op.name,false)||!text_ok(op.description,true)||
 op.input_count>FC_AUTHOR_MAX_IO||op.output_count>FC_AUTHOR_MAX_IO)return false;
 for(i=0;i<op.input_count;i++){
  if(!name_ok(op.inputs[i].name,false)||!name_ok(op.inputs[i].type,false))return false;
  for(j=0;j<i;j++)if(same(op.inputs[j].name,op.inputs[i].name))return false;
 }
 for(i=0;i<op.output_count;i++){
  if(!name_ok(op.outputs[i].name,false)||!name_ok(op.outputs[i].type,false))return false;
  for(j=0;j<i;j++)if(same(op.outputs[j].name,op.outputs[i].name))return false;
 }
 return true;
}
fc_result fc_author_operation_set(fc_author_session *s,fc_author_operation op){
 size_t i;
 if(!s || !operation_valid(op))return FC_INVALID;
 for(i=0;i<s->operation_count;i++)if(same(s->operations[i].id,op.id)){
  s->operations[i]=op;return FC_OK;
 }
 if(s->operation_count>=s->operation_cap)return FC_CAPACITY;
 s->operations[s->operation_count++]=op;return FC_OK;
}
fc_result fc_author_operation_remove(fc_author_session *s,const char *id){
 size_t i,j;
 if(!s || !name_ok(id,false))return FC_INVALID;
 for(i=0;i<s->operation_count;i++)if(same(s->operations[i].id,id)){
  for(j=i+1;j<s->operation_count;j++)s->operations[j-1]=s->operations[j];
  s->operation_count--;memset(&s->operations[s->operation_count],0,sizeof(*s->operations));
  return FC_OK;
 }
 return FC_NOT_FOUND;
}
fc_result fc_author_validate(const fc_author_session *s){
 size_t i,j,d;
 if(!s || !context_ok(s->context) || !name_ok(s->session_id,false) ||
 !s->nodes || !s->scratch || !s->node_cap || s->node_count>s->node_cap ||
 s->field_count>s->field_cap || s->operation_count>s->operation_cap ||
 (s->field_cap && !s->fields) || (s->operation_cap && !s->operations))
  return FC_INVALID;
 for(i=0;i<s->node_count;i++){
  const fc_author_node *n=&s->nodes[i];
  const char *parent=n->parent_id;
  if(!name_ok(n->id,false) || !name_ok(parent,true) || !text_ok(n->name,false) ||
   !kind_ok(n->kind) || !name_ok(n->component_id,true) ||
   !name_ok(n->asset.package_id,true) || !name_ok(n->asset.package_version,true) ||
   !name_ok(n->asset.asset_id,true) || !name_ok(n->asset.asset_version,true) ||
   !text_ok(n->asset.export_path,true))return FC_INVALID;
  for(j=0;j<i;j++)if(same(s->nodes[j].id,n->id))return FC_DUPLICATE;
  d=0;
  while(parent[0] && d<s->node_count){
   const fc_author_node *p=fc_author_find(s,parent);
   if(!p)return FC_NOT_FOUND;
   if(same(p->id,n->id))return FC_CYCLE;
   parent=p->parent_id;d++;
  }
  if(d>=s->node_count && parent[0])return FC_CYCLE;
 }
 for(i=0;i<s->field_count;i++){
  if(!valid_field(s,&s->fields[i]))return FC_INVALID;
  for(j=0;j<i;j++)if(field_same_key(&s->fields[i],&s->fields[j]))return FC_DUPLICATE;
 }
 for(i=0;i<s->operation_count;i++){
  if(!operation_valid(s->operations[i]))return FC_INVALID;
  for(j=0;j<i;j++)if(same(s->operations[i].id,s->operations[j].id))return FC_DUPLICATE;
 }
 return FC_OK;
}
