# Moonlight SimCity Touch Profile

## 사용 방법

Moonlight의 설정 → Touch Mode에서 **SimCity**를 선택하고 스트리밍을 시작한다.
설정은 앱 전체에 적용되며, 스트리밍을 다시 연결할 때 반영된다. 기본값은 기존
Moonlight 입력 방식이다. 원래의 Trackpad/Touchscreen 모드도 계속 선택할 수 있다.

| 입력 | 호스트에 보내는 동작 |
| --- | --- |
| 한 손가락 탭 | 해당 영상 좌표에서 좌클릭 |
| 한 손가락 드래그 | 시작 좌표에서 좌버튼을 누른 뒤 이동, 손을 떼면 해제 |
| 두 손가락 위/왼쪽/아래/오른쪽 드래그 | W/A/S/D, 대각선은 두 키 동시 입력 |
| 두 손가락 벌리기/오므리기 | 양/음의 마우스 휠 클릭 |
| 두 손가락 시계/반시계 회전 | E/Q |
| 세 손가락 | 카메라·마우스 입력 해제, 기존 키보드 표시/숨기기 |

- 한 손가락 입력을 120ms 판별한 뒤 드래그를 시작한다. 먼저 움직인 경로의
  시작점도 보존한다. 짧은 탭은 손을 뗄 때 클릭하고 50ms 후 버튼을 해제한다.
- 카메라 조작은 두 손가락을 가급적 동시에 대고 시작한다. 첫 손가락을 120ms
  이상 먼저 대면 이미 좌버튼이 눌릴 수 있다. 두 번째 손가락이 닿는 즉시 해제하지만,
  게임에 이미 전달된 도로 클릭까지 취소하는 것은 불가능하다.
- 두 손가락의 최초 이동에서 이동/핀치/회전 중 가장 뚜렷한 동작을 선택한다.
  선택은 두 손가락을 뗄 때까지 유지된다. 다른 동작으로 바꾸려면 손을 모두 떼고
  다시 시작한다. 회전과 핀치가 동시에 이동 키를 발생시키지 않도록 한 정책이다.
- 카메라 키는 움직이는 동안 유지하고, 마지막 유효 이동 후 120ms 안에 해제한다.
  손가락을 멈추면 카메라도 멈춘다. 좌우 방향은 손가락 이동 방향 기준이다.
- 두 손가락 중 하나를 떼거나 세 번째 손가락이 닿으면 모든 프로필 입력을 해제하고,
  남은 손가락도 모두 뗄 때까지 새 좌클릭을 억제한다.
- SimCity에서는 영상 자체의 이동/확대용 UIScrollView와 가상 게임패드를 사용하지 않는다.
  실제 마우스·키보드 입력 경로는 유지된다. 같은 키/좌버튼을 실제 주변기기와
  터치로 동시에 누르는 조합은 별도 입력 소유권 통합이 없으므로 피한다.
- 게임의 기본 WASD/Q/E/휠 바인딩이 전제다. 게임 단축키 설정이 다르면 먼저 맞춘다.
  Sunshine의 입력 허용과 게임 창 포커스도 필요하다.

## GitHub Actions 빌드

1. 이 변경을 포함한 `simcity-touch` 브랜치를 자신의 GitHub 포크에 push한다.
2. 포크의 Actions를 활성화하고 **SimCity unsigned iPad IPA**를 실행한다.
   해당 브랜치 push에서도 자동 실행된다. 기본 upstream CI와 별도 워크플로다.
3. 성공한 실행의 Artifacts에서 `Moonlight-SimCity-unsigned-<번호>` ZIP을 받는다.
4. ZIP을 풀면 `Moonlight-SimCity-unsigned.ipa`, SHA-256 파일, 빌드 JSON과 이 설명서가 나온다.
   Sideloadly에 넣을 파일은 아티팩트 ZIP이 아닌 **그 안의 .ipa**다.

워크플로는 실제 저장소의 `Moonlight.xcodeproj`, iOS `Moonlight` 스킴을 사용한다.
`iphoneos`, `generic/platform=iOS`, `ARCHS=arm64`, Release로 빌드한다.
서브모듈은 재귀 체크아웃하고 OpenSSL은 저장소의 Package.resolved 버전을 사용한다.
Mac runner에는 iPhoneOS SDK가 필요하며, SDK/Xcode 버전은 빌드 증거에 기록된다.
최소 OS는 15.0이다. 기기의 OS와 SDK 버전이 같을 필요는 없지만, iPadOS 26.6.2에서의
실행 및 입력 호환성은 실제 설치 후 확인해야 한다.

### unsigned IPA 조건과 제한

- `CODE_SIGNING_ALLOWED=NO`, `CODE_SIGNING_REQUIRED=NO`, 빈 Team/Identity로 빌드한다.
  인증서, Apple 계정, 프로비저닝 프로파일 또는 GitHub signing secret이 필요 없다.
- `xcodebuild -exportArchive` 대신 빌드된 앱을 `Payload/Moonlight.app` 형태로 압축한다.
  exportArchive의 서명·배포 프로비저닝 과정을 요구하지 않는다.
- 패키징 스크립트는 iPhoneOS plist, iPad device family, arm64 아키텍처와 Mach-O의
  IOS 플랫폼을 검사한다. Apple Silicon의 arm64 시뮬레이터 빌드도 거부한다.
- Mach-O의 서명과 embedded.mobileprovision을 제거하고 ZIP 구조·무결성을 확인한다.
  빌드 manifest와 SHA-256을 함께 제공한다.
