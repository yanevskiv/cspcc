BDIR     = build
ODIR     = $(BDIR)/objs
SDIR     = src
TDIR     = tests
OUT      = $(BDIR)/cspcc
CXX      = g++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra -fdiagnostics-color=always
GNATMAKE = gnatmake
INCDIR   = -Iinclude
LIBS     = -lCatch2Main -lCatch2

OBJS     = $(patsubst $(SDIR)/%.cpp,$(ODIR)/%.o,$(wildcard $(SDIR)/*.cpp))
TOBJS    = $(patsubst $(TDIR)/%.cpp,$(ODIR)/%.o,$(wildcard $(TDIR)/*.cpp))
HEADERS  = $(wildcard include/*.h)
EXAMPLES = $(patsubst examples/%.csp,$(BDIR)/examples/%,$(wildcard examples/*.csp))

$(OUT): $(OBJS)
	$(CXX) -o $@ $^

$(ODIR)/%.o: $(SDIR)/%.cpp $(HEADERS) Makefile
	@mkdir -p $(@D)
	$(CXX) -c $(INCDIR) -o $@ $< $(CXXFLAGS)

$(ODIR)/test_%.o: $(TDIR)/test_%.cpp $(TDIR)/test.h $(HEADERS) Makefile
	@mkdir -p $(@D)
	$(CXX) -c $(INCDIR) -o $@ $< $(CXXFLAGS)

$(BDIR)/test_cspcc: $(TOBJS) $(filter-out $(ODIR)/main.o,$(OBJS))
	$(CXX) -o $@ $^ $(LIBS)

test: $(BDIR)/test_cspcc
	./$(BDIR)/test_cspcc

examples: $(EXAMPLES)

$(BDIR)/examples/%: examples/%.csp $(OUT)
	@mkdir -p $(@D) $(ODIR)/examples/$*
	$(OUT) -o $(ODIR)/examples/$*/$*.adb $<
	cd $(ODIR)/examples/$* && $(GNATMAKE) -q -gnat2022 $*.adb -o $(CURDIR)/$@

.PHONY: test examples clean

clean:
	rm -rf $(BDIR)
