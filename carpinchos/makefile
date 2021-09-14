CC=gcc
SRC = $(wildcard ./*/*.c)
OUT = $(patsubst %.c,%.out,$(SRC))

$(OUT): %.out: %.c
	$(CC) -g3 -Wall -o $@ -L"../libmate" -I"../libmate/include" $^ -lmate -lpthread -lrt -Wl,-rpath,'$$ORIGIN/../../libmate'

.PHONY: simpleMutex

simpleMutex: ./simpleMutex/simpleMutex.out
	./simpleMutex/simpleMutex.out