let sources = import nix/sources.nix {}; in
{ pkgs ? import sources.nixpkgs {}
, src-dir ? ./.
, test-the-program ? true # during the build test that all unit tests are passing
, use-clang ? false # build using Clang instead of GCC
, clang-version ? 12 # Clang version to use (either a number or a derivation)
}:
let
  inherit (pkgs) lib nix-gitignore stdenv;

  srcFileFilter = fileName: fileType:
    builtins.match "^.*\.nix$" fileName == null &&
    builtins.match "^(Makefile|.+\..+)$" (baseNameOf fileName) != null;

  takeSrc =
    nix-gitignore.gitignoreFilterRecursiveSource srcFileFilter [ ./.gitignore ];

  testingDependencies = [
    pkgs.bash
    pkgs.jq
    pkgs.diffutils
  ];

  clang =
    if builtins.isInt clang-version
    then pkgs."clang_${toString clang-version}"
    else
    if lib.isDerivation clang-version
    then clang-version
    else throw "Unexpected “clang-version” type: ${builtins.typeOf clang-version}";

  overrideMakeCompiler =
    lib.optionalString use-clang
      (lib.escapeShellArg "CXX=${clang}/bin/clang++");
in
stdenv.mkDerivation {
  name = "cpp-parsing";
  src = takeSrc src-dir;

  nativeBuildInputs =
    lib.optional (!use-clang) pkgs.gcc
    ++ lib.optional use-clang clang
    ++ [ pkgs.gnumake ]
    ++ lib.optionals test-the-program testingDependencies;

  buildPhase = ''
    make ${overrideMakeCompiler}
    ${lib.optionalString test-the-program "make test ${overrideMakeCompiler}"}
  '';

  installPhase = ''
    mkdir -p -- "$out/bin"
    make install "PREFIX=$out" ${overrideMakeCompiler}
  '';
} // {
  inherit takeSrc; # Export in case it’s needed for debugging or something
  inherit testingDependencies; # To use for development nix-shell
}
