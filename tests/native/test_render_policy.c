// [XCDE] T259/T260: cache identity/lifetime, saturated readers, wraparound and explicit profile restoration.
#include <assert.h>
#include <stdio.h>
#include "../../runtime/render_policy.h"
#include "../../runtime/descriptor_cache.h"
#include "../../runtime/quality_policy.h"
int main(void) {
    uint64_t start=1000000000ull;
    assert(xcde_repeat_has_budget(start,start+10000000,30,60));
    assert(!xcde_repeat_has_budget(start,start+25000000,30,60));
    assert(!xcde_repeat_has_budget(start,start+55000000,30,60));
    assert(!xcde_repeat_has_budget(start,start,0,60));
    assert(!xcde_repeat_has_budget(start,start-1,30,60));
    assert(xcde_timestamp_delta(250,3,8)==9);
    assert(xcde_timestamp_delta(2,9,64)==7);
    assert(!xcde_timestamp_delta(2,9,0));
    int files[]={1,257,513,769}, users[]={0,1,0,2}; uint64_t ages[]={8,1,3,2};
    assert(xcde_fd_slot(files,users,ages,4,257)==1);
    assert(xcde_fd_slot(files,users,ages,4,1025)==2);
    users[0]=users[2]=1; assert(xcde_fd_slot(files,users,ages,4,1025)==-1);
    XcdeDescriptorKey keys[2]={{{1,2,3,4}},{{5,6,7,8}}};
    XcdeDescriptorEntry entry={0}; entry.sequence=11; entry.layout=3; entry.handle=9; entry.count=2;
    memcpy(entry.key,keys,sizeof keys);
    assert(xcde_descriptor_match(&entry,11,3,keys,2));
    assert(!xcde_descriptor_match(&entry,12,3,keys,2));
    assert(!xcde_descriptor_match(&entry,11,4,keys,2));
    keys[1].word[3]++; assert(!xcde_descriptor_match(&entry,11,3,keys,2));
    keys[1].word[3]--; assert(!xcde_descriptor_match(&entry,11,3,keys,1));
    Settings s={0}; s.fps=60; s.gpu_index=7; s.volume=.7f;
    xcde_quality_apply(&s,QUALITY_POTATO);
    assert(s.fps==60 && s.gpu_index==7 && s.volume==.7f);
    assert(s.res_mode==RES_DYNAMIC && s.dynamic_min_height==360 && s.dynamic_max_height==540);
    assert(!s.frame_gen && !s.upscaler_auto && !s.game_blur && s.dof_off);
    s.effects_off=31; s.cutscene_mode=2; s.upscale_quality=UQ_ULTRA;
    xcde_quality_apply(&s,QUALITY_BALANCED);
    assert(s.res_mode==RES_MATCH && !s.dynamic_max_height && !s.effects_off && s.game_blur);
    assert(!s.gpu_performance && s.fps==60 && s.gpu_index==7);
    assert(s.cutscene_mode==1 && s.upscale_quality==UQ_QUALITY);
    assert(xcde_quality_parse("performance")==QUALITY_PERFORMANCE);
    assert(xcde_quality_parse("potato")==QUALITY_POTATO);
    assert(!strcmp(xcde_quality_name(QUALITY_CUSTOM),"custom"));
    puts("render policy: deadlines, wraparound, reader lifetime, descriptor values and profile restoration passed");
    return 0;
}