- unsigned IPA는 iPad에 바로 설치할 수 없다. Windows Sideloadly가 사용자의
  Apple 계정으로 다시 서명해야 한다. **Mac과 유료 개발자 계정은 필요 없지만,
  서명에 사용할 무료 Apple 계정과 Apple 인증 서버 연결은 필요하다.**
- iOS 앱 ID는 `io.github.im-siyoun.moonlight.simcity`, 표시 이름은 `Moonlight SimCity`다.
  공식 앱의 `com.moonlight-stream.Moonlight`와 다르므로 나란히 설치할 수 있다.
  Sideloadly가 계정에 맞는 앱 ID를 요구하면 고유한 접두사로 바꾸되 공식 앱 ID는 쓰지 않는다.
  재설치/갱신 때는 같은 Apple 계정과 같은 최종 앱 ID를 유지한다.
- 무료 계정의 서명은 통상 7일 후 만료하며 재서명이 필요하다. 무료 계정에는
  동시 설치 앱 수 등의 제한도 있다. 해당 iPadOS 버전과 Apple 서버 정책에서
  실제 설치가 성공하는지는 Sideloadly 로그로 확인한다.

## Windows 11에서 Sideloadly 설치

1. [Sideloadly 공식 사이트](https://sideloadly.io/)에서 Windows 버전을 설치한다.
   공식 안내에 따라 Microsoft Store판 대신 웹 배포판 iTunes와 iCloud를 설치한다.
2. iPad를 USB로 연결하고 잠금을 해제한 뒤 ‘이 컴퓨터를 신뢰’를 승인한다.
3. Sideloadly에서 iPad를 선택하고 `.ipa`를 드래그한다. 무료 Apple 계정을 입력하고
   Apple ID Sideload 방식으로 Start를 누른다. 요구되는 인증/2FA는 Windows에서만 처리한다.
4. iPad의 설정 → 일반 → VPN 및 기기 관리에서 개발자 앱을 신뢰한다.
5. 설정 → 개인정보 보호 및 보안 → 개발자 모드를 켜고 재시동·확인을 진행한다.
   개발자 모드 메뉴가 처음에 없으면 먼저 설치를 시도한 뒤 다시 확인한다.
6. `Moonlight SimCity` 실행 후 로컬 네트워크 접근을 허용하고 Sunshine과 새로 페어링한다.
   공식 앱과 앱 저장 영역이 다르므로 기존 앱의 PC 페어링/설정은 자동 공유되지 않는다.
7. SimCity 프로필을 선택하고 게임을 연결한다. 무료 서명이 만료되기 전에 같은 ID로
   재설치하거나 Sideloadly의 자동 갱신을 설정한다.

Apple 계정 정보/암호, 인증 코드, 인증서, 개인키를 GitHub 저장소나 Actions secrets에
넣지 않는다. 이 빌드 작업에는 필요 없다.

공식 참고: [Sideloadly 설치·무료 계정 FAQ](https://sideloadly.io/faq),
[Apple 무료 계정과 7일 프로파일 제한](https://developer.apple.com/help/account/basics/about-your-developer-account/),
[Apple 개발자 모드](https://developer.apple.com/documentation/xcode/enabling-developer-mode-on-a-device).

## 검증 범위와 실제 기기 체크리스트

`bash BuildScripts/test-simcity-touch.sh`는 플랫폼 독립 C 코어를 ASan/UBSan과 함께 실행한다.
탭·드래그, 1→2→1 전환, 세 손가락 억제, WASD 대각/반전/정지, 휠 양방향,
Q/E 반전·각도 경계, 손가락 ID 교체, 모든 상태의 취소/중복 취소를 검사한다.
이 테스트는 UIKit 이벤트 전달, Sunshine 수신, 게임의 반응을 검증하지 않는다.
macOS 워크플로의 컴파일·IPA 패키징 성공도 실제 iPad 조작 성공과 별개다.

실제 iPad Pro 5세대 / iPadOS 26.6.2 / Windows Sunshine에서 다음을 기록한다.
아래는 **미실행 항목**이며 완료를 의미하지 않는다.

- [ ] Sideloadly 서명·설치, 개발자 신뢰, 앱 실행, 공식 Moonlight와 동시 설치
- [ ] Sunshine 페어링, 영상·오디오, 게임 창 포커스
- [ ] 탭으로 메뉴/건물 선택, 한 손가락으로 도로 연속 건설, 시작점 오차 확인
- [ ] 2손가락 상하좌우/대각 이동, 정지 시 카메라 정지, 방향 반전
- [ ] 핀치 양방향 확대·축소, 영상 프레임/배율 자체는 고정
- [ ] Q/E 양방향 회전, 180도 경계 회전, 핀치 중 이동/회전 오작동 없음
- [ ] 도로 드래그 중 2번째 손가락 추가 시 좌버튼 해제
- [ ] 카메라 조작 중 한 손가락만 떼도 멈춤, 남은 손가락으로 도로가 생기지 않음
- [ ] 세 손가락 키보드, 제어센터, 앱 전환, 화면 잠금, 화면 방향 변경 중 입력 해제
- [ ] 네트워크 단절 후 재접속 시 키/버튼이 남지 않음
- [ ] Trackpad/Touchscreen으로 되돌린 뒤 기존 입력·영상 확대 동작 확인

정상 종료/비활성화에서는 연결이 살아 있을 때 해제를 보낸다. 연결이 이미 끊어진
상황에서는 전송 성공을 보장할 수 없으며 Sunshine의 연결 종료 입력 정리도 필요하다.
재접속 시 클라이언트의 이전 터치 상태는 재사용하지 않는다.
