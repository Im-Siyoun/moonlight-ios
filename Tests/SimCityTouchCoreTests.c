// SPDX-License-Identifier: GPL-3.0-or-later
#include "SimCityTouchCore.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    SCEvent events[4096];
    size_t count;
    bool left, keys[256];
    unsigned downs, ups, wheel;
} Recorder;

static void record(void *context, SCEvent e) {
    Recorder *r = context;
    assert(r->count < 4096);
    r->events[r->count++] = e;
    if (e.type == SCLeftButton) {
        assert(r->left != e.down); // balanced, non-duplicated edges
        r->left = e.down;
        if (e.down) r->downs++; else r->ups++;
    }
    if (e.type == SCKey) {
        assert(!r->left);
        assert(r->keys[e.code] != e.down);
        r->keys[e.code] = e.down;
    }
    if (e.type == SCWheel) { assert(!r->left); r->wheel++; }
}

static void init(SCState *s, Recorder *r) { memset(r, 0, sizeof(*r)); SCInit(s, record, r); }
static void released(Recorder *r) {
    assert(!r->left);
    for (unsigned i = 0; i < 256; i++) assert(!r->keys[i]);
    assert(r->downs == r->ups);
}

static void tapAndDrag(void) {
    SCState s; Recorder r; init(&s, &r);
    SCTouch t = {1, 50, 60};
    SCUpdate(&s, &t, 1, 0);
    assert(r.count == 0);
    SCUpdate(&s, NULL, 0, .08);
    assert(r.left && r.downs == 1);
    SCTick(&s, .14);
    released(&r);
    SCUpdate(&s, &t, 1, 1);
    t.x = 80;
    SCUpdate(&s, &t, 1, 1.05);
    assert(!r.left);
    SCTick(&s, 1.13);
    assert(r.left && s.mode == SCSingleDrag);
    assert(r.events[r.count - 3].type == SCMove && r.events[r.count - 3].x == 50);
    assert(r.events[r.count - 1].type == SCMove && r.events[r.count - 1].x == 80);
    t.x = 100; SCUpdate(&s, &t, 1, 1.2);
    assert(r.events[r.count - 1].x == 100);
    SCUpdate(&s, NULL, 0, 1.3); released(&r);
    puts("PASS tap, delayed click release, continuous drag from original point");
}

static void transitions(void) {
    SCState s; Recorder r;
    SCTouch t[] = {{1, 50, 50}, {2, 150, 50}, {3, 250, 50}};
    init(&s, &r);
    SCUpdate(&s, t, 1, 0);
    SCUpdate(&s, t, 2, .05); // second finger before click commitment
    assert(r.downs == 0);
    SCUpdate(&s, t, 1, .1);
    assert(s.mode == SCBlocked);
    SCTick(&s, 1); assert(r.downs == 0);
    SCUpdate(&s, NULL, 0, 1.1);
    SCUpdate(&s, t, 1, 2); SCTick(&s, 2.13); assert(r.left);
    SCUpdate(&s, t, 2, 2.2); assert(!r.left);
    t[0].x += 20; t[1].x += 20;
    SCUpdate(&s, t, 2, 2.3); assert(r.keys['D']);
    SCUpdate(&s, t, 3, 2.4); released(&r);
    SCUpdate(&s, t, 2, 2.5); assert(s.mode == SCBlocked);
    SCUpdate(&s, NULL, 0, 3); assert(s.mode == SCIdle);
    puts("PASS 1-to-2 release, 2-to-1 suppression, third finger cancellation");
}

static void pan(void) {
    SCState s; Recorder r; init(&s, &r);
    SCTouch t[] = {{1, 100, 100}, {2, 200, 100}};
    SCUpdate(&s, t, 2, 0);
    t[0].x += 20; t[1].x += 20; t[0].y -= 20; t[1].y -= 20;
    SCUpdate(&s, t, 2, .02); assert(s.mode == SCPan && r.keys['W'] && r.keys['D']);
    SCTick(&s, .15); released(&r); // stationary fingers
    t[0].x -= 30; t[1].x -= 30; t[0].y += 30; t[1].y += 30;
    SCUpdate(&s, t, 2, .2); assert(r.keys['A'] && r.keys['S']);
    t[0].x += 40; t[1].x += 40;
    SCUpdate(&s, t, 2, .22);
    assert(!r.keys['A'] && !r.keys['S'] && r.keys['D']);
    SCUpdate(&s, t, 1, .3); released(&r);
    puts("PASS WASD diagonal, reverse, stationary watchdog, finger lift");
}

