// tests/p3/test_p3.cpp — YOUR P3 TEST SUITE.
//
// The five tests below are worked examples. Keep them, and add your own
// until you have AT LEAST 15 (spec §6 lists what must be covered).
// Name every test <area>_<what>: reg_ for the registry, ws_ for the
// workspace, calc_ for the calculator.
//
// Build & run:   cmake --build build && ./build/test_p3
// One area:      ./build/test_p3 reg_        (names starting with "reg_")
// One test:      ./build/test_p3 =reg_add_and_find

#include "../mini_test.h"
#include "../test_helpers.h"
#include "tools/calculator.h"
#include "tools/tool_registry.h"
#include "tools/workspace.h"
#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {

    class NamedTool : public Tool {
    public:
        explicit NamedTool(std::string n) : n_(std::move(n)), schema_(JsonValue::object()) {}
        std::string name() const override { return n_; }
        std::string description() const override { return "test tool"; }
        const JsonValue& schema() const override { return schema_; }
        ToolResult invoke(const JsonValue&, ToolContext&) override { return ToolResult::success(n_); }
    private:
        std::string n_;
        JsonValue schema_;
    };

    class CountedTool : public Tool {
    public:
        CountedTool(std::string n, int* destroyed)
            : name_(std::move(n)), destroyed_(destroyed), schema_(JsonValue::object()) {
        }
        ~CountedTool() override { ++*destroyed_; }
        std::string name() const override { return name_; }
        std::string description() const override { return "counted test tool"; }
        const JsonValue& schema() const override { return schema_; }
        ToolResult invoke(const JsonValue&, ToolContext&) override { return ToolResult::success(name_); }
    private:
        std::string name_;
        int* destroyed_;
        JsonValue schema_;
    };

    bool eval(const std::string& expr, double* value, std::string* error) {
        std::vector<CalcToken> tokens;
        if (!calc_tokenize(expr, &tokens, error)) {
            return false;
        }
        return calc_evaluate(tokens, value, error);
    }

}  // namespace

// ---------------------------------------------------------------- registry

TEST(reg_empty_table) {
    ToolRegistry r;
    CHECK(r.size() == 0);
    CHECK(r.bucket_count() >= 1);
    CHECK(r.bucket_count() <= 64);
    CHECK(r.load_factor() == 0.0);
    CHECK(r.max_probe_length() == 0);
    CHECK(r.find("missing") == nullptr);
}

TEST(reg_add_and_find) {
    ToolRegistry r;
    auto t = std::make_unique<NamedTool>("alpha");
    Tool* raw = t.get();
    REQUIRE(r.add(std::move(t)));
    CHECK(r.size() == 1);
    CHECK(r.find("alpha") == raw);
    CHECK(r.find("beta") == nullptr);
}

TEST(reg_duplicate_keeps_original) {
    ToolRegistry r;
    auto first = std::make_unique<NamedTool>("alpha");
    Tool* original = first.get();
    REQUIRE(r.add(std::move(first)));
    CHECK(!r.add(std::make_unique<NamedTool>("alpha")));
    CHECK(r.find("alpha") == original);
    CHECK(r.size() == 1);
}

TEST(reg_rejects_null_tool) {
    ToolRegistry r;
    std::unique_ptr<Tool> empty;
    CHECK(!r.add(std::move(empty)));
    CHECK(r.size() == 0);
    CHECK(r.load_factor() == 0.0);
}

TEST(reg_for_each_visits_every_tool_once) {
    ToolRegistry r;
    REQUIRE(r.add(std::make_unique<NamedTool>("gamma")));
    REQUIRE(r.add(std::make_unique<NamedTool>("alpha")));
    REQUIRE(r.add(std::make_unique<NamedTool>("beta")));

    std::vector<std::string> names;
    r.for_each([&names](const Tool& t) { names.push_back(t.name()); });
    std::sort(names.begin(), names.end());
    CHECK(names.size() == 3);
    CHECK(names[0] == "alpha");
    CHECK(names[1] == "beta");
    CHECK(names[2] == "gamma");
}

