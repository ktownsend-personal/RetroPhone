# module idea

|Acronym|Term|Description|
|---|---|---|
|SLIM|Subscriber Line Interface Module|Drives one phone|
|SLIP|Subscriber Line Interface Power|Power module for one SLIM|
|SLAT|Subscriber Line Audio Trunk|Wired audio for one SLIM|
|SLAB|Subscriber Line Access Backplane|Backplane for multiple lines|
|SLAM|Subscriber Line Access Module|Audio switching matrix|
|SLAP|Subscriber Line Access Power|Power module for entire backplane|


```mermaid
flowchart LR
  subgraph Standalone WiFi
    direction LR
    Phone((phone)) -.-|line| SLIM
    SLIM <==>|header| SLIP[/SLIP/]
  end
```

```mermaid
flowchart LR
  subgraph Standalone Wired
    direction LR
    Phone((phone)) -.-|line| SLIM
    SLIM <==>|header| SLAT([SLAT]) <==>|header| SLIP[/SLIP/]
  end
```
```mermaid
flowchart LR
  subgraph Backplane
    direction LR
    Phone1((phone)) -.-|line| SLIM1[SLIM] <==>|header| SLAT1([SLAT]) <==>|header| SLAB
    Phone2((phone)) -.-|line| SLIM2[SLIM] <==>|header| SLAT2([SLAT]) <==>|header| SLAB
    SLAM2{{SLAM}} -..-|remote trunk line| SLAM1{{SLAM}} <==>|header| SLAB
    SLAP[/SLAP/] <==>|header| SLAB[[SLAB]]
  end
```
