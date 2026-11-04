// SPDX-License-Identifier: GPL-3.0-or-later
// Platform-independent recognizer. Coordinates are UIKit points, time is monotonic seconds.
#ifndef SIMCITY_TOUCH_CORE_H
#define SIMCITY_TOUCH_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct { uintptr_t id; double x, y; } SCTouch;
typedef enum {
    SCIdle, SCSinglePending, SCSingleDrag, SCTapRelease,
    SCTwoPending, SCPan, SCPinch, SCRotate, SCBlocked
} SCMode;
typedef enum { SCMove, SCLeftButton, SCKey, SCWheel } SCEventType;
// SCKey uses Windows VK codes; SCWheel uses signed wheel clicks, not raw WHEEL_DELTA.
typedef struct { SCEventType type; int code; bool down; double x, y; } SCEvent;
typedef void (*SCEmit)(void *context, SCEvent event);
typedef struct {
    SCMode mode;
    SCEmit emit;
    void *context;
    uintptr_t ids[2];
    double startX, startY, x, y, distance, angle;
    double startedAt, releaseAt, motionAt, wheelRemainder;
    bool leftDown;
    unsigned keys;
} SCState;

void SCInit(SCState *s, SCEmit emit, void *context);
void SCUpdate(SCState *s, const SCTouch *touches, size_t count, double now);
void SCTick(SCState *s, double now);
// Release only inputs owned by this recognizer, then ignore the rest of this contact sequence.
void SCCancel(SCState *s);

#endif