TEST(reg_tools_are_destroyed_exactly_once) {
    int destroyed = 0;
    {
        ToolRegistry r;
        REQUIRE(r.add(std::make_unique<CountedTool>("same", &destroyed)));
        CHECK(destroyed == 0);
        CHECK(!r.add(std::make_unique<CountedTool>("same", &destroyed)));
        CHECK(destroyed == 1);  // the rejected duplicate is destroyed immediately
    }
    CHECK(destroyed == 2);      // the registered tool is destroyed with the registry
}

TEST(reg_hash_uses_order_and_all_characters) {
    CHECK(ToolRegistry::hash("ab") != ToolRegistry::hash("ba"));
    CHECK(ToolRegistry::hash("alpha") != ToolRegistry::hash("alphb"));
    CHECK(ToolRegistry::hash("") != ToolRegistry::hash("a"));
}

TEST(reg_10000_tools_and_measure_hashes) {
    ToolRegistry r;
    auto first = std::make_unique<NamedTool>("tool_0");
    Tool* first_address = first.get();
    REQUIRE(r.add(std::move(first)));

    std::vector<std::size_t> hashes;
    hashes.reserve(10000);
    hashes.push_back(ToolRegistry::hash("tool_0"));
    CHECK(r.load_factor() <= 1.0);

    for (int i = 1; i < 10000; ++i) {
        const std::string name = "tool_" + std::to_string(i);
        hashes.push_back(ToolRegistry::hash(name));
        REQUIRE(r.add(std::make_unique<NamedTool>(name)));
        CHECK(r.load_factor() <= 1.0);
    }

    for (int i = 0; i < 10000; ++i) {
        const std::string name = "tool_" + std::to_string(i);
        CHECK(r.find(name) != nullptr);
    }
    CHECK(r.size() == 10000);
    CHECK(r.find("tool_0") == first_address);
    CHECK(r.max_probe_length() <= 64);

    // The spec asks us to compare the real hash to s[0] + s.size() using the
    // final table size. This deliberately weak hash clusters these tool names.
    std::vector<int> weak_counts(r.bucket_count(), 0);
    for (int i = 0; i < 10000; ++i) {
        const std::string name = "tool_" + std::to_string(i);
        const std::size_t weak_hash = static_cast<unsigned char>(name[0]) + name.size();
        ++weak_counts[weak_hash % weak_counts.size()];
    }
    std::size_t weak_longest = 0;
    for (int count : weak_counts) {
        weak_longest = std::max(weak_longest, static_cast<std::size_t>(count));
    }

    std::sort(hashes.begin(), hashes.end());
    const auto unique_end = std::unique(hashes.begin(), hashes.end());
    const std::size_t unique_hashes = static_cast<std::size_t>(unique_end - hashes.begin());
    CHECK(unique_hashes >= 9990);
    CHECK(weak_longest > r.max_probe_length());

    std::printf("10,000-tool metrics: bucket_count=%zu, load_factor=%.4f, max_chain=%zu; ",
        r.bucket_count(), r.load_factor(), r.max_probe_length());
    std::printf("weak s[0]+size longest chain=%zu; distinct FNV hashes=%zu\n",
        weak_longest, unique_hashes);
}

// ---------------------------------------------------------------- workspace

TEST(ws_accepts_relative_and_normalized_paths) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;

    REQUIRE(ws.resolve("notes/todo.txt", &out, &reason));
    CHECK(out == ws.root() / "notes" / "todo.txt");
    REQUIRE(ws.resolve("./notes//todo.txt", &out, &reason));
    CHECK(out == ws.root() / "notes" / "todo.txt");
    REQUIRE(ws.resolve("new/dir/file.txt", &out, &reason));
    CHECK(out == ws.root() / "new" / "dir" / "file.txt");
    REQUIRE(ws.resolve("my..notes.txt", &out, &reason));
    CHECK(out == ws.root() / "my..notes.txt");
}

TEST(ws_accepts_paths_that_normalize_to_root_or_inside) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;

    REQUIRE(ws.resolve(".", &out, &reason));
    CHECK(out == ws.root());
    REQUIRE(ws.resolve("a/..", &out, &reason));
    CHECK(out == ws.root());
    REQUIRE(ws.resolve("a/../b", &out, &reason));
    CHECK(out == ws.root() / "b");
}

TEST(ws_rejects_empty_path) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    CHECK(!ws.resolve("", &out, &reason));
    CHECK(!reason.empty());
}

