20151219

Note: this software is very old. The amount of support I can offer is minimal.

The dashboard has a number of features:
-OBD2 compatibility 
-Displays speed, tach, and trouble codes in real time
-Interfaces with WinAmp for music
-Touch-screen compatible interface
-Metric or imperial display
-Can display external webcam image
-Can display text messages from any program that can create a text log

Since I wrote it specifically for my 1996 Dodge Neon, it also has a number of limitations:
-Runs at 800x600 only
-Heavy CPU usage
-Most settings are hard coded
-Tach and Speedometer are always drawn as full-page left-to-right images
-Only supports US ISO through the BR16F84 interface IC. I got my box from http://obddiagnostics.com/, 
 and it is the only one I've tested.
-Limited error recovery.
-Always starts with KM/H, and WinAmp in shuffle mode
-External webcam image must be 'webcam.bmp' in the working folder and must be 160x120 pixels
-Only tested on Windows XP - might work on 2000. Not likely to work on any other.
-Font sizes, layouts, and colors are hard coded
-Bitmap filenames are hard coded, but may be changed. The masks must be mono bitmaps, with
 white representing all colored areas in the related color file (see the provided samples)

The frame rate is more or less limited by the OBD2 protocol itself.
Sometimes the box listed above can not communicate with the car - and must be reset by unplugging 
it from the car, and plugging it back in. I will publish a circuit tweak and software update later 
that lets the PC do this.

I installed this program on a Thinkpad 560Z, Pentium 2 laptop. I also installed SCWebCam3 
(http://harmlesslion.com/software/scwebcam). This was configured for a Vibra Webcam, and the
system set to autodial through my cell phone on demand to allow me to upload webcam and dashboard
whereever I went. The webcam didn't work very well, but the rest wasn't bad.

It's also intended to work with WinAmp, and most of the touchscreen interface is based on that.
If you don't have a touchscreen, you can use a mouse, but since it's hidden, you need to remember
where the cursor is. There are five regions on the screen for control:

	// Top Left - WinAmp Previous
	// Top Right - WinAmp Next
	// Top Center - Play/Pause
	// Bottom Left - Winamp Shuffle toggle
	// Bottom Right - Miles/KM

That's all there is to it right now - until I add some configuration, it'll probably only work
on US cars with the above interface chip. I ran it on a long (4000km round trip!) drive, and it
worked pretty damn well, except the laptop kept shutting off the LCD. But since the plan is to
install a VGA touchscreen instead, that's a minor detail.

Updates will be coming, however, as I sold the Neon. When I get time, I will attempt to interface
it to my Toyota RAV4! ;)

To use: 
-Extract the zip file to a working folder, preferably C:\DASHBOARD.
-Edit Dashboard.ini for your system (there are very few configurable options, don't mess with the
 timeouts unless you need to)
-C:\Log.txt will be written if any errors occur, this usually gives more information as to the problem
-If you want to use SCWebCam, consider using the files in Samples\ with it - Startup.bat will help start
 the system smoothly, and Dashboard.txt is an SCWebCam script file that handles dashboard and webcam,
 and reports status into C:\Dashboard\SCWebCam.txt for display.
-Connect the PC to your interface box, and the interface to the car
-Start Dashboard.exe

If it works, let me know! If it doesn't, well, it's too early to let me know. :)

NOTE: I believe the fonts are redistributable (that's how *I* got them!) If not, let me know and I'll
      pull them from my distribution. :)
