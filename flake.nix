{
  description = "Airpods Handoff Build Flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
  }:
    {
      nixosModules.default = import ./nix/module.nix;
      nixosModules.airpods-handoff = import ./nix/module.nix;

      overlays.default = final: prev: {
        airpods-handoff = final.callPackage ./nix/package.nix {src = ./.;};
      };
    }
    // flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs {
        inherit system;
      };
    in {
      packages = rec {
        default = handoff;
        handoff = pkgs.callPackage ./nix/package.nix {src = ./.;};
      };
    });
}
