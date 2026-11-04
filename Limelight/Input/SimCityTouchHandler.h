// SPDX-License-Identifier: GPL-3.0-or-later
#import <UIKit/UIKit.h>

// Separate from the upstream Core Data schema; absence means the original touch mode.
static NSString *const SimCityTouchProfileKey = @"SimCityTouchProfileEnabled";

@class StreamView;
@interface SimCityTouchHandler : UIResponder
- (instancetype)initWithView:(StreamView *)view;
- (void)cancel;
@end
