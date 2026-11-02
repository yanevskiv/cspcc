// Tests that build and run every program in examples/.
// Copyright (C) 2026 Ivan Janevski
// SPDX-License-Identifier: GPL-3.0-or-later

#include "test.h"

#include <map>
#include <regex>

// Check, from --trace output, that no two hooks named hook run at once.
static std::string mutex_problem(const std::string &output, const std::string &hook = "work()")
{
    int inside = 0;
    int runs = 0;
    for (const std::string &line : lines(output)) {
        if (contains(line, ": " + hook + " end")) {
            inside--;
        } else if (contains(line, ": " + hook)) {
            runs++;
            if (++inside > 1) {
                return "overlap: " + line;
            }
        }
    }
    return runs ? "" : "no " + hook + " ran";
}

// Check, from --trace output, that neighbors never eat at once and all five eat.
static std::string philosophers_problem(const std::string &output)
{
    std::regex eat(R"(^Phil\((\d)\): EAT( end)?$)");
    bool eating[5] = {};
    int meals[5] = {};
    for (const std::string &line : lines(output)) {
        std::smatch match;
        if (!std::regex_match(line, match, eat)) {
            continue;
        }
        int p = std::stoi(match[1]);
        if (match[2].matched) {
            eating[p] = false;
            continue;
        }
        if (eating[(p + 4) % 5] || eating[(p + 1) % 5]) {
            return "neighbors eat: " + line;
        }
        eating[p] = true;
        meals[p]++;
    }
    for (int p = 0; p < 5; p++) {
        if (!meals[p]) {
            return "Phil(" + std::to_string(p) + ") never ate";
        }
    }
    return "";
}

// Check that each portion producer i made, i * 100 + k, was consumed once.
static std::string producer_consumer_problem(const std::string &output)
{
    std::regex consume(R"(^Consumer\(\d+\): consume\((\d+)\)$)");
    std::vector<int> consumed;
    for (const std::string &line : lines(output)) {
        std::smatch match;
        if (std::regex_match(line, match, consume)) {
            consumed.push_back(std::stoi(match[1]));
        }
    }
    std::sort(consumed.begin(), consumed.end());
    std::vector<int> expected;
    for (int p = 1; p <= 5; p++) {
        for (int k = 0; k < 6; k++) {
            expected.push_back(p * 100 + k);
        }
    }
    return consumed == expected ? "" : "the portions consumed differ from those produced";
}

// Check, from --trace output, that a writer writes alone and readers read together.
static std::string readers_writers_problem(const std::string &output)
{
    int readers = 0;
    int writers = 0;
    int reads = 0;
    for (const std::string &line : lines(output)) {
        if (contains(line, ": READING end")) {
            readers--;
        } else if (contains(line, ": WRITING end")) {
            writers--;
        } else if (contains(line, ": READING")) {
            readers++;
            reads++;
            if (writers > 0) {
                return "read while writing: " + line;
            }
        } else if (contains(line, ": WRITING")) {
            if (++writers > 1 || readers > 0) {
                return "write not alone: " + line;
            }
        }
    }
    return reads ? "" : "nobody read";
}

// Check, from --trace output, that Santa rewards all reindeer after delivering toys and
// dismisses elves three at a time after helping them.
static std::string santa_claus_problem(const std::string &output)
{
    std::string want;
    int left = 0;
    int deliveries = 0;
    int helps = 0;
    for (const std::string &line : lines(output)) {
        if (line == "Santa: DELIVER_TOYS end") {
            want = "reward()";
            left = 9;
            deliveries++;
        } else if (line == "Santa: HELP_ELVES end") {
            want = "done()";
            left = 3;
            helps++;
        } else if (line.starts_with("Santa: ")) {
            if (left) {
                return "unfinished round before: " + line;
            }
        } else if (line.starts_with("Santa -> ")) {
            if (!left || !contains(line, want)) {
                return "unexpected: " + line;
            }
            left--;
        }
    }
    if (!deliveries || !helps) {
        return "Santa delivered " + std::to_string(deliveries) + " times and helped " +
               std::to_string(helps) + " times";
    }
    return "";
}

