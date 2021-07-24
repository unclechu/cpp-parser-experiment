let sources = import nix/sources.nix {}; in
{ pkgs ? import sources.nixpkgs {}
, srcDir ? ./.
, test-the-program ? true # during the build test that all unit tests are passing
}:
let
  inherit (pkgs) lib nix-gitignore stdenv;

  srcFileFilter = fileName: fileType:
    builtins.match "^.*\.nix$" fileName == null &&
    builtins.match "^(Makefile|.+\..+)$" (baseNameOf fileName) != null;

  takeSrc =
    nix-gitignore.gitignoreFilterRecursiveSource srcFileFilter [ ./.gitignore ];
in
stdenv.mkDerivation {
  name = "cpp-parsing";
  src = takeSrc srcDir;

  nativeBuildInputs = [
    pkgs.gcc
    pkgs.gnumake
  ] ++ lib.optionals test-the-program [
    pkgs.bash
    pkgs.jq
    pkgs.diffutils
  ];

  buildPhase = ''
    make
    ${lib.optionalString test-the-program "make test"}
  '';

  installPhase = ''
    mkdir -p -- "$out/bin"
    make install "PREFIX=$out"
  '';
} // {
  inherit takeSrc; # Export in case it’s needed for debugging or something
}
