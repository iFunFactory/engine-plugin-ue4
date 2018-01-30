FROM ubuntu:16.04
MAINTAINER JunHyun Park <junhyunpark@ifunfactory.com>

RUN apt-get update
RUN apt-get install -y wget apt-transport-https net-tools
RUN apt-get install -y gettext lsb dos2unix

WORKDIR /home/
RUN wget https://ifunfactory.com/engine/funapi-apt-setup.deb
RUN dpkg -i funapi-apt-setup.deb

RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y funapi1-dev

WORKDIR /home/test
RUN funapi_initiator test
ADD event_handlers.cc /home/test/test-source/src/event_handlers.cc
RUN test-source/setup_build_environment --type=makefile

WORKDIR /home/test/test-build/debug
RUN make

#ADD account.ilf /etc/ifunfactory/account.ilf

ADD MANIFEST.json /home/test/test-source/src/

CMD /home/test/test-build/debug/test-local -session_message_logging_level=2
