#include "idt.h"
#include "../cpu/gdt.h"
#include "../cpu/privilege_level.h"

typedef struct __attribute__((__aligned__(8), __packed__)) {
    u16 offset_0_15;
    u16 segment_selector;
    u8 ist;
    u8 flags;
    u16 offset_16_31;
    u32 offset_32_63;
    u32 reserved;
} interrupt_descriptor;

typedef struct __attribute__((__packed__)) {
    const u16 limit;
    const interrupt_descriptor* data;
} idt_descriptor;

const int PRESENT_OFFSET = 7;
const int DPL_OFFSET = 5;
const int INTERRUPT_TYPE_OFFSET = 0;

interrupt_descriptor idt_data[256];
const idt_descriptor idt = {.data = idt_data, .limit = sizeof(idt_data) - 1};

u8 gen_interrupt_descriptor_flags(bool is_present, privilege_level dpl,
                                  bool is_trap_gate) {

    return ((is_present & 1) << PRESENT_OFFSET) | ((dpl & 0b11) << DPL_OFFSET)
           | ((is_trap_gate & 1) << INTERRUPT_TYPE_OFFSET) | 0b1110;
}

interrupt_descriptor gen_interrupt_descriptor(u16 segment_selector, u64 offset,
                                              bool is_present,
                                              privilege_level dpl,
                                              bool is_trap_gate) {

    interrupt_descriptor result = {
        .offset_0_15 = offset & 0xFFFF,
        .segment_selector = segment_selector,
        .ist = 0,
        .flags = gen_interrupt_descriptor_flags(is_present, dpl, is_trap_gate),
        .offset_16_31 = (offset >> 16) & 0xFFFF,
        .offset_32_63 = (offset >> 32) & 0xFFFFFFFF,
        .reserved = 0};

    return result;
}

