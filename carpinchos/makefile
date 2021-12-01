  
CC=gcc
SRC = $(wildcard ./*/*.c)
OUT = $(patsubst %.c,%.out,$(SRC))

$(OUT): %.out: %.c
	$(CC) -g3 -Wall -o $@ -L"../libmate" -I"../libmate/include" $^ -lmatelib -lpthread -lrt -Wl,-rpath,'$$ORIGIN/../../libmate'

.PHONY: simpleMutex simpleDeadlock mediumTermUsage

simpleMutex: ./simpleMutex/simpleMutex.out
	./simpleMutex/simpleMutex.out

simpleDeadlock: ./simpleDeadlock/simpleDeadlock.out
	./simpleDeadlock/simpleDeadlock.out

mediumTermUsage: ./mediumTermUsage/mediumTermUsage.out
	./mediumTermUsage/mediumTermUsage.out

memoryUsage: ./memoryUsage/memoryUsage.out
	./memoryUsage/memoryUsage.out