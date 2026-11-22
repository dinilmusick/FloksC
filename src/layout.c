#include "floksc/layout.h"
#include <string.h>
#include <math.h>
static bool finite_value(double d){return isfinite(d)!=0;}
static bool style_ok(fc_style s){
 return finite_value(s.x)&&finite_value(s.y)&&finite_value(s.w)&&
 finite_value(s.h)&&finite_value(s.font_size)&&s.w>=0&&s.h>=0&&s.font_size>0;
}
static size_t bounded_length(const char *text){
 size_t i;
 if(!text)return SIZE_MAX;
 for(i=0;i<FC_NAME;i++)if(!text[i])return i;
 return SIZE_MAX;
}
fc_result fc_themes_init(fc_themes *themes,fc_theme *mem,size_t cap){
 if(!themes||!mem||!cap||cap>SIZE_MAX/sizeof(*mem))return FC_INVALID;
 memset(themes,0,sizeof(*themes));memset(mem,0,cap*sizeof(*mem));
 themes->items=mem;themes->capacity=cap;return FC_OK;
}
fc_result fc_theme_add(fc_themes *themes,const char *name,size_t *index){
 size_t i,len=bounded_length(name);fc_theme *theme;
 if(!themes||!themes->items||!index||len==SIZE_MAX||!len)return FC_INVALID;
 for(i=0;i<themes->count;i++)if(strcmp(themes->items[i].name,name)==0)return FC_DUPLICATE;
 if(themes->count>=themes->capacity)return FC_CAPACITY;
 *index=themes->count++;
 theme=&themes->items[*index];memset(theme,0,sizeof(*theme));
 memcpy(theme->name,name,len+1);return FC_OK;
}
fc_result fc_theme_style(fc_themes *themes,size_t index,fc_kind kind,fc_style style){
 if(!themes||!themes->items||index>=themes->count||
 kind<FC_PANEL||kind>FC_LABEL||!style_ok(style))return FC_INVALID;
 themes->items[index].styles[kind]=style;
 themes->items[index].present[kind]=true;return FC_OK;
}
fc_result fc_theme_apply(fc_ui *ui,fc_handle h,const fc_theme *theme){
 const fc_component *c=fc_get(ui,h);
 if(!c)return FC_STALE;
 if(!theme||!theme->present[c->kind])return FC_NOT_FOUND;
 return fc_style_set(ui,h,theme->styles[c->kind]);
}
static bool direct_child(const fc_component *child,fc_handle h){
 return child->live&&child->parent.index==h.index&&child->parent.generation==h.generation;
}
fc_result fc_theme_apply_tree(fc_ui *ui,fc_handle root,const fc_theme *theme){
 size_t i;
 if(!ui||!ui->entries||!theme||!fc_get(ui,root))return FC_INVALID;
 /* Require every distinct kind in subtree to have a style, before changes. */
 for(i=0;i<ui->capacity;i++)if(ui->entries[i].live){
  fc_handle h={(uint32_t)i,ui->entries[i].generation};
  fc_handle current=h;size_t depth=0;
  while(depth<ui->capacity){
   const fc_component *c=fc_get(ui,current);
   if(!c)return FC_STALE;
   if(current.index==root.index&&current.generation==root.generation){
    if(!theme->present[ui->entries[i].kind])return FC_NOT_FOUND;
    break;
   }
   if(c->parent.index==UINT32_MAX)break;
   current=c->parent;depth++;
  }
  if(depth==ui->capacity)return FC_CYCLE;
 }
 for(i=0;i<ui->capacity;i++)if(ui->entries[i].live){
  fc_handle h={(uint32_t)i,ui->entries[i].generation};
  fc_handle current=h;size_t depth=0;
  while(depth<ui->capacity){
   const fc_component *c=fc_get(ui,current);
   if(!c)return FC_STALE;
   if(current.index==root.index&&current.generation==root.generation){
    ui->entries[i].style=theme->styles[ui->entries[i].kind];break;
   }
   if(c->parent.index==UINT32_MAX)break;
   current=c->parent;depth++;
  }
 }
 return FC_OK;
}
fc_result fc_layout_apply(fc_ui *ui,fc_handle parent,fc_layout cfg){
 size_t count=0,i,col,row,cols,rows,ordinal=0;
 double available_w,available_h,w,h,span_w,span_h,px,py;
 if(!ui||!ui->entries||!fc_get(ui,parent))return FC_STALE;
 if(cfg.kind!=FC_LAYOUT_ROW&&cfg.kind!=FC_LAYOUT_COLUMN&&cfg.kind!=FC_LAYOUT_GRID)return FC_INVALID;
 if(!finite_value(cfg.x)||!finite_value(cfg.y)||!finite_value(cfg.width)||
 !finite_value(cfg.height)||!finite_value(cfg.padding)||!finite_value(cfg.gap)||
 cfg.width<0||cfg.height<0||cfg.padding<0||cfg.gap<0)return FC_INVALID;
 for(i=0;i<ui->capacity;i++)if(direct_child(&ui->entries[i],parent))count++;
 if(count==0)return FC_OK;
 cols=cfg.kind==FC_LAYOUT_GRID?cfg.columns:(cfg.kind==FC_LAYOUT_ROW?count:1);
 if(cols==0)return FC_INVALID;
 if(cols>count)cols=count;
 rows=count/cols+(count%cols!=0);
 available_w=cfg.width-2*cfg.padding;
 available_h=cfg.height-2*cfg.padding;
 span_w=(double)(cols-1)*cfg.gap;
 span_h=(double)(rows-1)*cfg.gap;
 if(!finite_value(span_w)||!finite_value(span_h)||
 available_w<span_w||available_h<span_h)return FC_CAPACITY;
 w=(available_w-span_w)/(double)cols;
 h=(available_h-span_h)/(double)rows;
 if(!finite_value(w)||!finite_value(h)||w<0||h<0)return FC_OVERFLOW;
 /* Preflight extreme coordinates and multiplications before writing any child. */
 px=cfg.x+cfg.padding+(double)(cols-1)*(w+cfg.gap);
 py=cfg.y+cfg.padding+(double)(rows-1)*(h+cfg.gap);
 if(!finite_value(px)||!finite_value(py)||!finite_value(px+w)||!finite_value(py+h))
  return FC_OVERFLOW;
 for(i=0;i<ui->capacity;i++)if(direct_child(&ui->entries[i],parent)){
  col=ordinal%cols;row=ordinal/cols;ordinal++;
  ui->entries[i].style.x=cfg.x+cfg.padding+(double)col*(w+cfg.gap);
  ui->entries[i].style.y=cfg.y+cfg.padding+(double)row*(h+cfg.gap);
  ui->entries[i].style.w=w;ui->entries[i].style.h=h;
 }
 return FC_OK;
}
