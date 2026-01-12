{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = [
    pkgs.platformio
    pkgs.gcc-arm-embedded
    pkgs.stlink
  ];

}