TEST(ws_rejects_nul_byte) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    const std::string path = std::string("notes") + '\0' + "/x";
    CHECK(!ws.resolve(path, &out, &reason));
    CHECK(!reason.empty());
}

TEST(ws_rejects_absolute_path) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    CHECK(!ws.resolve("/etc/passwd", &out, &reason));
    CHECK(!reason.empty());
}

TEST(ws_rejects_parent_traversal_after_normalization) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    CHECK(!ws.resolve("..", &out, &reason));
    CHECK(!reason.empty());
    CHECK(!ws.resolve("../etc/passwd", &out, &reason));
    CHECK(!reason.empty());
    CHECK(!ws.resolve("a/../../b", &out, &reason));
    CHECK(!reason.empty());
    CHECK(!ws.resolve("notes/../../ws/notes/todo.txt", &out, &reason));
    CHECK(!reason.empty());
}

// ---------------------------------------------------------------- calculator

TEST(calc_precedence) {
    double value = 0.0;
    std::string error;
    REQUIRE(eval("2 + 3 * 4", &value, &error));
    CHECK(value == 14.0);
}

TEST(calc_division_by_zero) {
    double value = 0.0;
    std::string error;
    CHECK(!eval("1 / 0", &value, &error));
    CHECK(error == "division by zero");
}

TEST(calc_left_to_right_subtraction) {
    double value = 0.0;
    std::string error;
    REQUIRE(eval("10 - 4 - 3", &value, &error));
    CHECK(value == 3.0);
}

TEST(calc_left_to_right_division) {
    double value = 0.0;
    std::string error;
    REQUIRE(eval("100 / 10 / 5", &value, &error));
    CHECK(value == 2.0);
}

TEST(calc_parentheses) {
    double value = 0.0;
    std::string error;
    REQUIRE(eval("(2 + 3) * 4", &value, &error));
    CHECK(value == 20.0);
}

TEST(calc_nested_mixed_operators) {
    double value = 0.0;
    std::string error;
    REQUIRE(eval("2 * (3 + 4) - 5", &value, &error));
    CHECK(value == 9.0);
    REQUIRE(eval("2 * (3 + 4) / 7", &value, &error));
    CHECK(value == 2.0);
}

TEST(calc_negative_numbers) {
    double value = 0.0;
    std::string error;
    REQUIRE(eval("-3 * -3", &value, &error));
    CHECK(value == 9.0);
    REQUIRE(eval("2 * -3 + 10", &value, &error));
    CHECK(value == 4.0);
}

TEST(calc_decimal_numbers) {
    double value = 0.0;
    std::string error;
    REQUIRE(eval("7 / 2", &value, &error));
    CHECK(value == 3.5);
    REQUIRE(eval("1.25 + 2.5 * 2", &value, &error));
    CHECK(value == 6.25);
}

TEST(calc_division_by_zero_inside_parentheses) {
    double value = 0.0;
    std::string error;
    CHECK(!eval("1 / (2 - 2)", &value, &error));
    CHECK(error == "division by zero");
}

TEST(calc_empty_or_malformed_token_lists_fail_safely) {
    double value = 0.0;
    std::string error;
    CHECK(!calc_evaluate({}, &value, &error));
    CHECK(!error.empty());

    const std::vector<CalcToken> malformed = {
        { CalcToken::Kind::Number, 2.0 },
        { CalcToken::Kind::Plus, 0.0 }
    };
    error.clear();
    CHECK(!calc_evaluate(malformed, &value, &error));
    CHECK(!error.empty());
}

TEST(calc_handles_twenty_thousand_terms_iteratively) {
    std::string expression = "1";
    for (int i = 1; i < 20000; ++i) {
        expression += "+1";
    }
    double value = 0.0;
    std::string error;
    REQUIRE(eval(expression, &value, &error));
    CHECK(value == 20000.0);
}

TEST(calc_handles_deep_parentheses_iteratively) {
    std::string expression(5000, '(');
    expression += "7";
    expression += std::string(5000, ')');
    double value = 0.0;
    std::string error;
    REQUIRE(eval(expression, &value, &error));
    CHECK(value == 7.0);
}

int main(int argc, char** argv) { return mt::run_all(argc, argv); }

