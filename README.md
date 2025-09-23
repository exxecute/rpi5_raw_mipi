# Linux kernel with added module for getting raw 4 lane mipi

## Links

[CSI Manual on Pi5](https://wiki.geekworm.com/CSI_Manual_on_Pi_5)

[Wiki for setting up V4L2 mode for rpi pipeline](https://wiki.veye.cc/index.php/V4L2_mode_for_Raspberry_Pi)

## Setup topology

Need to setup media pipeline topology:

- 1st:

```bash
[module] → 0:[csi2]:4 → 0:[rp1-cfe-fe_ch0 (/dev/video0)]
```

- 2nd:

```bash
[rp1-cfe-fe_config (/dev/video7)]0:→ ↘
                                      :1
      [module] → 0:[csi2]:4 → 0:[PISP FE]:2 → 0:[rp1-cfe-fe_image0 (/dev/video4)]
                                   :4
                                     ↘
                                       → 0:[rp1-cfe-fe_stats (/dev/video6)]
```
