# libvalce

libvalce is a GObject based API wrapper for libVLC >= v3.0.0

## example of the API use in Vala

```
using Valce;
using Gtk;

	var player = new Player ();
	bool load = player.set_uri ("file:///home/user/Music/file.m4a");
	if (load)
               player.play ();

        player.volume_changed.connect ( () => {
               stdout.printf ("Volume_changed: %f\n", player.volume);
        });

        stdout.printf ("album: %s\n", player.media_info.album);

        var button = new Button.with_label ("Play/Pause");
        button.clicked.connect (() => {
               if (player.state == PlayerState.PLAYING)
                       player.pause ();
               else
                       player.play ();
        });

```
