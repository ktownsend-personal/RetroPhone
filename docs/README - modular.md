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
  subgraph Backplane Audio Matrix
    direction LR
    Phone1((phone)) -.-|line| SLIM1[SLIM] <==>|header| SLAT1([SLAT]) <==>|header| SLAB
    Phone2((phone)) -.-|line| SLIM2[SLIM] <==>|header| SLAT2([SLAT]) <==>|header| SLAB
    Phone3((phone)) -.-|line| SLIM3[SLIM] <==>|header| SLAT3([SLAT]) <==>|header| SLAB
    SLAP[/SLAP/] <==>|header| SLAB[[SLAB]]
    SLAM2{{SLAM\nLocation 2}} -..-|remote trunk line| SLAM1{{SLAM}} <==>|header| SLAB
    SLAM3{{SLAM\nLocation 3}} -..-|remote trunk line| SLAM1
  end
```
```mermaid
flowchart LR
  subgraph Standalone Audio Matrix
    direction LR
      SLIM1[SLIM] <==>|header| SLIP1[/SLIP/] <==>|header| SLAT1([SLAT]) -.-|wire| SLAM
      SLIM2[SLIM] <==>|header| SLIP2[/SLIP/] <==>|header| SLAT2([SLAT]) -.-|wire| SLAM
      SLIM3[SLIM] <==>|header| SLIP3[/SLIP/] <==>|header| SLAT3([SLAT]) -.-|wire| SLAM
      SLIP4[/SLIP/] <==> SLAM{{SLAM}}
      SLAM -.-|remote trunk line| SLAM2{{SLAM\nLocation 2}}
  end
```