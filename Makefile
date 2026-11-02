OUT      = cspcc
CXX      = g++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra -fdiagnostics-color=always
GNATMAKE = gnatmake
ODIR     = obj
INCDIR   = -Iinclude
JOBS     = $(shell nproc)

vpath %.cpp src src/cspcc

SRCS     = $(wildcard src/*.cpp src/cspcc/*.cpp)
OBJS     = $(patsubst %.cpp,$(ODIR)/%.o,$(notdir $(SRCS)))
HEADERS  = $(wildcard include/cspcc/*.h include/cspcc/data/*.h)
TESTS    = $(wildcard tests/lang/*.csp tests/errs/*.csp tests/examples/*.csp)
EXAMPLES = $(patsubst %.csp,%,$(wildcard examples/*.csp))

$(OUT): $(OBJS)
	$(CXX) -o $@ $^

$(ODIR)/%.o: %.cpp $(HEADERS) Makefile
	$(CXX) -c $(INCDIR) -o $@ $< $(CXXFLAGS)

test: $(OUT)
	@printf '%s\n' $(TESTS) | xargs -P $(JOBS) -n 1 tests/run_test | tee $(ODIR)/tests.log
	@echo "$$(grep -c PASS $(ODIR)/tests.log) passed, $$(grep -c FAIL $(ODIR)/tests.log) failed"
	@! grep -q FAIL $(ODIR)/tests.log

examples: $(EXAMPLES)

examples/%: examples/%.csp $(OUT)
	@mkdir -p $(ODIR)/examples/$*
	./$(OUT) -o $(ODIR)/examples/$*/$*.adb $<
	cd $(ODIR)/examples/$* && $(GNATMAKE) -q -gnat2022 $*.adb -o ../../../$@

.PHONY: test examples clean

clean:
	rm -f $(OBJS) $(OUT) $(EXAMPLES) $(ODIR)/tests.log
	rm -rf $(ODIR)/tests $(ODIR)/examples
