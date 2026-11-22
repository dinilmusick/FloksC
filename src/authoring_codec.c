#include "floksc/authoring.h"
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

/* Explicit, platform-independent, little-endian versioned FCAS v1.
 * Scratch storage and input/output bytes must be disjoint. */
typedef struct {
 uint8_t *output;const uint8_t *input;size_t pos,capacity;fc_result status;
} fc_wire;
static bool binary64(void){
 return sizeof(double)==8 && FLT_RADIX==2 && DBL_MANT_DIG==53 && DBL_MAX_EXP==1024;
}
static void put(fc_wire *w,uint8_t byte){
 if(w->status!=FC_OK && w->status!=FC_CAPACITY)return;
 if(w->pos==SIZE_MAX){w->status=FC_OVERFLOW;return;}
 if(w->output){
  if(w->pos<w->capacity && w->status==FC_OK)w->output[w->pos]=byte;
  else w->status=FC_CAPACITY;
 }
 w->pos++;
}
static uint8_t get(fc_wire *w){
 if(w->status!=FC_OK)return 0;
 if(w->pos>=w->capacity){w->status=FC_INVALID;return 0;}
 return w->input[w->pos++];
}
static void u32put(fc_wire *w,uint32_t n){
 unsigned int i;for(i=0;i<4;i++)put(w,(uint8_t)(n>>(8*i)));
}
static uint32_t u32get(fc_wire *w){
 uint32_t n=0;unsigned int i;
 for(i=0;i<4;i++)n|=((uint32_t)get(w))<<(8*i);
 return n;
}
static void u64put(fc_wire *w,uint64_t n){
 unsigned int i;for(i=0;i<8;i++)put(w,(uint8_t)(n>>(8*i)));
}
static uint64_t u64get(fc_wire *w){
 uint64_t n=0;unsigned int i;
 for(i=0;i<8;i++)n|=((uint64_t)get(w))<<(8*i);
 return n;
}
static void double_put(fc_wire *w,double value){
 uint64_t bits=0;
 if(!isfinite(value)){w->status=FC_INVALID;return;}
 memcpy(&bits,&value,sizeof(bits));u64put(w,bits);
}
static double double_get(fc_wire *w){
 uint64_t bits=u64get(w);double value=0;
 memcpy(&value,&bits,sizeof(value));
 if(!isfinite(value))w->status=FC_INVALID;
 return value;
}
static void string_put(fc_wire *w,const char *text,size_t cap){
 size_t len=0,i;
 if(!text){w->status=FC_INVALID;return;}
 while(len<cap && text[len])len++;
 if(len>=cap || len>=256){w->status=FC_INVALID;return;}
 put(w,(uint8_t)len);
 for(i=0;i<len;i++)put(w,(uint8_t)text[i]);
}
static void string_get(fc_wire *w,char *out,size_t cap){
 size_t len,i;
 if(w->status!=FC_OK)return;
 len=get(w);
 if(w->status!=FC_OK || len>=cap){w->status=FC_INVALID;return;}
 memset(out,0,cap);
 for(i=0;i<len;i++){
  uint8_t c=get(w);
  if(w->status!=FC_OK || c==0){w->status=FC_INVALID;return;}
  out[i]=(char)c;
 }
}
static void value_put(fc_wire *w,fc_author_value value){
 put(w,(uint8_t)value.kind);
 if(value.kind==FC_AUTHOR_STRING)string_put(w,value.text,FC_TEXT);
 else if(value.kind==FC_AUTHOR_NUMBER)double_put(w,value.number);
 else if(value.kind==FC_AUTHOR_BOOLEAN)put(w,value.boolean?1:0);
 else w->status=FC_INVALID;
}
static void value_get(fc_wire *w,fc_author_value *value){
 memset(value,0,sizeof(*value));
 value->kind=(fc_author_value_kind)get(w);
 if(value->kind==FC_AUTHOR_STRING)string_get(w,value->text,FC_TEXT);
 else if(value->kind==FC_AUTHOR_NUMBER)value->number=double_get(w);
 else if(value->kind==FC_AUTHOR_BOOLEAN){
  uint8_t v=get(w);
  if(v>1)w->status=FC_INVALID;
  else value->boolean=v!=0;
 }else w->status=FC_INVALID;
}
static void magic_put(fc_wire *w){
 put(w,'F');put(w,'C');put(w,'A');put(w,'1');
}
static void magic_get(fc_wire *w){
 if(get(w)!='F' || get(w)!='C' || get(w)!='A' || get(w)!='1')
  w->status=FC_INVALID;
}
static void asset_put(fc_wire *w,const fc_author_asset *a){
 string_put(w,a->package_id,FC_NAME);
 string_put(w,a->package_version,FC_NAME);
 string_put(w,a->export_path,FC_TEXT);
 string_put(w,a->asset_id,FC_NAME);
 string_put(w,a->asset_version,FC_NAME);
}
static void asset_get(fc_wire *w,fc_author_asset *a){
 string_get(w,a->package_id,FC_NAME);
 string_get(w,a->package_version,FC_NAME);
 string_get(w,a->export_path,FC_TEXT);
 string_get(w,a->asset_id,FC_NAME);
 string_get(w,a->asset_version,FC_NAME);
}
static void node_put(fc_wire *w,const fc_author_node *n){
 put(w,(uint8_t)n->kind);
 string_put(w,n->id,FC_NAME);
 string_put(w,n->parent_id,FC_NAME);
 string_put(w,n->name,FC_TEXT);
 string_put(w,n->component_id,FC_NAME);
 asset_put(w,&n->asset);
}
static void node_get(fc_wire *w,fc_author_node *n){
 memset(n,0,sizeof(*n));
 n->kind=(fc_author_kind)get(w);
 string_get(w,n->id,FC_NAME);
 string_get(w,n->parent_id,FC_NAME);
 string_get(w,n->name,FC_TEXT);
 string_get(w,n->component_id,FC_NAME);
 asset_get(w,&n->asset);
}
static void field_put(fc_wire *w,const fc_author_field *f){
 put(w,(uint8_t)f->kind);
 string_put(w,f->node_id,FC_NAME);
 string_put(w,f->key,FC_NAME);
 string_put(w,f->selector,FC_NAME);
 value_put(w,f->value);
 if(f->kind==FC_AUTHOR_BINDING){
  string_put(w,f->target_kind,FC_NAME);
  string_put(w,f->target_api,FC_NAME);
  string_put(w,f->target_field,FC_NAME);
  string_put(w,f->target_type,FC_NAME);
 }
}
static void field_get(fc_wire *w,fc_author_field *f){
 memset(f,0,sizeof(*f));
 f->kind=(fc_author_field_kind)get(w);
 string_get(w,f->node_id,FC_NAME);
 string_get(w,f->key,FC_NAME);
 string_get(w,f->selector,FC_NAME);
 value_get(w,&f->value);
 if(f->kind==FC_AUTHOR_BINDING){
  string_get(w,f->target_kind,FC_NAME);
  string_get(w,f->target_api,FC_NAME);
  string_get(w,f->target_field,FC_NAME);
  string_get(w,f->target_type,FC_NAME);
 }
}
static void operation_put(fc_wire *w,const fc_author_operation *op){
 size_t i;
 string_put(w,op->id,FC_NAME);
 string_put(w,op->name,FC_TEXT);
 string_put(w,op->description,FC_TEXT);
 put(w,(uint8_t)op->input_count);
 for(i=0;i<op->input_count;i++){
  string_put(w,op->inputs[i].name,FC_NAME);
  string_put(w,op->inputs[i].type,FC_NAME);
 }
 put(w,(uint8_t)op->output_count);
 for(i=0;i<op->output_count;i++){
  string_put(w,op->outputs[i].name,FC_NAME);
  string_put(w,op->outputs[i].type,FC_NAME);
 }
}
static void operation_get(fc_wire *w,fc_author_operation *op){
 size_t i;
 memset(op,0,sizeof(*op));
 string_get(w,op->id,FC_NAME);
 string_get(w,op->name,FC_TEXT);
 string_get(w,op->description,FC_TEXT);
 op->input_count=get(w);
 if(op->input_count>FC_AUTHOR_MAX_IO){w->status=FC_INVALID;return;}
 for(i=0;i<op->input_count;i++){
  string_get(w,op->inputs[i].name,FC_NAME);
  string_get(w,op->inputs[i].type,FC_NAME);
 }
 op->output_count=get(w);
 if(op->output_count>FC_AUTHOR_MAX_IO){w->status=FC_INVALID;return;}
 for(i=0;i<op->output_count;i++){
  string_get(w,op->outputs[i].name,FC_NAME);
  string_get(w,op->outputs[i].type,FC_NAME);
 }
}
fc_result fc_author_save(const fc_author_session *s,uint8_t *out,size_t capacity,size_t *written){
 fc_wire w={0};size_t i;fc_result valid;
 if(written)*written=0;
 if(!s || !written || !binary64())return FC_INVALID;
 valid=fc_author_validate(s);
 if(valid!=FC_OK)return valid;
 if(s->node_count>UINT32_MAX || s->field_count>UINT32_MAX || s->operation_count>UINT32_MAX)
  return FC_CAPACITY;
 w.output=out;w.capacity=capacity;
 magic_put(&w);
 string_put(&w,s->session_id,FC_NAME);
 put(&w,(uint8_t)s->context);
 u32put(&w,(uint32_t)s->node_count);
 u32put(&w,(uint32_t)s->field_count);
 u32put(&w,(uint32_t)s->operation_count);
 for(i=0;i<s->node_count;i++)node_put(&w,&s->nodes[i]);
 for(i=0;i<s->field_count;i++)field_put(&w,&s->fields[i]);
 for(i=0;i<s->operation_count;i++)operation_put(&w,&s->operations[i]);
 *written=w.pos;
 return w.status;
}
fc_result fc_author_load(fc_author_session *target,const uint8_t *data,size_t len,
 fc_author_node *nodes,fc_author_field *fields,fc_author_operation *operations,uint8_t *scratch){
 fc_wire w={0};fc_author_session staged;size_t i,nn,nf,no;fc_result r;
 char id[FC_NAME]={0};fc_author_context context;
 if(!target||!data||!len||!nodes||!scratch||
  (target->field_cap&&!fields)||(target->operation_cap&&!operations)||
  nodes==target->nodes || fields==target->fields ||
  operations==target->operations || scratch==target->scratch || !binary64())
  return FC_INVALID;
 w.input=data;w.capacity=len;
 magic_get(&w);
 string_get(&w,id,FC_NAME);
 context=(fc_author_context)get(&w);
 nn=u32get(&w);nf=u32get(&w);no=u32get(&w);
 if(w.status!=FC_OK)return FC_INVALID;
 if(nn>target->node_cap||nf>target->field_cap||no>target->operation_cap)
  return FC_CAPACITY;
 r=fc_author_init(&staged,id,context,nodes,target->node_cap,fields,target->field_cap,
  operations,target->operation_cap,scratch);
 if(r!=FC_OK)return r;
 for(i=0;i<nn&&w.status==FC_OK;i++)node_get(&w,&staged.nodes[i]);
 for(i=0;i<nf&&w.status==FC_OK;i++)field_get(&w,&staged.fields[i]);
 for(i=0;i<no&&w.status==FC_OK;i++)operation_get(&w,&staged.operations[i]);
 if(w.status!=FC_OK || w.pos!=w.capacity)return FC_INVALID;
 staged.node_count=nn;staged.field_count=nf;staged.operation_count=no;
 if((r=fc_author_validate(&staged))!=FC_OK)return r;
 memcpy(target->nodes,staged.nodes,target->node_cap*sizeof(*target->nodes));
 if(target->field_cap)memcpy(target->fields,staged.fields,target->field_cap*sizeof(*target->fields));
 if(target->operation_cap)memcpy(target->operations,staged.operations,
  target->operation_cap*sizeof(*target->operations));
 memset(target->scratch,0,target->node_cap);
 memcpy(target->session_id,staged.session_id,FC_NAME);
 target->context=staged.context;
 target->node_count=nn;target->field_count=nf;target->operation_count=no;
 return FC_OK;
}
