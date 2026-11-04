# Repository analysis and implementation plan

Analyzed upstream `moonlight-stream/moonlight-ios` master at
`02dc9780496eeeac6d01c8bbdccb8b6fe71ef28a` (2026-09-26).

## Verified upstream structure

| Concern | Existing source |
| --- | --- |
| Project and iOS/tvOS targets | `Moonlight.xcodeproj/project.pbxproj`: Moonlight / Moonlight TV |
| Existing CI and scheme names | `.github/workflows/ci.yml`: Moonlight + iphonesimulator, Moonlight TV + appletvsimulator, Debug/Release |
| Touch dispatch, video coordinate conversion | `Limelight/Input/StreamView.m` |
| Existing touch handlers | `Limelight/Input/AbsoluteTouchHandler.m`, `RelativeTouchHandler.m` |
| Local video pan/zoom | `Limelight/ViewControllers/StreamFrameViewController.m`: absolute-mode UIScrollView |
| Settings UI | `Limelight/ViewControllers/SettingsViewController.m`, iPad/iPhone storyboards |
| Host transport API | `moonlight-common/moonlight-common-c/src/Limelight.h` and InputStream.c |
| Dependencies | recursive X1Kit/common-c/enet/nanors submodules, checked-in opus/SDL2/FFmpeg libraries, OpenSSL Swift package |

There is no checked-in shared xcscheme; upstream CI uses Xcode's auto-generated target
schemes. The new workflow records `xcodebuild -list` and builds that same Moonlight scheme.
No new scheme name is assumed. The upstream CI builds simulators and does not package an IPA.
Upstream signing settings contain the maintainer team ID; this fork clears it.

## Changes

1. Add a third touch-mode segment in code so both existing storyboards get the same choice.
   Store opt-in via NSUserDefaults, preserving the upstream Core Data schema and defaults.
2. Select an iOS-only SimCityTouchHandler in StreamView. It tracks direct finger identities,
   feeds monotonic samples into a C state machine, and uses the existing video coordinate mapper.
3. Use LiSendMouseButtonEvent, LiSendMousePositionEvent (via StreamView),
   LiSendKeyboardEvent with `0x8000 | VK`, and LiSendScrollEvent. No protocol fork is needed.
4. Bypass the local UIScrollView in SimCity mode, suppress on-screen controller input, and
   retain existing keyboard/pointer paths and the three-finger keyboard shortcut.
5. Release profile-owned inputs on finger-count changes, cancellation, inactivity, stream exit,
   teardown and connection termination. Gate profile input until connected/active. Never
   resume road construction using a leftover camera finger.
6. Use distinct iOS bundle/display names and an unsigned device build/package workflow.
   Preserve the upstream tvOS input implementation and target bundle ID.

## State machine

`Idle → SinglePending → SingleDrag → Idle` handles roads. A short lift from SinglePending
emits a click and enters TapRelease for 50ms. A second finger releases the button and enters
TwoPending. Translation/scale/rotation thresholds select Pan/Pinch/Rotate exclusively.
Pan and Rotate release their keys after 120ms without meaningful movement. A finger lift,
third finger, cancellation or identity replacement enters Blocked and releases owned inputs;
only zero contacts returns to Idle. TapRelease cannot create a delayed new press.

The 120ms single-finger window trades a small road-start delay for fewer accidental clicks
at the start of two-finger gestures. Once a click has reached the host it cannot be retracted.
Camera gesture types lock until lift; simultaneous compound gestures are intentionally excluded.

## Validation boundaries

Portable tests exercise the production C core, including balanced input edges and zero
held keys/buttons after interruptions. CI also builds the UIKit adapter as part of the actual
iOS app, checks real-device Mach-O platform/architecture, removes signatures, and validates
the resulting ZIP. These checks cannot prove physical iPad gesture quality or game behavior.
See the Korean installation guide for the device test checklist.
