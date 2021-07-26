let sources = import nix/sources.nix; in
{ pkgs ? import sources.nixpkgs {}
, with-build-dependencies ? true
, build-the-program ? true
, test-the-program ? true # during the build test that all unit tests are passing
, use-clang ? false # build using Clang instead of GCC
, clang-version ? 12 # Clang version to use (either a number or a derivation)
}:
let
  cpp-parsing = pkgs.callPackage ./. {
    inherit test-the-program use-clang clang-version;
  };
in
pkgs.mkShell {
  buildInputs =
    pkgs.lib.optionals with-build-dependencies
      (cpp-parsing.nativeBuildInputs
      ++ cpp-parsing.buildInputs
      ++ cpp-parsing.testingDependencies)
    ++ pkgs.lib.optional build-the-program cpp-parsing;
}
