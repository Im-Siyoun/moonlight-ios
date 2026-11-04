// SPDX-License-Identifier: GPL-3.0-or-later
#import "SimCityTouchHandler.h"
#import "SimCityTouchCore.h"
#import "StreamView.h"
#include <Limelight.h>
#include <QuartzCore/QuartzCore.h>

@interface SimCityTouchHandler ()
- (void)emit:(SCEvent)event;
@end

static void sendEvent(void *context, SCEvent event) {
    [(__bridge SimCityTouchHandler *)context emit:event];
}

@implementation SimCityTouchHandler {
    __weak StreamView *_view;
    NSMutableSet<UITouch *> *_touches;
    NSTimer *_timer;
    SCState _state;
}

- (instancetype)initWithView:(StreamView *)view {
    if ((self = [super init])) {
        _view = view;
        _touches = [NSMutableSet set];
        SCInit(&_state, sendEvent, (__bridge void *)self);
        __weak SimCityTouchHandler *weakSelf = self;
        _timer = [NSTimer timerWithTimeInterval:1.0 / 60 repeats:YES block:^(NSTimer *timer) {
            SimCityTouchHandler *handler = weakSelf;
            if (handler) SCTick(&handler->_state, CACurrentMediaTime());
        }];
        [[NSRunLoop mainRunLoop] addTimer:_timer forMode:NSRunLoopCommonModes];
    }
    return self;
}

- (void)emit:(SCEvent)event {
    switch (event.type) {
        case SCMove: [_view updateCursorLocation:CGPointMake(event.x, event.y) isMouse:NO]; break;
        case SCLeftButton: LiSendMouseButtonEvent(event.down ? BUTTON_ACTION_PRESS : BUTTON_ACTION_RELEASE, BUTTON_LEFT); break;
        case SCKey: LiSendKeyboardEvent(0x8000 | event.code, event.down ? KEY_ACTION_DOWN : KEY_ACTION_UP, 0); break;
        case SCWheel: LiSendScrollEvent((signed char)event.code); break;
    }
}

- (void)update {
    NSArray<UITouch *> *ordered = [[_touches allObjects] sortedArrayUsingComparator:^NSComparisonResult(UITouch *a, UITouch *b) {
        uintptr_t first = (uintptr_t)(__bridge void *)a, second = (uintptr_t)(__bridge void *)b;
        return first < second ? NSOrderedAscending : (first > second ? NSOrderedDescending : NSOrderedSame);
    }];
    SCTouch points[2] = {0};
    for (NSUInteger i = 0; i < MIN(ordered.count, 2); i++) {
        CGPoint p = [ordered[i] locationInView:_view];
        points[i] = (SCTouch){(uintptr_t)(__bridge void *)ordered[i], p.x, p.y};
    }
    SCUpdate(&_state, points, ordered.count, CACurrentMediaTime());
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        if (touch.type == UITouchTypeDirect) [_touches addObject:touch];
    }
    [self update];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event { [self update]; }

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    // Capture the final position before removing ended contacts.
    [self update];
    [_touches minusSet:touches];
    [self update];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    SCCancel(&_state);
    [_touches minusSet:touches];
    [self update];
}

- (void)cancel {
    SCCancel(&_state);
    [_touches removeAllObjects];
    SCUpdate(&_state, NULL, 0, CACurrentMediaTime());
}

- (void)dealloc {
    [_timer invalidate];
    SCCancel(&_state);
}
@end
