let sources = import nix/sources.nix {}; in
{ pkgs ? import sources.nixpkgs {}
, srcDir ? ./.
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
  ];

  buildPhase = ''
    make
  '';

  installPhase = ''
    mkdir -p -- "$out/bin"
    make install "PREFIX=$out"
  '';
} // {
  inherit takeSrc; # Export in case it’s needed for debugging or something
}
