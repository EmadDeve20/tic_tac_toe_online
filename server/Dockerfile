FROM gcc:latest as builder

RUN mkdir app

WORKDIR /app

COPY ./main.c .

RUN gcc main.c


FROM debian

RUN mkdir /app/

WORKDIR /app/

COPY --from=builder /app/a.out .

CMD [ "./a.out" ]
