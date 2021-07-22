## To Do

- Implement parsers for numbers
    * [x] unsigned decimal
    * [x] signed decimal (reuse unsigned decimal)
    * [ ] unsigned fractional (floating point)
    * [ ] signed fractional (reuse unsigned fractional)
- Implement JSON value parser
    > Combine other atomic-ish JSON parsers together in order to cover any JSON value.
    * [ ] Object parser
    * [ ] Array parser
    * [ ] String parser
    * [ ] Number parser (either decimal or fractional)
    * [ ] Boolean parser
    * [ ] Null parser
- Read JSON from stdin
- Pretty-printer for JSON
- Parse command-line arguments
    * [ ] Run some “stupid” tests by “stupid-testing” command
    * [ ] Pretty-print JSON by “--pretty” option
    * [ ] Print one-line JSON without any provided options
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
