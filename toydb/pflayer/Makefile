
CC = gcc
CFLAGS = -std=gnu89

SRC = buf.c hash.c pf.c slotted.c
OBJ = buf.o hash.o pf.o slotted.o
HDR = pftypes.h pf.h slotted.h

pflayer.o: $(OBJ)
	ld -r -o pflayer.o $(OBJ)
tests: testhash testpf test_slotted_pf

testpf: testpf.o pflayer.o
	$(CC) -o testpf testpf.o pflayer.o

testhash: testhash.o pflayer.o
	$(CC) -o testhash testhash.o pflayer.o

test_slotted_pf: test_slotted_pf.o pflayer.o
	$(CC) -o test_slotted_pf test_slotted_pf.o pflayer.o

$(OBJ): $(HDR)

testhash.o: $(HDR)

testpf.o: $(HDR)

test_slotted_pf.o: $(HDR)

lint:
	lint $(SRC)

install: pflayer.o
