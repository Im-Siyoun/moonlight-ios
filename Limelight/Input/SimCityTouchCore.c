// SPDX-License-Identifier: GPL-3.0-or-later
#include "SimCityTouchCore.h"
#include <math.h>
#include <string.h>

static const int keyCodes[] = { 'W', 'A', 'S', 'D', 'Q', 'E' };
enum { W = 1, A = 2, S = 4, D = 8, Q = 16, E = 32 };
static const double singleDelay = 0.120;
static const double motionTimeout = 0.120;
static const double wheelStep = 0.077; // approximately 8 percent scale per wheel click

static void output(SCState *s, SCEventType type, int code, bool down, double x, double y) {
    s->emit(s->context, (SCEvent){type, code, down, x, y});
}

static void keys(SCState *s, unsigned desired) {
    // Releases precede presses, including direction reversals.
    for (unsigned i = 0; i < 6; i++) {
        if ((s->keys & ~desired) & (1u << i)) output(s, SCKey, keyCodes[i], false, 0, 0);
    }
    for (unsigned i = 0; i < 6; i++) {
        if ((desired & ~s->keys) & (1u << i)) output(s, SCKey, keyCodes[i], true, 0, 0);
    }
    s->keys = desired;
}

static void button(SCState *s, bool down) {
    if (s->leftDown != down) output(s, SCLeftButton, 0, down, 0, 0);
    s->leftDown = down;
}

void SCInit(SCState *s, SCEmit emit, void *context) {
    memset(s, 0, sizeof(*s));
    s->emit = emit;
    s->context = context;
}

void SCCancel(SCState *s) {
    button(s, false);
    keys(s, 0);
    s->wheelRemainder = 0;
    s->mode = SCBlocked;
}

static void startDrag(SCState *s) {
    output(s, SCMove, 0, false, s->startX, s->startY);
    button(s, true);
    output(s, SCMove, 0, false, s->x, s->y);
    s->mode = SCSingleDrag;
}

void SCTick(SCState *s, double now) {
    if (s->mode == SCSinglePending && now - s->startedAt >= singleDelay) startDrag(s);
    if (s->mode == SCTapRelease && now >= s->releaseAt) {
        button(s, false);
        s->mode = SCIdle;
    }
    // No key repeats or sleeps: host holds the key, and stationary fingers stop the camera.
    if (s->keys && now - s->motionAt >= motionTimeout) keys(s, 0);
}

static double angleDelta(double angle, double previous) {
    return atan2(sin(angle - previous), cos(angle - previous));
}

void SCUpdate(SCState *s, const SCTouch *t, size_t count, double now) {
    if (count == 0) {
        if (s->mode == SCSinglePending) {
            startDrag(s);
            s->mode = SCTapRelease;
            s->releaseAt = now + 0.050; // let the game observe a short click across frames
        } else if (s->mode != SCTapRelease) {
            SCCancel(s);
            s->mode = SCIdle;
        }
        return;
    }
    if (s->mode == SCBlocked) return;
    if (count > 2) { SCCancel(s); return; }
    if (s->mode == SCTapRelease) { button(s, false); s->mode = SCIdle; }

    if (count == 1) {
        if (s->mode == SCIdle) {
            s->ids[0] = t[0].id;
            s->startX = s->x = t[0].x;
            s->startY = s->y = t[0].y;
            s->startedAt = now;
            s->mode = SCSinglePending;
        } else if ((s->mode != SCSinglePending && s->mode != SCSingleDrag) || s->ids[0] != t[0].id) {
            // Never turn the remaining camera finger into a road-building click.
            SCCancel(s);
            return;
        }
        s->x = t[0].x;
        s->y = t[0].y;
        if (s->mode == SCSingleDrag) output(s, SCMove, 0, false, s->x, s->y);
        else SCTick(s, now);
        return;
    }

    // Stable finger ordering is required (adapter sorts by identity, not position).
    double x = (t[0].x + t[1].x) / 2, y = (t[0].y + t[1].y) / 2;
    double distance = hypot(t[1].x - t[0].x, t[1].y - t[0].y);
    double angle = atan2(t[1].y - t[0].y, t[1].x - t[0].x);
    if (s->mode == SCIdle || s->mode == SCSinglePending || s->mode == SCSingleDrag) {
        button(s, false);
        keys(s, 0);
        s->ids[0] = t[0].id; s->ids[1] = t[1].id;
        s->x = x; s->y = y; s->distance = fmax(distance, 1); s->angle = angle;
        s->wheelRemainder = 0;
        s->mode = SCTwoPending;
        return;
    }
    if (s->ids[0] != t[0].id || s->ids[1] != t[1].id || distance < 1) {
        SCCancel(s);
        return;
    }
    double dx = x - s->x, dy = y - s->y;
    double scale = log(distance / s->distance);
    double rotation = angleDelta(angle, s->angle);
    if (s->mode == SCTwoPending) {
        double panScore = hypot(dx, dy) / 10;
        double pinchScore = fabs(scale) / 0.06;
        double rotationScore = fabs(rotation) / 0.10;
        if (fmax(panScore, fmax(pinchScore, rotationScore)) < 1) return;
        // Lock the dominant gesture until lift, avoiding accidental pan during pinch/rotation.
        if (pinchScore >= panScore && pinchScore >= rotationScore) s->mode = SCPinch;
        else if (rotationScore >= panScore) s->mode = SCRotate;
        else s->mode = SCPan;
    }
    if (s->mode == SCPan) {
        if (hypot(dx, dy) < 2) return; // accumulate sub-point updates at high refresh rates
        unsigned desired = 0;
        if (fabs(dx) >= fabs(dy) * 0.5) desired |= dx > 0 ? D : A;
        if (fabs(dy) >= fabs(dx) * 0.5) desired |= dy > 0 ? S : W;
        keys(s, desired);
        s->motionAt = now;
    } else if (s->mode == SCPinch) {
        s->wheelRemainder += scale;
        int clicks = (int)(s->wheelRemainder / wheelStep);
        if (clicks) {
            s->wheelRemainder -= clicks * wheelStep;
            // Bound sudden coordinate jumps to avoid an extreme zoom.
            clicks = clicks > 8 ? 8 : (clicks < -8 ? -8 : clicks);
            output(s, SCWheel, clicks, false, 0, 0);
        }
    } else if (s->mode == SCRotate) {
        if (fabs(rotation) < 0.025) return;
        keys(s, rotation > 0 ? E : Q); // UIKit Y increases downwards: positive is clockwise
        s->motionAt = now;
    }
    s->x = x; s->y = y; s->distance = distance; s->angle = angle;
}
