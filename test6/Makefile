.PHONY: test clean

test-custom-write: test-custom-write.c ../mustach-json-c.h ../mustach-json-c.c ../mustach-wrap.c ../mustach.h ../mustach.c
	@echo building test-custom-write
	$(CC) $(CFLAGS) $(LDFLAGS) -g -o test-custom-write test-custom-write.c  ../mustach.c  ../mustach-json-c.c ../mustach-wrap.c -ljson-c

test: test-custom-write
	@echo starting test
	@valgrind ./test-custom-write json -U must -l must -x must > resu.last 2> vg.last
	@sed -i 's:^==[0-9]*== ::' vg.last
	@diff -w resu.ref resu.last && echo "result ok" || echo "ERROR! Result differs"
	@diff -wq vg.ref vg.last && echo "memory ok" || echo "memory differs"
	@echo

clean:
	rm -f resu.last vg.last test-custom-write

