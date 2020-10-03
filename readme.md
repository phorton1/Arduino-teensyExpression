# teensyExpression MIDI controller

[![teensyExpression MIDI controller](images/teensyExpression01_resized.jpg)](images/teensyExpression01.jpg)

This github repository describes and contains the source code and other materials
for a 3D printed, **teensy 3.6** based MIDI controller.  The controller has a 5x5 array
of switches, each with an individually adressable ws2812B colored LED,
four rotary controllers,
a 3.5" touchscreen,
inputs for four (4) standard 1/4" expression pedals, a 1/8" stereo jack used for a serial port, and the main USB type "A" connector,
a USB host port, and, lol, a volt/amp meter to monitor the USB power supply.


[![teensyExpression rotary controls](images/teensyExpression04_resized.jpg)](images/teensyExpression04.jpg)
*four rotary controllers*

[![teensyExpression touch screen](images/teensyExpression03_resized.jpg)](images/teensyExpression03.jpg)
*3.5" touchscreen*

[![teensyExpression back panel](images/teensyExpression02_resized.jpg)](images/teensyExpression02.jpg)
*inputs for four (4) standard 1/4" expression pedals, 1/8" stereo jack for serial port, and the main USB type "A" connector*

[![teensyExpression host port and amp-meter](images/teensyExpression05_resized.jpg)](images/teensyExpression05.jpg)
*USB host port, and volt/amp meter on USB power supply*



It is intended to be a *foot* controller ... hence the tiered setup which makes it
easier to press buttons with your toes without having to reach over other buttons.
FWIW, I play the guitar barefooted, while sitting in a chair.

There are two major aspects to this project.  First there is the **hardware**, which is
relatively straight forward, and there are circuit diagrams, fusion 360 files, some
instructions, and so on that might allow someone to create a copy if they wanted to.

The second major aspect is the **software**. Inasmuch as this was conceived as BOTH an
experimental platform to let me mess with USB midi, and particularly the **Fishman FTP
Triple Play** dongle/controller, it also has been programmed to fit into my existing
performance setup.  For me, it is "gig ready", as well as a place I can try different
experiments with various things.

The software does not present itself as a general purpose, commercially viable,
consumer product.  Though that could be done, it is not my primary focus. So, while
I would be able to create, for example, a box on which the end user could map the
buttons to various MIDI CC control messages at "run time", that is not currently
implemented.

Because the entire thing can be (fairly) easily programmed from the Arduino IDE,
at this time I am selfishly doing whatever the heck I want in C++.   Some stuff
has been modularized and incorporated into things like a system configuration "preferences"
page, with persistent values stored in EEPROM, other stuff is "hard coded" into the C++, an
approach which limits the use of this by less technically saavy "consumer" or "nubie"
readers and potential users.

[![early preferences page](images/teensyExpression06_resized.jpg)](images/teensyExpression06.jpg)

That's what it is.

There is still, in my opinion, considerable value in making this public, and so,
here, in spite of ongoing development, I am creating a public github
repository so that I can present it to you.


## THIS Repository Organization

This repository contains contains the main INO file (teensyExpression.ino) and C++ and H files
directly included in it.

It contains two subfolders which are considered "public" containing ancilary information
and readme files about how to build the circuit board and some information on my reverse
engineering efforts with regards to the Fishman FTP Triple Play midi pickup:

* [hardware/](hardware/) - contains the basic schematic, circuit diagrams, and information about the electronics hardware design
* [ftp/](ftp/) - contains separate discussions and documents regarding my reverse engineering efforts on the FTP Triple play dongle and controller

In addition, in this iteration of the github project, I have moved the fusion 360 files
to a separate github project.   The fusion files are large, and are not needed to build
the INO file:

