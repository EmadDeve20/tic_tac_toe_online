FROM gcc:latest as builder

RUN mkdir app

WORKDIR /app

COPY ./main.c .

RUN gcc main.c -o server


FROM debian

RUN mkdir /app/

WORKDIR /app/

COPY --from=builder /app/server .

CMD [ "./server" ]
