FROM ubuntu:16.04
MAINTAINER JunHyun Park <junhyunpark@ifunfactory.com>

RUN apt-get update
RUN apt-get install -y wget apt-transport-https net-tools gettext

ADD ifunfactory.gpg.key /root/
RUN apt-key add /root/ifunfactory.gpg.key

ADD ifunfactory.list /etc/apt/sources.list.d/

RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y funapi1-dev

RUN mkdir -p /home/test

WORKDIR /home/test
RUN funapi_initiator test
RUN test-source/setup_build_environment --type=makefile

WORKDIR /home/test/test-build/debug
RUN make

#ADD account.ilf /etc/ifunfactory/account.ilf

ADD MANIFEST.json /home/test/test-source/src/

CMD /home/test/test-build/debug/test-local -session_message_logging_level=2
