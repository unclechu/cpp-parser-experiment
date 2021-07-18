## To Do

- Implement Alternative type class as in Haskell
    * [ ] Use || (or) operator for <|>
- Implement parsers for numbers
    * [ ] unsigned decimal
    * [ ] signed decimal (reuse unsigned decimal)
    * [ ] unsigned fractional (floating point)
    * [ ] signed fractional (reuse unsigned fractional)
- Implement “many” parser
    > In a generic way so that you take a parser and get a multiple version of it.
    * [ ] Implement also “many1” for non-empty list (1 or more items in a list)
- Improve testing
    * [ ] Print result of each test
    * [ ] If a test has failed continue to try other tests
    * [ ] Fail at the end if any of the tests have failed
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
