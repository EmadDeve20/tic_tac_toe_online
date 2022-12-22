FROM gcc:latest as builder

RUN mkdir app

WORKDIR /app

COPY ./main.c .

RUN gcc main.c -o client


FROM debian

RUN mkdir /app/

WORKDIR /app/

COPY --from=builder /app/client .

CMD [ "./client" ]
