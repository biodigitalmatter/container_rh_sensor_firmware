{
  description = "Description for the project";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    systems.url = "github:nix-systems/default";
  };
  outputs = inputs @ {flake-parts, ...}:
    flake-parts.lib.mkFlake {inherit inputs;} {
      systems = import inputs.systems;
      perSystem = {pkgs, ...}: {
        devShells.default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            arduino-cli
            python3
            (python3.withPackages (python-pkgs: [
              python-pkgs.pyserial
            ]))
            (writeShellScriptBin "a" "${lib.getExe arduino-cli} $@")
          ];
        };
        formatter = pkgs.alejandra;
      };
    };
}
