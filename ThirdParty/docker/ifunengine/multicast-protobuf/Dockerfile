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

RUN mkdir -p /home/test

WORKDIR /home/test
RUN funapi_initiator test
RUN test-source/setup_build_environment --type=makefile

WORKDIR /home/test/test-build/debug
RUN make

RUN apt-get install -y zookeeperd

#ADD account.ilf /etc/ifunfactory/account.ilf
ADD MANIFEST.json /home/test/test-source/src/

ADD entrypoint.sh /home/
RUN chmod +x /home/entrypoint.sh
RUN dos2unix /home/entrypoint.sh

ENTRYPOINT /home/entrypoint.sh