#define SET_ISR(i)                                                             \
    extern void isr_##i();                                                     \
    idt_data[i] = gen_interrupt_descriptor(KERNEL_CODE_SEGMENT_SELECTOR,       \
                                           (u64) isr_##i, true, PL_3, false)

#define SET_ESR(i)                                                             \
    extern void esr_##i();                                                     \
    idt_data[i] = gen_interrupt_descriptor(KERNEL_CODE_SEGMENT_SELECTOR,       \
                                           (u64) esr_##i, true, PL_3, false)

void idt_init(void) {
    SET_ESR(0);
    SET_ESR(1);
    SET_ESR(2);
    SET_ESR(3);
    SET_ESR(4);
    SET_ESR(5);
    SET_ESR(6);
    SET_ESR(7);
    SET_ESR(8);
    SET_ESR(9);
    SET_ESR(10);
    SET_ESR(11);
    SET_ESR(12);
    SET_ESR(13);
    SET_ESR(14);
    SET_ESR(15);
    SET_ESR(16);
    SET_ESR(17);
    SET_ESR(18);
    SET_ESR(19);
    SET_ESR(20);
    SET_ESR(21);
    SET_ESR(22);
    SET_ESR(23);
    SET_ESR(24);
    SET_ESR(25);
    SET_ESR(26);
    SET_ESR(27);
    SET_ESR(28);
    SET_ESR(29);
    SET_ESR(30);
    SET_ESR(31);

    SET_ISR(32);
    SET_ISR(33);
    SET_ISR(34);
    SET_ISR(35);
    SET_ISR(36);
    SET_ISR(37);
    SET_ISR(38);
    SET_ISR(39);
    SET_ISR(40);
    SET_ISR(41);
    SET_ISR(42);
    SET_ISR(43);
    SET_ISR(44);
    SET_ISR(45);
    SET_ISR(46);
    SET_ISR(47);

    SET_ISR(48);
    SET_ISR(49);
    SET_ISR(50);
    SET_ISR(51);
    SET_ISR(52);
    SET_ISR(53);
    SET_ISR(54);
    SET_ISR(55);
    SET_ISR(56);
    SET_ISR(57);
    SET_ISR(58);
    SET_ISR(59);
    SET_ISR(60);
    SET_ISR(61);
    SET_ISR(62);
    SET_ISR(63);
    SET_ISR(64);
    SET_ISR(65);
    SET_ISR(66);
    SET_ISR(67);
    SET_ISR(68);
    SET_ISR(69);
    SET_ISR(70);
    SET_ISR(71);
    SET_ISR(72);
    SET_ISR(73);
    SET_ISR(74);
    SET_ISR(75);
    SET_ISR(76);
    SET_ISR(77);
    SET_ISR(78);
    SET_ISR(79);
    SET_ISR(80);
    SET_ISR(81);
    SET_ISR(82);
    SET_ISR(83);
    SET_ISR(84);
    SET_ISR(85);
    SET_ISR(86);
    SET_ISR(87);
    SET_ISR(88);
    SET_ISR(89);
    SET_ISR(90);
    SET_ISR(91);
    SET_ISR(92);
    SET_ISR(93);
    SET_ISR(94);
    SET_ISR(95);
    SET_ISR(96);
    SET_ISR(97);
    SET_ISR(98);
    SET_ISR(99);
    SET_ISR(100);
    SET_ISR(101);
    SET_ISR(102);
    SET_ISR(103);
    SET_ISR(104);
    SET_ISR(105);
    SET_ISR(106);
    SET_ISR(107);
    SET_ISR(108);
    SET_ISR(109);
    SET_ISR(110);
    SET_ISR(111);
    SET_ISR(112);
    SET_ISR(113);
    SET_ISR(114);
    SET_ISR(115);
    SET_ISR(116);
    SET_ISR(117);
    SET_ISR(118);
    SET_ISR(119);
    SET_ISR(120);
    SET_ISR(121);
    SET_ISR(122);
    SET_ISR(123);
    SET_ISR(124);
    SET_ISR(125);
    SET_ISR(126);
    SET_ISR(127);
    SET_ISR(128);
    SET_ISR(129);
    SET_ISR(130);
    SET_ISR(131);
    SET_ISR(132);
    SET_ISR(133);
    SET_ISR(134);
    SET_ISR(135);
    SET_ISR(136);
    SET_ISR(137);
    SET_ISR(138);
    SET_ISR(139);
    SET_ISR(140);
    SET_ISR(141);
    SET_ISR(142);
    SET_ISR(143);
    SET_ISR(144);
    SET_ISR(145);
    SET_ISR(146);
    SET_ISR(147);
    SET_ISR(148);
    SET_ISR(149);
    SET_ISR(150);
    SET_ISR(151);
    SET_ISR(152);
    SET_ISR(153);
    SET_ISR(154);
    SET_ISR(155);
    SET_ISR(156);
    SET_ISR(157);
    SET_ISR(158);
    SET_ISR(159);
    SET_ISR(160);
    SET_ISR(161);
    SET_ISR(162);
    SET_ISR(163);
    SET_ISR(164);
    SET_ISR(165);
    SET_ISR(166);
    SET_ISR(167);
    SET_ISR(168);
    SET_ISR(169);
    SET_ISR(170);
    SET_ISR(171);
    SET_ISR(172);
    SET_ISR(173);
    SET_ISR(174);
    SET_ISR(175);
    SET_ISR(176);
    SET_ISR(177);
    SET_ISR(178);
    SET_ISR(179);
    SET_ISR(180);
    SET_ISR(181);
    SET_ISR(182);
    SET_ISR(183);
    SET_ISR(184);
    SET_ISR(185);
    SET_ISR(186);
    SET_ISR(187);
    SET_ISR(188);
    SET_ISR(189);
    SET_ISR(190);
    SET_ISR(191);
    SET_ISR(192);
    SET_ISR(193);
    SET_ISR(194);
    SET_ISR(195);
    SET_ISR(196);
    SET_ISR(197);
    SET_ISR(198);
    SET_ISR(199);
    SET_ISR(200);
    SET_ISR(201);
    SET_ISR(202);
    SET_ISR(203);
    SET_ISR(204);
    SET_ISR(205);
    SET_ISR(206);
    SET_ISR(207);
    SET_ISR(208);
    SET_ISR(209);
    SET_ISR(210);
    SET_ISR(211);
    SET_ISR(212);
    SET_ISR(213);
    SET_ISR(214);
    SET_ISR(215);
    SET_ISR(216);
    SET_ISR(217);
    SET_ISR(218);
    SET_ISR(219);
    SET_ISR(220);
    SET_ISR(221);
    SET_ISR(222);
    SET_ISR(223);
    SET_ISR(224);
    SET_ISR(225);
    SET_ISR(226);
    SET_ISR(227);
    SET_ISR(228);
    SET_ISR(229);
    SET_ISR(230);
    SET_ISR(231);
    SET_ISR(232);
    SET_ISR(233);
    SET_ISR(234);
    SET_ISR(235);
    SET_ISR(236);
    SET_ISR(237);
    SET_ISR(238);
    SET_ISR(239);
    SET_ISR(240);
    SET_ISR(241);
    SET_ISR(242);
    SET_ISR(243);
    SET_ISR(244);
    SET_ISR(245);
    SET_ISR(246);
    SET_ISR(247);
    SET_ISR(248);
    SET_ISR(249);
    SET_ISR(250);
    SET_ISR(251);
    SET_ISR(252);
    SET_ISR(253);
    SET_ISR(254);
    SET_ISR(255);

    __asm__ volatile("lidt %0" : : "m"(idt));
}