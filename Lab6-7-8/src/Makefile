all: bin/manager bin/worker

bin/manager: src/manager.c
	gcc src/manager.c src/log/log.c -I/opt/homebrew/Cellar/czmq/4.2.1/include -L/opt/homebrew/Cellar/czmq/4.2.1/lib -L/opt/homebrew/Cellar/zeromq/4.3.4/lib -lczmq -lzmq -o bin/manager

bin/worker: src/worker.c
	gcc src/worker.c src/log/log.c -I/opt/homebrew/Cellar/czmq/4.2.1/include -L/opt/homebrew/Cellar/czmq/4.2.1/lib -L/opt/homebrew/Cellar/zeromq/4.3.4/lib -lczmq -lzmq -o bin/worker

clean:
	rm -r bin/*

gcc src/worker.c  -L -lzmq -o bin/worker
