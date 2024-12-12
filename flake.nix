{
  description = "A flake with hellwal package/hm-module/dev-shell";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, flake-utils, nixpkgs }:
      let
        system = "x86_64-linux";
        inherit (nixpkgs) lib;
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages."x86_64-linux".default = pkgs.callPackage ./nix/default.nix {};
        
        homeManagerModules.default = import ./nix/hm-module.nix;

        devShells."x86_64-linux".default = pkgs.mkShell {
          nativeBuildInputs = [
            self.packages.x86_64-linux.default
          ];
        };
      };
}