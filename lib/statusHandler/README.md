# statusHandler

This code manages the addressable RGB LED to show status based on call progress mode. 

I chose an addressable LED rather than a bare RGB LED to reduce the number of GPIO pins needed to drive it (1 vs. 3). I also hadn't used one before and wanted to try it out.

We are using [FastLED](https://fastled.io/) to do the magic.