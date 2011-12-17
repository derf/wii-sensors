Various tools related to the Wii Remote / Wii Balance Board.

## bal

Displays Balance Board weight data, mostly for debug purposes.

Usage: `./bal`

## mpcal

Calibrates the Motion Plus and outputs the calibration data.

Usage: `./mpcal`

(the program will then tell you to put the wiimote onto a flat surface)

## mplog

Logs Motion Plus data to stdout, in the format

    seconds.nanoseconds pitch roll yaw

Where Pitch, Roll and Yaw are the raw 16-bit values.

Usage: `./mplog` > *filename*

See also: http://www.grc.nasa.gov/WWW/k-12/airplane/Images/rotations.gif

## wibble

Wiimote Blinkenlights. Press D-Pad up to toggle modes, Home to quit.

Usage: `./wibble`
