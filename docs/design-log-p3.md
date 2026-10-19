# Project 3 Design Log

**Author:** Christopher Cornejo  
**P2 code used:** My own P2 implementation  
**Tools used:** ChatGPT (AI assistant) helped interpret the specification, draft and review the three required implementations, expand the tests, and troubleshoot the build. I used CMake, GCC, CTest, and the supplied test programs to compile and validate the repository.

## 1. Hash table

I implemented `ToolRegistry` with **separate chaining**. The table is a vector of buckets, and each bucket is a vector of entries containing a tool name and a `std::unique_ptr<Tool>`. Chaining makes collisions straightforward to handle: multiple different names can share a bucket without needing tombstones or a complicated probe sequence. It also gives the required `max_probe_length()` measurement a direct meaning: the size of the longest bucket.

The table starts with 16 buckets and uses 64-bit FNV-1a as its string hash. The hash visits every character in order and returns the full hash value; only afterward do I take the remainder by the bucket count to select a bucket. The load-factor threshold is 0.75. Before adding a new entry, `add()` checks whether the next insertion would cross that threshold. If it would, the registry doubles its bucket count and reinserts all existing entries using their new bucket indexes. Duplicate names are rejected before resizing, and a null tool is rejected immediately.

The 10,000-tool test produced these measurements:

| Measurement | Result |
|---|---:|
| Bucket count | 16,384 |
| Load factor | 0.6104 |
| Longest FNV-1a chain | 6 |
| Longest chain using `s[0] + s.size()` | 9,000 |
| Distinct FNV-1a hashes among the 10,000 names | 10,000 |

The weak hash performs badly because nearly every `tool_0` through `tool_9999` name starts with the same character, and their lengths take only a few values. Most names therefore pile up into a small number of buckets. FNV-1a spreads these keys much more evenly. `add()` is amortized O(1): most insertions only hash the name and append to a short bucket, while a resize is O(n) but doubles capacity, so the resize cost is spread over many later insertions. Expected lookup is O(1) with a well-distributed hash and a controlled load factor.

## 2. Pointer stability

Each entry owns its tool through `std::unique_ptr<Tool>`. Rehashing moves entries and their unique pointers between bucket vectors, but it does not move the actual `Tool` object that was allocated on the heap. Therefore, a `Tool*` returned by `find()` continues pointing to the same object after later insertions trigger resizes. The scale test saves the pointer for `tool_0`, inserts all 10,000 tools, and checks that the pointer is still identical. A separate ownership test verifies that a rejected duplicate is destroyed immediately and the registered tool is destroyed when the registry is destroyed.

## 3. Calculator trace

For `2 * (3 + 4) - 5`, the values stack holds numbers and the operators stack holds operators and opening parentheses. First, `2` is pushed to values and `*` is pushed to operators. The `(` is then pushed, followed by `3`. The `+` is pushed above the parenthesis, and `4` is pushed to values. When `)` arrives, the evaluator applies `3 + 4`, replacing those two values with `7`, and removes the matching `(`. When `-` arrives, the waiting `*` has higher precedence, so `2 * 7` is applied first, leaving `14`; then `-` is pushed. The final `5` is pushed. At the end, subtraction is applied to produce `14 - 5 = 9`. Both stacks are processed iteratively, so deeply nested parentheses and long expressions do not consume the C++ call stack.

## 4. Errors as values

Path rejection, a missing file, or division by zero are expected outcomes when a model asks a tool to do something. Returning `false` with a reason lets the caller pass the error back to the model so it can recover or try another request. Exceptions are more appropriate for unexpected programming failures, such as calling a JSON accessor with the wrong type. `Workspace::resolve` does only lexical path checks; the provided `check_symlinks` routine separately checks the filesystem for symbolic-link escapes before file I/O.

## 5. What I would design differently

If I extended the registry, I would benchmark storing the hash alongside each entry so rehashing could reuse it instead of hashing every name again. I would keep the public interface the same and retain chaining, pointer ownership, and the current threshold unless measurements showed a clear improvement. For this project, the measured chain length and passing regression and integration tests show that the simpler representation meets the requirements.
