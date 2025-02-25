{
  description = "A Nix flake for OS development with arm-none-eabi toolchain and QEMU";

  # Use the latest unstable channel from nixpkgs
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs = { self, nixpkgs }:
    let
      # Define supported systems
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      # Helper function to generate attributes for all supported systems
      forAllSystems = f: nixpkgs.lib.genAttrs supportedSystems (system: f system);

      # Create a package set for each system
      pkgsForSystem = system: import nixpkgs {
        inherit system;
        config.allowUnfree = true;
      };
    in
    {
      # Generate devShells for all supported systems
      devShells = forAllSystems (system:
        let
          pkgs = pkgsForSystem system;

          # System-specific package selections to handle differences
          # between Linux and macOS
          systemSpecificPackages =
              [
                pkgs.util-linux
                pkgs.minicom
              ];

          # Common packages for all platforms
          commonPackages = [
            # Cross-platform packages
            (if pkgs ? gcc-arm-embedded then pkgs.gcc-arm-embedded else pkgs.pkgsCross.arm-embedded.buildPackages.gcc)
            pkgs.qemu
            pkgs.bear
            pkgs.lrzsz
          ];

          # Add seer if available (might not be on all platforms)
          debuggerPackage = if pkgs ? seer then [ pkgs.seer ] else
                           if pkgs ? gdb then [ pkgs.gdb ] else [];

        in {
          default = pkgs.mkShell {
            buildInputs = commonPackages ++ systemSpecificPackages ++ debuggerPackage;

            # Shell initialization
            shellHook = ''
              echo "Welcome to the OS development shell on ${system}."
              echo "ARM toolchain and QEMU are available."
            '';
          };
        }
      );
    };
}
