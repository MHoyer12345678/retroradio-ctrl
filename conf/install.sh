speaker-test -r 48000 -c 2 -D dlna -l 1
speaker-test -r 48000 -c 2 -D lmc -l 1
speaker-test -r 48000 -c 2 -D mpc -l 1
/usr/sbin/alsactl -E HOME=/run/alsa restore
