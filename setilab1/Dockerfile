# ---- Стадия сборки ----
FROM alpine:3.24.2 AS build
WORKDIR /app
COPY include ./include
COPY src ./src
RUN apk add --no-cache g++
RUN g++ -std=c++20 -O2 -pthread -Iinclude src/*.cpp -o setilab1


# ---- Стадия запуска ----
FROM alpine:3.24.2
WORKDIR /app
COPY --from=build /app/setilab1 /app/setilab1

RUN apk add --no-cache libstdc++ libgcc iproute2 iputils tini

ENTRYPOINT ["/sbin/tini", "--", "/app/setilab1"]
CMD ["239.255.0.1"]