static void pinchAndRotation(void) {
    SCState s; Recorder r; init(&s, &r);
    SCTouch t[] = {{1, 100, 100}, {2, 200, 100}};
    SCUpdate(&s, t, 2, 0);
    t[0].x -= 20; t[1].x += 20;
    SCUpdate(&s, t, 2, .02); assert(s.mode == SCPinch && r.wheel == 1);
    assert(r.events[r.count - 1].code > 0);
    t[0].x += 40; t[1].x -= 40;
    SCUpdate(&s, t, 2, .04); assert(r.events[r.count - 1].code < 0);
    SCUpdate(&s, NULL, 0, .1); released(&r);
    init(&s, &r);
    t[0] = (SCTouch){1, 150 - 50 * cos(3.1), 100 - 50 * sin(3.1)};
    t[1] = (SCTouch){2, 150 + 50 * cos(3.1), 100 + 50 * sin(3.1)};
    SCUpdate(&s, t, 2, 0);
    t[0] = (SCTouch){1, 150 - 50 * cos(-3.0), 100 - 50 * sin(-3.0)};
    t[1] = (SCTouch){2, 150 + 50 * cos(-3.0), 100 + 50 * sin(-3.0)};
    SCUpdate(&s, t, 2, .02); assert(s.mode == SCRotate && r.keys['E']);
    t[0] = (SCTouch){1, 150 - 50 * cos(2.9), 100 - 50 * sin(2.9)};
    t[1] = (SCTouch){2, 150 + 50 * cos(2.9), 100 + 50 * sin(2.9)};
    SCUpdate(&s, t, 2, .04); assert(r.keys['Q'] && !r.keys['E']);
    SCTick(&s, .2); released(&r);
    assert(r.wheel == 0 && r.downs == 0);
    puts("PASS wheel direction, rotation angle wrapping, Q/E reversal, gesture isolation");
}

static void cancelEveryMode(void) {
    for (int mode = SCIdle; mode <= SCBlocked; mode++) {
        SCState s; Recorder r; init(&s, &r);
        // Model the outputs each state is allowed to own before interruption.
        s.mode = (SCMode)mode;
        if (mode == SCSingleDrag || mode == SCTapRelease) s.leftDown = r.left = true, r.downs = 1;
        if (mode == SCPan) s.keys = 9, r.keys['W'] = r.keys['D'] = true;
        if (mode == SCRotate) s.keys = 32, r.keys['E'] = true;
        SCCancel(&s); SCCancel(&s); SCTick(&s, 100);
        released(&r);
        assert(s.mode == SCBlocked);
        SCUpdate(&s, NULL, 0, 101); assert(s.mode == SCIdle);
    }
    puts("PASS idempotent interruption/reset in all nine states");
}

static void replacementAndJitter(void) {
    SCState s; Recorder r; init(&s, &r);
    SCTouch t[] = {{1, 100, 100}, {2, 200, 100}};
    SCUpdate(&s, t, 2, 0);
    for (int i = 1; i < 100; i++) {
        t[0].x = 100 + sin(i) * 2; t[1].x = 200 + cos(i) * 2;
        SCUpdate(&s, t, 2, i / 120.0);
    }
    assert(r.count == 0);
    t[0].x += 30; t[1].x += 30;
    SCUpdate(&s, t, 2, 1); assert(r.keys['D']);
    t[1].id = 3; SCUpdate(&s, t, 2, 1.1); released(&r);
    assert(s.mode == SCBlocked);
    puts("PASS jitter deadzones, finger identity replacement");
}

int main(void) {
    tapAndDrag(); transitions(); pan(); pinchAndRotation(); cancelEveryMode(); replacementAndJitter();
    puts("All SimCity touch core tests passed.");
}
