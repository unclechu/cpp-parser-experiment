## To Do

- Read JSON from stdin
- Implement JSON value parser
    > Combine other atomic-ish JSON parsers together in order to cover any JSON value.
    * [ ] Object parser
    * [ ] Array parser
    * [ ] String parser
    * [ ] Number parser (either decimal or fractional)
    * [ ] Boolean parser
    * [ ] Null parser
- Pretty-printer for JSON
- Nix: Add build option for using clang instead of gcc
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
