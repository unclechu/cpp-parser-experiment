## To Do

- Add good description of this solution to README
    * [ ] Motivation
    * [ ] Explanation for mimicking Haskell type classes
    * [ ] Basic types for Parser implementation
    * [ ] Some parsers examples
    * [ ] Demonstration of parsers composition  in Haskell and showing equivalents-ish in C++
    * [ ] Demonstrate “curry” helper
- Implement parsing some structure from “JsonValue”
    > Like “Data.Aeson.FromJSON” in Haskell but without deriving abilities.
    * [ ] Template “fromJSON”
    * [ ] Implement some data model for the data from example JSON file

## Done

- Refactor std::variant type-casting using std::visit
    > With pattern-matching-like solution (cppreference.com has some examples)
- Improve testing
    * [x] Print result of each test
    * [x] If a test has failed continue to try other tests
    * [x] Fail at the end if any of the tests have failed
- Implement Alternative type class as in Haskell
    * [x] Use || (or) operator for <|>
- Implement “many” parser
    > In a generic way so that you take a parser and get a multiple version of it.
    * [x] Implement also “many1” for non-empty list (1 or more items in a list) -> it’s “some”
- Implement parsers for numbers
    * [x] unsigned decimal
    * [x] signed decimal (reuse unsigned decimal)
    * [x] unsigned fractional (floating point)
    * [x] signed fractional (reuse unsigned fractional)
- Parse command-line arguments
- Read JSON from stdin
- Implement JSON value parser
    > Combine other atomic-ish JSON parsers together in order to cover any JSON value.
    * [x] Object parser
    * [x] Array parser
    * [x] String parser
    * [x] Number parser (either decimal or fractional)
    * [x] Boolean parser
    * [x] Null parser
- JSON serialization
    * [x] Pretty printing
- Nix: Add build option for using clang instead of gcc
    * [x] Fix building errors when using clang
