# Star-Code Menu

## \*1*x* - set region
* \*10 &rarr; output current region to serial console
* \*11 &rarr; North America
* \*12 &rarr; United Kingdom

## \*2*x* - DTMF decoding with hardware or software
* \*20 &rarr; use hardware DTMF decoding
* \*21 &rarr; use software DTMF decoding
* \*22 &rarr; play DTMF "D" to clear LED's on DTMF hardware module

## \*3*x* - DTMF decode speed testing
* \*30 &rarr; test with increasing speed until complete failure to read any digits
* \*31 to 39 &rarr; last digit determines how many iterations to test

## \*4*x* - play mp3 messages
* \*41 &rarr; circuits-xxx-anna.mp3
* \*42 &rarr; complete2-bell-f1.mp3
* \*43 &rarr; discoornis-bell-f1.mp3
* \*44 &rarr; timeout-bell-f1.mp3
* \*45 &rarr; svcoroption-xxx-anna.mp3
* \*46 &rarr; anna-1DSS-default-vocab.mp3
* \*47 &rarr; feature5-xxx-anna.mp3
* \*48 &rarr; feature6-xxx-anna.mp3

## \*5*xx* - play mp3 slices
* \*500: &rarr; zero
* \*501: &rarr; one
* \*502: &rarr; two
* \*503: &rarr; three
* \*504: &rarr; four
* \*505: &rarr; five
* \*506: &rarr; six
* \*507: &rarr; seven
* \*508: &rarr; eight
* \*509: &rarr; nine
* \*510: &rarr; zero (alt)
* \*511: &rarr; please note
* \*512: &rarr; this is a recording
* \*513: &rarr; has been changed
* \*514: &rarr; the number you have dialed
* \*515: &rarr; is not in service
* \*516: &rarr; please check the number and dial again
* \*517: &rarr; the number dialed
* \*518: &rarr; the new number is
* \*519: &rarr; enter function
* \*520: &rarr; please enter
* \*521: &rarr; area code
* \*522: &rarr; new number
* \*523: &rarr; invalid
* \*524: &rarr; not available
* \*525: &rarr; enter service code
* \*526: &rarr; deleted
* \*527: &rarr; category
* \*528: &rarr; date
* \*529: &rarr; re enter
* \*530: &rarr; thank you
* \*531: &rarr; or dial directory assistance

## \*6*x* - WiFi network
* \*60 &rarr; TODO: show current network status on serial console
* \*61 &rarr; list available networks to serial console

## \*7*x* - console input mode
* \*71 &rarr; start console mode
* commands:
  * exit &rarr; use exit command or reset button to return to phone mode
  * networks &rarr; list available networks
  * test &rarr; confirms receipt of test command
  