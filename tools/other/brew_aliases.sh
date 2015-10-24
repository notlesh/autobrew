#!/bin/bash

alias brew_ls="cat ~/ab2/tools/other/brew_aliases.sh"

# shortcuts for running temp controllers
alias bk="sudo ~/ab2/tools/pid_temp_controller/debug/pid_temp_controller --temp-probe 28.EE9B8B040000 --pin-id 17 --safety-id 10 -s "
alias bk_pwm="sudo ~/ab2/tools/pwm_temp_controller/debug/pwm_temp_controller --temp-probe 28.EE9B8B040000 --pin-id 17 --safety-id 10 -f 20 -l "
alias hlt="sudo ~/ab2/tools/pid_temp_controller/debug/pid_temp_controller --temp-probe 28.3AA87D040000 --pin-id 4 --safety-id 24 -s "

# print out temps by running a "fake" pwm controller (pins are not hooked up to anything)
alias monitor_temps="sudo ~/ab2/tools/pid_temp_controller/debug/pid_temp_controller --temp-probe 28.EE9B8B040000 --pin-id 2 --safety-id 2 -s -100"

# aliases for common brew stages
alias mash_out_hold="hlt 82"
alias sparge_hold="hlt 79.25"
alias pre_boil="bk 90"
alias boil="bk_pwm 0.6"

# aliases for turning things on and off
alias p1_on="sudo gpio export 18 out; sudo gpio -g write 18 1"
alias p1_off="sudo gpio export 18 out; sudo gpio -g write 18 0"
alias p2_on="sudo gpio export 27 out; sudo gpio -g write 27 1"
alias p2_off="sudo gpio export 27 out; sudo gpio -g write 27 0"
alias valve_on="sudo gpio export 22 out; sudo gpio -g write 22 1"
alias valve_off="sudo gpio export 22 out; sudo gpio -g write 22 0"
alias valve_controller="sudo ~/ab2/tools/valve_controller/debug/valve_controller -v 22 -f 14"

# utilities
alias ftc="~/ab2/tools/other/ftc.sh"
