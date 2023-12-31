# module ideas

|Acronym|Term|Headers|Description|
|---|---|---|---|
|SLIM|Subscriber Line Interface Module|1 Male|Drives one phone|
|SLIP|Subscriber Line Interface Power|1 Male, 1 Female|Power module for one SLIM|
|SLAT|Subscriber Line Audio Trunk|1 Male, 1 Female|Wired audio for one SLIM; has 600 ohm transformer, RJ11 jack, surge protection and jumpers or switches for selecting which header pins tie to audio if using backplane|
|SLAB|Subscriber Line Access Backplane|several Female|Backplane for multiple lines; tidy wiring, single chassis, single power supply|
|SLAM|Subscriber Line Access Module|1 Male|Audio switching matrix|
|SLAP|Subscriber Line Access Power|1 Male, 1 Female|Power module for entire backplane; beefier than SLIP; can be sandwiched between SLAM and SLAB or in separate slot|
|SLAJ|Subscriber Line Access Jacks|1 Female|Breakout of RJ11 jacks for standalone interconnect with standalone SLIMs if not using backplane|

>Modules with both male and female headers are able to be sandwiched between other modules. 

```mermaid
flowchart LR
  subgraph -
    direction LR
    Phone((phone)) -.-|line| SLIM
    subgraph Standalone WiFi
      SLIM <==>|header| SLIP[/SLIP/]
    end
  end
```
```mermaid
flowchart LR
  subgraph -
    direction LR
    Phone((phone)) -.-|line| SLIM
    subgraph Standalone Wired
      SLIM <==>|header| SLAT([SLAT]) <==>|header| SLIP[/SLIP/]
    end
  end
```
```mermaid
flowchart LR
  subgraph Wired Audio Matrix
    direction LR
    subgraph Standalone SLAM
      SLAJ <==> SLIP4[/SLIP/] <==> SLAM{{SLAM}}
    end
    subgraph Standalone SLIM 1
      SLIM1[SLIM] <==>|header| SLIP1[/SLIP/] <==>|header| SLAT1([SLAT]) -.-|audio trunk| SLAJ
    end
    Phone1((phone)) -.-|line| SLIM1
    subgraph Standalone SLIM 2
      SLIM2[SLIM] <==>|header| SLIP2[/SLIP/] <==>|header| SLAT2([SLAT]) -.-|audio trunk| SLAJ
    end
    Phone2((phone)) -.-|line| SLIM2
    subgraph Standalone SLIM 3
      SLIM3[SLIM] <==>|header| SLIP3[/SLIP/] <==>|header| SLAT3([SLAT]) -.-|audio trunk| SLAJ
    end
    Phone3((phone)) -.-|line| SLIM3
    SLAM -.-|remote trunk line| SLAM2{{SLAM\nLocation 2}}
    SLAM -.-|remote trunk line| SLAM3{{SLAM\nLocation 3}}
  end
```
```mermaid
flowchart LR
  subgraph Backplane Audio Matrix
    direction LR
    Phone1((phone)) -.-|line| SLIM1
    Phone2((phone)) -.-|line| SLIM2
    Phone3((phone)) -.-|line| SLIM3
    subgraph Backplane Unit
      SLIM1[SLIM] <==>|header| SLAT1([SLAT]) <==>|header| SLAB
      SLIM2[SLIM] <==>|header| SLAT2([SLAT]) <==>|header| SLAB
      SLIM3[SLIM] <==>|header| SLAT3([SLAT]) <==>|header| SLAB
      SLAP[/SLAP/] <==>|header| SLAB[[SLAB]]
      SLAM1{{SLAM}} <==>|header| SLAB
    end
    SLAM2{{SLAM\nLocation 2}} -..-|remote trunk line| SLAM1
    SLAM3{{SLAM\nLocation 3}} -..-|remote trunk line| SLAM1
  end
```

### Common Header
|Signal|Source|Sink|Comment|
|---|---|---|---|
|GND|SLIP/SLAP|Any|multiple pins for extra current?|
|3.3V|SLIP/SLAP|Any|multiple pins for extra current?|
|5V|SLIP/SLAP|Any|multiple pins for extra current?|
|-5V|SLIP/SLAP|Any|multiple pins for extra current?|
|I2C SDA|Any|Any|I2C multi-master mode|
|I2C SCL|Any|Any|I2C multi-master mode|
|Tip|SLIM|SLAT|phone loop|
|Ring|SLIM|SLAT|phone loop|
|Audio Relay|SLIM|SLAT|connect Tip/Ring to Audio +/-|
|Audio+|SLAT|SLAM|bidirectional audio trunk signal|
|Audio-|SLAT|SLAM|omit if grounded audio works with matrix chip|

### SLAM Header Extension
|Signal|Comment|
|---|---|
|A1+|from SLIM slot 1|
|A1-|omit if grounded audio works with matrix chip|
|A2+|from SLIM slot 2|
|A2-|omit if grounded audio works with matrix chip|
|A3+|from SLIM slot 3|
|A3-|omit if grounded audio works with matrix chip|
|A4+|from SLIM slot 4|
|A4-|omit if grounded audio works with matrix chip|
|A5+|from SLIM slot 5|
|A5-|omit if grounded audio works with matrix chip|
|A6+|from SLIM slot 6|
|A6-|omit if grounded audio works with matrix chip|
|A7+|from SLIM slot 7|
|A7-|omit if grounded audio works with matrix chip|
|A8+|from SLIM slot 8|
|A8-|omit if grounded audio works with matrix chip|
>Front panel display for backplane assembly possible with I2C bus