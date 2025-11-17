{
  config,
  lib,
  pkgs,
  ...
}:
with lib; let
  cfg = config.services.airpods-handoff;
in {
  options.services.airpods-handoff = {
    enable = mkEnableOption "AirPods Linux-Apple seamless handoff daemon";

    package = mkOption {
      type = types.package;
      default = pkgs.airpods-handoff;
      defaultText = literalExpression "pkgs.airpods-handoff";
      description = "The airpods-handoff package to use.";
    };

    macAddress = mkOption {
      type = types.str;
      example = "34:0E:22:49:C4:73";
      description = "Bluetooth MAC address of your AirPods.";
    };

    user = mkOption {
      type = types.str;
      default = "root";
      description = "User to run the airpods-handoff service as.";
    };
  };

  config = mkIf cfg.enable {
    assertions = [
      {
        assertion = cfg.macAddress != "";
        message = "services.airpods-handoff.macAddress must be set";
      }
    ];

    # Ensure bluetooth is enabled
    hardware.bluetooth.enable = mkDefault true;

    # Add user to bluetooth group
    users.users.${cfg.user}.extraGroups = ["bluetooth"];

    hardware.bluetooth.settings = {
      General = {
        DeviceID = "bluetooth:004C:0000:0000";
      };
    };

    # Configure bluetooth main.conf for Apple DeviceID
    # environment.etc."bluetooth/main.conf".text = mkAfter ''
    #   [General]
    #   DeviceID = bluetooth:004C:0000:0000
    # '';

    systemd.services.airpods-handoff = {
      description = "AirPods Linux-Apple Handoff";
      after = ["bluetooth.target"];
      wantedBy = ["multi-user.target"];

      serviceConfig = {
        Type = "simple";
        ExecStart = "${cfg.package}/bin/airpods-handoff ${cfg.macAddress}";
        Restart = "on-failure";
        User = cfg.user;
      };
    };
  };
}
