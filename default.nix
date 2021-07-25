let sources = import nix/sources.nix {}; in
{ pkgs ? import sources.nixpkgs {}
, src-dir ? ./.
, test-the-program ? true # during the build test that all unit tests are passing
, use-clang ? false # build using Clang instead of GCC
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

  clang = pkgs.clang_12;

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
