#include "floksc/layout.h"
#include <assert.h>
#include <math.h>
int main(void) {
 fc_ui ui;fc_component cs[5];fc_handle root,a,b,c;
 fc_theme colors[2];fc_themes themes;size_t index=99;
 fc_style panel={0,0,300,200,14,0xFFFFFF,0x000000};
 fc_style tile={0,0,100,40,12,0x111111,0x222222};
 assert(fc_init(&ui,cs,5)==FC_OK);
 assert(fc_add(&ui,FC_PANEL,"root",fc_invalid(),&root)==FC_OK);
 assert(fc_add(&ui,FC_TILE,"a",root,&a)==FC_OK);
 assert(fc_add(&ui,FC_TILE,"b",root,&b)==FC_OK);
 assert(fc_add(&ui,FC_TILE,"c",root,&c)==FC_OK);
 assert(fc_themes_init(&themes,colors,2)==FC_OK);
 assert(fc_theme_add(&themes,"dark",&index)==FC_OK&&index==0);
 assert(fc_theme_add(&themes,"dark",&index)==FC_DUPLICATE);
 assert(fc_theme_style(&themes,0,FC_PANEL,panel)==FC_OK);
 assert(fc_theme_apply_tree(&ui,root,&themes.items[0])==FC_NOT_FOUND);
 assert(fc_theme_style(&themes,0,FC_TILE,tile)==FC_OK);
 assert(fc_theme_apply_tree(&ui,root,&themes.items[0])==FC_OK);
 assert(fc_get(&ui,c)->style.background==0x222222);
 assert(fc_layout_apply(&ui,root,(fc_layout){FC_LAYOUT_ROW,0,0,300,100,10,5,0})==FC_OK);
 assert(fabs(fc_get(&ui,a)->style.w-90)<1e-9);
 assert(fabs(fc_get(&ui,c)->style.x-200)<1e-9);
 assert(fc_layout_apply(&ui,root,(fc_layout){FC_LAYOUT_GRID,0,0,200,200,0,0,2})==FC_OK);
 assert(fc_get(&ui,c)->style.y==100&&fc_get(&ui,c)->style.x==0);
 double old=fc_get(&ui,c)->style.y;
 assert(fc_layout_apply(&ui,root,(fc_layout){FC_LAYOUT_GRID,0,0,10,10,10,20,2})==FC_CAPACITY);
 assert(fc_get(&ui,c)->style.y==old);
 assert(fc_layout_apply(&ui,root,(fc_layout){FC_LAYOUT_GRID,0,0,10,10,0,0,0})==FC_INVALID);
 assert(fc_theme_apply(&ui,c,&themes.items[0])==FC_OK);
 assert(fc_theme_style(&themes,0,FC_TILE,(fc_style){0,0,-1,1,12,0,0})==FC_INVALID);
 assert(fc_destroy(&ui,root)==FC_OK);
 assert(fc_theme_apply(&ui,c,&themes.items[0])==FC_STALE);
 return 0;
}
