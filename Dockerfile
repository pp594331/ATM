FROM gcc:latest

WORKDIR /app

COPY . .

RUN g++ main.cpp -o server

EXPOSE 8080

CMD ["./server"]
