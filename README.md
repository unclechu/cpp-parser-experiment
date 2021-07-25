My own experiment of implementing Haskell-like parsers in C++.

**WIP:** See [TODO.md](TODO.md) for details.

## Disclaimer

This solution wasn’t intended to be super fast or to have a small footprint in
memory. I didn’t care to pay much attention for optimization and/or to add some
sort of a benchmark (but you can make some merge request(s) if you’d like to
that would fill these gaps). I only created this as a Proof of Concept and for
fun. Nice API that would be as close as possible to Haskell was the goal.

Also I didn’t plan to fully cover the whole set of JSON features. At least JSON
string parser is not complete. It does not understand escaped unicode characters
and stuff. This can be improved later but parsing simple JSON file is enough as
an example for this Proof of Concept.

## Motivation

**TODO:** Add a description of what this solution introduces.

## How to build and run

You can just run `make` but providing dependencies is on you.
I recommend to use [Nix](https://nixos.org/download.html#nix-quick-install) so
that all the dependencies will be provided for you automatically and with exact
same version I used when developing this stuff.

### Using Nix

By default the program reads raw JSON from stdin and parses it printing the
serialized JSON back to stdout.

``` sh
nix-shell --run 'cpp-parsing < example.json'
```

Or build and run it separately:

``` sh
nix build
result/bin/cpp-parsing < example.json
```

#### Development shell

If you’re going to manually (re-)build the app using `make` or/and `g++`
directly you can omit building the app when entering a nix-shell by setting the
`build-the-program` argument to `false` like this:

``` sh
nix-shell --arg build-the-program false
```

During the build of Nix derivation it ensures that all unit tests are passing.
If you for instance want to enter a nix-shell with broken state use this
argument:

``` sh
nix-shell --arg test-the-program false
```

#### GNU/Make commands

You can also use `make` command inside a nix-shell.
Here are some examples:

``` sh
nix-shell --arg build-the-program false --run 'make test'
nix-shell --arg build-the-program false --run 'make run'
```

See [Makefile](Makefile) for all available commands.

#### Clang support

GCC is used by default. But you can use Clang instead by setting `use-clang`
Nix argument to `true` like this:

``` sh
nix-shell --arg use-clang true --run 'cpp-parsing --pretty < example.json'
```

Or:

``` sh
nix build --arg use-clang true
```

##### Overriding compiler for GNU/Make

If you want/need to use Clang with `make` directly here is an example of how you
can do it:

``` sh
nix-shell --arg build-the-program false --arg use-clang true --run 'make test CXX=clang++'
```

## Author

Viacheslav Lotsmanov

## Citation

[example.json](example.json) was taken as a reference for testing the
implementation from this Wikipedia page:
https://en.wikipedia.org/wiki/JSON#Syntax
