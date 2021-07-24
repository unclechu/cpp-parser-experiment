let sources = import nix/sources.nix; in
{ pkgs ? import sources.nixpkgs {}
, with-build-dependencies ? true
, build-the-program ? true
, test-the-program ? true # during the build test that all unit tests are passing
}:
let
  cpp-parsing = pkgs.callPackage ./. { inherit test-the-program; };
in
pkgs.mkShell {
  buildInputs =
    pkgs.lib.optionals with-build-dependencies
      (cpp-parsing.nativeBuildInputs ++ cpp-parsing.buildInputs ++ [
        pkgs.bash
        pkgs.jq
        pkgs.diffutils
      ])
    ++ pkgs.lib.optional build-the-program cpp-parsing;
}
