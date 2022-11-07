


FROM java:8


MAINTAINER ljystu

ENV LANG=C.UTF-8 LC_ALL=C.UTF-8

VOLUME /tmp

ADD target/*.jar payment.jar

ENTRYPOINT ["java","-jar","/app.jar"]

EXPOSE 8083

