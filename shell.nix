let sources = import nix/sources.nix; in
{ pkgs ? import sources.nixpkgs {}
, with-build-dependencies ? true
, build-the-program ? true
}:
let
  cpp-parsing = pkgs.callPackage ./. {};
in
pkgs.mkShell {
  buildInputs =
    pkgs.lib.optionals with-build-dependencies
      (cpp-parsing.nativeBuildInputs ++ cpp-parsing.buildInputs)
    ++ pkgs.lib.optional build-the-program cpp-parsing;
}
