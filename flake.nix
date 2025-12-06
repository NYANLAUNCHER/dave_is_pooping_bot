{
  description = "Nix flake for ESP-IDF Discord bot project";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-23.05";
    flake-utils.url = "github:numtide/flake-utils";
    esp-flake.url = "github:mirrexagon/nixpkgs-esp-dev";
    esp-idf = {
      url = "github:espressif/esp-idf?ref=refs/tags/v5.5.1";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, flake-utils, esp-flake, esp-idf }: flake-utils.lib.eachDefaultSystem
  (system:
    let
      pkgs = import nixpkgs { inherit system; };
      pythonEnv = pkgs.python310.withPackages (ps: with ps; [ setuptools wheel ]);
    in
    {
      devShells.default = pkgs.mkShell {
        name = "esp-project";
        buildInputs = [
          #(esp-flake.packages.${system}.esp-idf-full.override {
          #  rev = "cf7e743a9b2e5fd2520be4ad047c8584188d54da";
          #  sha256 = "sha256-tqWUTJlOWk4ayfQIxgiLkTrrTFU0ZXuh76xEZWKRZ/s=";
          #})
          esp-flake.packages.${system}.esp-idf-full
          pkgs.openssl
        ];
      };
    }
  );
}
