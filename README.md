# FTL_Algorithm
FlashMem Simulator with FTL Algorithm(Sector Mapping Method, Block Mapping Method)

<strong>< Sector Mapping Method, Block Mapping Method Command List ></strong>

| Command | Action |
|---|:---:|
| init x or i x | Create x MB Storage File |
| read LSN or r LSN | Read data at Logical Sector Num(LSN) Position |
| write LSN data or w LSN data | Write data at Logical Sector Num(LSN) Position |
| change | Change Mapping Method |
| print | Print Mapping Table(LSN -> PSN or LBN -> PBN) |

<br></br>

<strong>< Normal Mode (with not use Any Mapping Method) Command List ></strong>
  
| Command | Action |
|---|:---:|
| init x or i x | Create x MB Storage File |
| read PSN or r PSN | Read data at Physical Sector Num(PSN) Position |
| write PSN data or w PSN data | Write data at Physical Sector Num(PSN) Position |
| erase PBN or e PBN | Erase data at Physical Block Num(PBN) Position |
| change | Change Mapping Method |

<br></br>


- Simulator that creates flash memory, reads data in physical sectors, inputs data, and erases data in blocks
- No limit on FlashMem Storage File creation capacity<br>

<br></br>
[![HitCount](http://hits.dwyl.io/hyung8789/FTL_Algorithm.svg)](http://hits.dwyl.io/hyung8789/FTL_Algorithm)
