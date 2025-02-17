{
  description = "A Nix flake for OS development with arm-none-eabi toolchain and QEMU";

  # Use the latest unstable channel from nixpkgs (adjust as needed)
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs = {
    self,
    nixpkgs,
  }: let
    system = "x86_64-linux";
    pkgs = import nixpkgs {inherit system;};
  in {
    devShells.${system} = {
      default = pkgs.mkShell {
        buildInputs = [
          pkgs.gcc-arm-embedded
          pkgs.util-linux
          pkgs.qemu
          pkgs.bear
          pkgs.minicom
          pkgs.lrzsz
        ];

        # Optionally, you can set environment variables if needed by your build system:
        shellHook = ''
          echo "Welcome to the OS development shell."
          echo "ARM toolchain and QEMU are available."
        '';
      };
    };
  };
}
