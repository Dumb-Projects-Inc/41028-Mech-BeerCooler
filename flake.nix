{
  description = "PlatformIO FHS DevShell";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = {
    self,
    nixpkgs,
    ...
  }: let
    systems = ["x86_64-linux" "aarch64-linux"];
    forAllSystems = nixpkgs.lib.genAttrs systems;
  in {
    devShells = forAllSystems (system: let
      pkgs = import nixpkgs {inherit system;};

      fhs = pkgs.buildFHSEnv {
        name = "pio-fhs";
        targetPkgs = pkgs:
          with pkgs; [
            platformio
            python3
            zlib
          ];
        profile = ''
          export PLATFORMIO_CORE_DIR=$PWD/.platformio
        '';
      };
    in {
      default = fhs.env;
    });
  };
}