TEST_CASE("examples/copy.csp", "[examples]")
{
    Run result = run("copy", example("copy"), "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "east: print('h')", "east: print('e')", "east: print('l')",
        "east: print('l')", "east: print('o')",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("examples/factorial.csp", "[examples]")
{
    Run result = run("factorial", example("factorial"), "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "fac(0): print(120)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("examples/factorial.csp with -DN=10 -DLIMIT=11", "[examples]")
{
    Run result =
        run("factorial_define", example("factorial"), "--delay=0", {{"N", "10"}, {"LIMIT", "11"}});
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "fac(0): print(3628800)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("examples/factorial.csp with too short a chain", "[examples]")
{
    Run result =
        run("factorial_limit", example("factorial"), "--delay=0", {{"N", "12"}, {"LIMIT", "4"}});
    CHECK(result.m_status == 1);
    CHECK(contains(result.m_error, "no process fac(5)"));
}

TEST_CASE("examples/integral.csp", "[examples]")
{
    Run result = run("integral", example("integral"), "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "Bag: print(0.5)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("examples/matrix.csp", "[examples]")
{
    Run result = run("matrix", example("matrix"), "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "M(4,1): print(42)", "M(4,1): print(54)", "M(4,2): print(51)",
        "M(4,2): print(66)", "M(4,3): print(60)", "M(4,3): print(78)",
    };
    CHECK(sorted_lines(result.m_output) == expected);
}

TEST_CASE("examples/philosophers.csp", "[examples]")
{
    Run result = run("philosophers", example("philosophers"), "--delay=2 --trace --timeout=2");
    CHECK(result.m_status == 0);
    CHECK(philosophers_problem(result.m_output) == "");
}

TEST_CASE("examples/producer_consumer.csp", "[examples]")
{
    Run result = run("producer_consumer", example("producer_consumer"), "--delay=0");
    CHECK(result.m_status == 0);
    CHECK(producer_consumer_problem(result.m_output) == "");
}

TEST_CASE("examples/readers_writers.csp", "[examples]")
{
    Run result =
        run("readers_writers", example("readers_writers"), "--delay=2 --trace --timeout=2");
    CHECK(result.m_status == 0);
    CHECK(readers_writers_problem(result.m_output) == "");
}

TEST_CASE("examples/santa_claus.csp", "[examples]")
{
    Run result = run("santa_claus", example("santa_claus"), "--delay=2 --trace --timeout=2");
    CHECK(result.m_status == 0);
    CHECK(santa_claus_problem(result.m_output) == "");
}

TEST_CASE("examples/semaphore.csp", "[examples]")
{
    Run result = run("semaphore", example("semaphore"), "--delay=1 --trace");
    CHECK(result.m_status == 0);
    CHECK(mutex_problem(result.m_output) == "");
}

TEST_CASE("examples/set.csp", "[examples]")
{
    Run result = run("set", example("set"), "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: print(3, true)",
        "X: print(4, false)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("examples/set_array.csp", "[examples]")
{
    Run result = run("set_array", example("set_array"), "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "S(0): print(9, true)",
        "S(0): print(5, false)",
        "S(0): print(1, true)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("examples/set_scan.csp", "[examples]")
{
    Run result = run("set_scan", example("set_scan"), "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: print(2, true)",
        "X: print(7)",
        "X: print(2)",
        "X: print(9)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("examples/squash.csp", "[examples]")
{
    Run result = run("squash", example("squash"), "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "east: print('a')", "east: print('^')", "east: print('b')", "east: print('*')",
        "east: print('c')", "east: print('^')", "east: print('^')", "east: print('d')",
    };
    CHECK(lines(result.m_output) == expected);
}