* [fusion/](https://github.com/phorton1/Arduino-teensyExpression-fusion) - contains the STL, fusion 360 files, and notes on how to print and assemble the box


There are a number of other sub-folders which are considered "private" for my own
personal use, and/or which exist only to support the readme.md files in the this
and the above subfolders:

* **images** - contains images used on this and various readme pages
* **design** - contains a variety of weird files, images, doc files, that, for me, pertain
      to the design of the project, but which are not meant to be necessrily useful
      for a reader or anyone besides myself.
* **test** - contains a number of test programs that are not necessarily currently up-to-date
    or useful.  I keep these for posterities sake as they may have snippets of code I will
    need in the future.



### Notes on Building and GitHub **DEPENDENCIES**


This project relies on the teensyDuino installation.

It also depends on a number of Arduino libraries that must be manually placed in your
Arduino/libraries folder.  There is one small library that I wrote, that I generally use in my
Arduino programs, and is common to more than one of my published projects.

In addition there are several libraries that are publicly available, but unchanged by me,
which I have forked into my github account for ease in finding them, and to provide
me with a stable known "snapshot" of those libraries.

And finally there are some libraries that are publicly available, but to which I have made modifications
to, small or large, in order to get them to work in this project.


Each of these libraries is available as a github project here in my account, with a
name that is formatted like "Arduino-libraries-NameOfLibrary".   These libraries
should be "cloned" or otherwise copied into your Arduino/libraries folder.  Once
you have done that, you may rename them to remove the "Arduino-libraries" portion,
just leaving the NameOfLibrary portion in the directory structure.

* [myDebug](https://github.com/phorton1/Arduino-libraries-myDebug) -
   A debugging output library common to my Arduino projects, it contains
   global methods with names like "display" and "display_bytes" which are called
   throughout my code, and which can be turned on or off via defines.

* [ILI9341_t3](https://github.com/phorton1/Arduino-libraries-ILI9341_t3) and
  [ILI9341_fonts](https://github.com/phorton1/Arduino-libraries-ILI9341_fonts) -
  These are UNMODIFIED versions of Paul's ILI9341 libraries, forked from
  his repository on 2020-10-03.   My program *may* compile and work with
  more current versions that he makes available at https://github.com/PaulStoffregen/ILI9341_t3
  and https://github.com/PaulStoffregen/ILI9341_fonts, but these forks are
  known to compile and work properly with my code.

* [my_LCDWIKI_GUI](https://github.com/phorton1/Arduino-libraries-my_LCDWIKI_GUI),
  [my_LCDWIKI_KBV](https://github.com/phorton1/Arduino-libraries-my_LCDWIKI_KBV), and
  [my_LCDWIKI_TouchScreen](https://github.com/phorton1/Arduino-libraries-my_LCDWIKI_TouchScreen) -
  These repositories contain the rude and crude port of the LCDWIKI Arduino libraries
  to the Teensy for the cheap touch screen I ended up using.  The touch screen
  is an 8 bit parellel port screen that looks like this:

  [![touchscreen holder](images/teensyExpression15_resized.jpg)](images/teensyExpression15.jpg)


When I started working on this project, I started off using a different, much
faster and more teensy compatible 2.8" SPI touch screen.  One night I burned it, and
because I did not want to wait 6 weeks for a replacement (I live on a remote
island in the tropics) I was forced to go to a slower speed 8 bit parallel
Arduino touch screen ... which in turn required me to port the LCDWIKI library
for it to the teensy 3.6, which I did.



# Credits

Pretty much everything I do has been tremendously influenced by, and is more or less a direct
result of the work done by **Paul Stoffregen** and his wonderful **teensy** series of MPU boards.

Please visit his website at [prjc.com](https://www.prjc.com) and support him by purchasing teensy
boards and teensy related parts and accessories.    He also has github, hackaday, and facebook pages
that you can find easily on the internet. The man's a giant to me, and has forged important new
ground not only with the boards he makes, but also with the **vast** amount of open source
software he has created, not the least of which includes the ws2812 and ili9431 libraries, but
also his audio library, and well, what can you say ... so much more!


Also thanks to the many folks who have contributed to the [lcdwiki](http://www.lcdwiki.com) project.
Though I can't name them by name, many are the times when I have visited their website and utilized
their code.



That's it!

Time for an initial rude, crude, commit.

Thanks for checking this out!

July 2020 - Patrick
