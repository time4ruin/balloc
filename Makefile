NM=nm
CFLAGS=-g -Os -fPIC -fno-stack-protector -Wall -Wno-unused-result -U_FORTIFY_SOURCE

.PHONY: clean test

define run_test
    @echo "Running $(2) x $(3)"
	@for n in `seq 1 $(3)`; do cat $(2).bin; done | $(1) 2>$(2).out
endef

test: replay
	@echo "Imports (Do not use any that are not in whitelist!)"
	@$(NM) -u ./$< | sed -ne 's/\s*U \([^@]*\).*/\t\1/p'
	$(call run_test,./$<,test1,1)
	$(call run_test,./$<,test2,1)
	$(call run_test,./$<,test3,1)

replay: alloc.c main.c printf.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) replay test*.time test*.out
