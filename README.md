Funapi plugin unreal engine 4
========================

이 플러그인은 iFun Engine 게임 서버를 사용하는 Unreal Engine 4 사용자를 위한 클라이언트 플러그인입니다.

# 기능

* TCP, UDP, HTTP 프로토콜 사용 가능
* JSON, Protobuf-net 형식의 메시지 타입 지원
* ChaCha20, AES-128을 포함한 4종류의 암호화 타입 지원
* 멀티캐스트, 채팅, 게임내 리소스 다운로드 등 다양한 기능 지원

# 언리언 엔진 지원 버전에 대해서

Funapi plugin 은 다음 버전의 Unreal engine 4 를 지원합니다.

| Unreal engine 4 버전 | Windows |  macOS  | Android |   iOS  |
|:--------------------|:-------:|:-------:|:-------:|--------:|
| 4.19   | ▲ | o | o | o |
| 4.20   | o | o | o | o |
| 4.21   | o | o | o | o |
| 4.22   | o | o | o | o |
| 4.23   | ▲ | ▲ | ▲ | ▲ |

* **4.19 버전** : Windows 환경에서 패키징이 안 되는 문제가 있어 확인중에 있습니다.
* **4.23 버전** : ``FunapiDedicatedServer`` 플러그인 모듈에서 사용하는 ``DEPRECATED`` 매크로가
  ``UE_DEPRECATED`` 로 변경되어 경고가 발생 중이지만 컴파일 및 동작에는 문제가 없습니다.
  추후 플러그인 업데이트로 수정될 예정입니다.

**지원 엔진 버전 이외의 플러그인 동작은 컴파일 및 패키징 단계에서 오류가 발생할수 있습니다.**

# 다운로드

**git clone** 으로 다운 받거나 **zip 파일** 을 다운 받아 압축을 풀고 사용하시면 됩니다.

클라이언트 플러그인 코드는 ``Plugins`` 폴더의 ``Funapi``에 있습니다.

# 테스트 프로젝트

``funapi_plugin_ue4.uproject`` 파일을 열면 됩니다.

테스트 코드는 ``funapi_tester.cpp`` 와 ``funapi_tester.h`` 파일에 있습니다.

서버 주소가 로컬로 되어 있으니 서버가 로컬에 있지 않다면
*Server Address* 값을 수정해 주세요.

서버를 띄우고 실행을 하면 여러가지 기능들을 테스트해볼 수 있습니다.

서버를 설치하고 아무것도 변경하지 않았다면 기본적으로 TCP, HTTP의 JSON 포트만 열려 있습니다.
다른 프로토콜과 메시지 타입을 사용하려면 서버와 클라이언트의 포트 번호를 맞춰서 변경하고 테스트하면 됩니다.

# 내 프로젝트에 적용

1. 파일 복사

``Plugins`` 폴더의 ``Funapi`` 폴더를 플러그인을 사용할 프로젝트의 ``Plugins`` 폴더로
복사하면 됩니다.

엔진 하위 디렉토리가 아니라 게임 프로젝트의 하위 디렉토리에 복사해야만 합니다.

2. uproject 설정 추가

**uproject** 파일에서 플러그인을 사용하도록 설정해야 합니다.

```json
{
    "Plugins": [
        {
            "Name": "Funapi",
            "Enabled": true
        }
    ]
}
```

언리얼 에디터의 메뉴에서 **Edit -> Plugins** 를 선택하고 ``Funapi`` 플러그인을 **Enbled** 체크해도 됩니다.

3. 게임 프로젝트의 Build.cs 파일에 플러그인 내용 추가

게임 프로젝트의 **Build.cs** 파일에서 플러그인을 사용하도록 설정해야 합니다.

```csharp
PrivateDependencyModuleNames.AddRange(new string[] { "Funapi" });
```

자세한 사용방법은 샘플 프로젝트를 참고해 주세요.

# 언리얼 엔진과 관련한 알려진 문제

언리얼 엔진의 제약 사항이나 아직 수정되지 않은 문제로 인해서 Funapi 플러그인 사용 시 발생할 수 있는 문제들 대해서 아래 매뉴얼 페이지 링크를 참고 해 주시기 바랍니다.

## 버전 4.21 : .cc 파일 컴파일 오류
https://www.ifunfactory.com/engine/documents/reference/ko/client-plugin.html#cc

## 버전 4.22 이전: iPhone XS 에서 크래시하는 문제
https://www.ifunfactory.com/engine/documents/reference/ko/client-plugin.html#iphone-xs

# 도움말

클라이언트 플러그인의 도움말은 <https://www.ifunfactory.com/engine/documents/reference/ko/client-plugin.html> 를 참고해 주세요.

플러그인에 대한 궁금한 점은 <https://answers.ifunfactory.com> 에 질문을 올려주세요.
가능한 빠르게 답변해 드립니다.

그 외에 플러그인에 대한 문의 사항이나 버그 신고는 <funapi-support@ifunfactory.com> 으로 메일을
보내주세요.

# 지원 예정인 기능들

아래 기능들은 서버에서 지원하지만 아직 플러그인에서는 지원하지 않는 기능들입니다.
이후 플러그인 업데이트로 지원될 예정이며 업데이트에 대한 내용은
[릴리즈 노트](https://github.com/iFunFactory/engine-plugin-ue4/releases)를 참고해 주세요.

### 멀티캐스트에서 WebSocket 프로토콜을 지원
아이펀 엔진 1.0.0-4392 Stable 버전에서 추가된 기능으로 플러그인은 TCP 프로토콜만 지원하지만 추후 업데이트로 WebSocket 프로토콜을 지원.
