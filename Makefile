OUT      = cspcc
CXX      = g++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra -fdiagnostics-color=always
GNATMAKE = gnatmake
ODIR     = obj
SDIR     = src
TDIR     = tests
INCDIR   = -Iinclude
LIBS     = -lCatch2Main -lCatch2

_OBJS    = diagnostics.o lexer.o ast.o parser.o checker.o emitter.o runtime.o options.o
OBJS     = $(patsubst %,$(ODIR)/%,$(_OBJS))
_TOBJS   = test_lexer.o test_parser.o test_checker.o test_emitter.o test_programs.o test_examples.o
TOBJS    = $(patsubst %,$(ODIR)/%,$(_TOBJS))
HEADERS  = $(wildcard include/*.h)
EXAMPLES = $(patsubst %.csp,%,$(wildcard examples/*.csp))

$(OUT): $(OBJS) $(ODIR)/main.o
	$(CXX) -o $@ $^

$(ODIR)/%.o: $(SDIR)/%.cpp $(HEADERS) Makefile
	$(CXX) -c $(INCDIR) -o $@ $< $(CXXFLAGS)

$(ODIR)/test_%.o: $(TDIR)/test_%.cpp $(TDIR)/test.h $(HEADERS) Makefile
	$(CXX) -c $(INCDIR) -o $@ $< $(CXXFLAGS)

$(ODIR)/test_cspcc: $(TOBJS) $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

test: $(ODIR)/test_cspcc
	./$(ODIR)/test_cspcc

examples: $(EXAMPLES)

examples/%: examples/%.csp $(OUT)
	@mkdir -p $(ODIR)/examples/$*
	./$(OUT) -o $(ODIR)/examples/$*/$*.adb $<
	cd $(ODIR)/examples/$* && $(GNATMAKE) -q -gnat2022 $*.adb -o ../../../$@

.PHONY: test examples clean

clean:
	rm -f $(OBJS) $(TOBJS) $(ODIR)/main.o $(ODIR)/test_cspcc $(OUT) $(EXAMPLES)
	rm -rf $(ODIR)/tests $(ODIR)/examples
