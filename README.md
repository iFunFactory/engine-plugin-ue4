Funapi plugin unreal engine 4
========================

이 플러그인은 iFun Engine 게임 서버를 사용하는 Unreal Engine 4 사용자를 위한 클라이언트 플러그인입니다.

# 기능

* TCP, UDP, HTTP 프로토콜 사용 가능
* JSON, Protobuf-net 형식의 메시지 타입 지원
* ChaCha20, AES-128을 포함한 4종류의 암호화 타입 지원
* 멀티캐스트, 채팅, 게임내 리소스 다운로드 등 다양한 기능 지원

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
``Plugins`` 폴더의 ``Funapi`` 폴더를 플러그인을 사용할 프로젝트의 ``Plugins`` 폴더로
복사하면 됩니다.

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

자세한 사용방법은 샘플과 도움말을 참고해 주세요.

# 도움말

클라이언트 플러그인의 도움말은 <https://www.ifunfactory.com/engine/documents/reference/ko/client-plugin.html> 를 참고해 주세요.

플러그인에 대한 궁금한 점은 <https://answers.ifunfactory.com> 에 질문을 올려주세요.
가능한 빠르게 답변해 드립니다.

그 외에 플러그인에 대한 문의 사항이나 버그 신고는 <funapi-support@ifunfactory.com> 으로 메일을
보내주세요.

# 버전별 주요 이슈

아래 설명의 버전보다 낮은 버전의 플러그인을 사용하고 있다면 아래 내용을 참고해 주세요.
괄호안은 지원하는 서버 버전입니다.

### v85 (2497 experimental)
TCP TLS

### v85 (2497 experimental)
Websocket Transport

### v73 (2368 experimental)
Dedicated Server Rpc

### v62 (2118 experimental, 2214 stable)
멀티캐스트 채널에 token 을 지정할 수 있는 기능 추가

### v61 (2109 experimental, 2214 stable)
Protobuf 의 Message Type 을 String 대신 Integer 를 쓸 수 있는 기능 추가
