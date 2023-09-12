# ringHandler

This library wraps the logic for instructing the SLIC to ring a phone. It uses two GPIO pins for RM and FR inputs of the SLIC. To ring a phone we must set RM (ring mode) high and then toggle FR (forward-reverse) at the desired ring frequency. For North America we use 20Hz. We must also manage the cadence of silence between rings. 