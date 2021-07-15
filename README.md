My own experiment of implementing Haskell-like parsers in C++

## How to build and run

You can just run `make` but providing dependencies is on you.
I recommend to use [Nix](https://nixos.org/download.html#nix-quick-install) so
that all the dependencies will be provided for you automatically and with exact
same version I used when developing this stuff.

### Using Nix

``` sh
nix-shell --run cpp-parsing
```

Or build and run it separately:

``` sh
nix build
result/bin/cpp-parsing
```

## Author

Viacheslav Lotsmanov

## Citation

[example.json](example.json) was taken as a reference for testing the
implementation from this Wikipedia page:
https://en.wikipedia.org/wiki/JSON#Syntax
