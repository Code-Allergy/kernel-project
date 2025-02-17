#ifndef BBB_BOARD_H
#define BBB_BOARD_H

// watchdog timers
#define SOC_WDT_0_REGS                       (0x44E33000)
#define SOC_WDT_1_REGS                       (0x44E35000)

// BOARDINFO board_info;
#define EFUSE_OPP_MASK                       (0x00001FFFu)
#define DEVICE_VERSION_1_0                   (0u)
#define DEVICE_VERSION_2_0                   (1u)
#define DEVICE_VERSION_2_1                   (2u)

/* EFUSE OPP bit mask */
#define EFUSE_OPP_MASK                       (0x00001FFFu)

/* EFUSE bit for OPP100 275Mhz - 1.1v */
#define EFUSE_OPP100_275_MASK                (0x00000001u)
#define EFUSE_OPP100_275                     (0x0u)

/* EFUSE bit for OPP100 500Mhz - 1.1v */
#define EFUSE_OPP100_500_MASK                (0x00000002u)
#define EFUSE_OPP100_500                     (0x1u)

/* EFUSE bit for OPP120 600Mhz - 1.2v */
#define EFUSE_OPP120_600_MASK                (0x00000004u)
#define EFUSE_OPP120_600                     (0x2u)

/* EFUSE bit for OPP TURBO 720Mhz - 1.26v */
#define EFUSE_OPPTB_720_MASK                 (0x00000008u)
#define EFUSE_OPPTB_720                      (0x3u)

/* EFUSE bit for OPP50 300Mhz - 950mv */
#define EFUSE_OPP50_300_MASK                 (0x00000010u)
#define EFUSE_OPP50_300                      (0x4u)

/* EFUSE bit for OPP100 300Mhz - 1.1v */
#define EFUSE_OPP100_300_MASK                (0x00000020u)
#define EFUSE_OPP100_300                     (0x5u)

/* EFUSE bit for OPP100 600Mhz - 1.1v */
#define EFUSE_OPP100_600_MASK                (0x00000040u)
#define EFUSE_OPP100_600                     (0x6u)

/* EFUSE bit for OPP120 720Mhz - 1.2v */
#define EFUSE_OPP120_720_MASK                (0x00000050u)
#define EFUSE_OPP120_720                     (0x7u)

/* EFUSE bit for OPP TURBO 800Mhz - 1.26v */
#define EFUSE_OPPTB_800_MASK                 (0x00000100u)
#define EFUSE_OPPTB_800                      (0x8u)

/* EFUSE bit for OPP NITRO 1000Mhz - 1.325v */
#define EFUSE_OPPNT_1000_MASK                (0x00000200u)
#define EFUSE_OPPNT_1000                     (0x9u)

#define EFUSE_OPP_MAX                        (EFUSE_OPPNT_1000 + 1)
/* Types of Opp */
#define OPP_NONE                             (0u)
#define OPP_50                               (1u)
#define OPP_100                              (2u)
#define OPP_120                              (3u)
#define SR_TURBO                             (4u)
#define OPP_NITRO                            (5u)

#endif // BBB_BOARD_H
